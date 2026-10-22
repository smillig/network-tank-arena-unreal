#include "TankMovementComponent.h"

#include "Components/PrimitiveComponent.h"
#include "GameFramework/Pawn.h"
#include "TankGameplayMath.h"
#include "TankPawn.h"

UTankMovementComponent::UTankMovementComponent()
{
	// Simulation is synchronous inside ATankPawn::Tick, never a second component tick.
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	bAutoUpdateTickRegistration = false;
	bTickBeforeOwner = false;
	SetPlaneConstraintNormal(FVector::UpVector);
	SetPlaneConstraintEnabled(true);
}

void UTankMovementComponent::InitializeGameplayPlane()
{
	if (IsValid(UpdatedComponent) && NetworkTankArena::LocalTank::IsFiniteVector(UpdatedComponent->GetComponentLocation()))
	{
		SetPlaneConstraintNormal(FVector::UpVector);
		SetPlaneConstraintOrigin(UpdatedComponent->GetComponentLocation());
		SetPlaneConstraintEnabled(true);
		bGameplayPlaneInitialized = true;
	}
}

float UTankMovementComponent::GetMaxSpeed() const
{
	using namespace NetworkTankArena::LocalTank;
	return FMath::Max(NonNegativeFinite(ForwardSpeed), NonNegativeFinite(ReverseSpeed));
}

void UTankMovementComponent::SimulateMovement(const FTankControlIntent& Intent, float DeltaTime)
{
	using namespace NetworkTankArena::LocalTank;
	if (!IsValid(PawnOwner) || !PawnOwner->HasAuthority())
	{
		return; // Clients consume Actor replication; they never simulate final gameplay transforms.
	}
	Velocity = FVector::ZeroVector;
	if (!IsValid(UpdatedPrimitive) || !bGameplayPlaneInitialized
		|| !IsValidDeltaTime(DeltaTime) || UpdatedPrimitive->IsSimulatingPhysics())
	{
		UpdateComponentVelocity();
		return;
	}

	const FVector OldLocation = UpdatedComponent->GetComponentLocation();
	if (!IsFiniteVector(OldLocation))
	{
		UpdateComponentVelocity();
		return;
	}
	UpdatedComponent->SetWorldScale3D(FVector::OneVector);
	const float Yaw = CalculateChassisYaw(UpdatedComponent->GetComponentRotation().Yaw,
		Intent.Turn, TurnRateDegreesPerSecond, DeltaTime);
	const FQuat Rotation = FRotator(0.0, Yaw, 0.0).Quaternion();

	// The upright, unit-scaled capsule is yaw-invariant. A box would need rotation collision handling.
	FHitResult Hit;
	SafeMoveUpdatedComponent(FVector::ZeroVector, Rotation, true, Hit);
	const FVector Delta = CalculateMovementDelta(Intent.Throttle, ForwardSpeed, ReverseSpeed,
		UpdatedComponent->GetComponentRotation().Yaw, DeltaTime);
	SafeMoveUpdatedComponent(Delta, Rotation, true, Hit);
	if (Hit.IsValidBlockingHit())
	{
		HandleImpact(Hit, DeltaTime, Delta);
		SlideAlongSurface(Delta, 1.f - Hit.Time, Hit.Normal, Hit, true);
	}
	const FVector ActualVelocity = (UpdatedComponent->GetComponentLocation() - OldLocation) / DeltaTime;
	Velocity = IsFiniteVector(ActualVelocity) ? ActualVelocity : FVector::ZeroVector;
	UpdateComponentVelocity();
}

// Change: local player sends more intents for server to assess
void UTankMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	ATankPawn* Tank = Cast<ATankPawn>(GetOwner());
	if (!Tank) return;

	if (Tank->IsLocallyControlled())
	{
		// 1. Gather current input intent
		FTankControlIntent CurrentIntent = Tank->GetPendingControlIntent();
		const uint32 Sequence = NextMoveSequence++;

		// 2. PREDICT: Simulate locally RIGHT NOW (0ms latency, 60+ FPS)
		SimulateMovement(CurrentIntent, DeltaTime);

		// 3. Save this predicted step into our buffer
		FSavedTankMove SavedMove;
		SavedMove.SequenceNumber = Sequence;
		SavedMove.DeltaTime = DeltaTime;
		SavedMove.Intent = CurrentIntent;
		SavedMove.ResultingLocation = UpdatedComponent->GetComponentLocation();
		SavedMove.ResultingRotation = UpdatedComponent->GetComponentRotation();
		SavedMovesBuffer.Add(SavedMove);

		// 4. Send this move to the server
		ServerSendMove(Sequence, DeltaTime, CurrentIntent);
	}
}

// Client sending intents on TickComponent
void UTankMovementComponent::ServerSendMove_Implementation(uint32 Sequence, float ClientDeltaTime, FTankControlIntent Intent)
{
	// Server simulates the exact same move with authority
	SimulateMovement(Intent, ClientDeltaTime);

	// Send the authoritative result back to the owning client
	FTankServerMoveAck Ack;
	Ack.LastProcessedSequence = Sequence;
	Ack.AuthoritativeLocation = UpdatedComponent->GetComponentLocation();
	Ack.AuthoritativeRotation = UpdatedComponent->GetComponentRotation();

	ClientAcknowledgeMove(Ack);
}

// Server ack and if needed corrects
void UTankMovementComponent::ClientAcknowledgeMove_Implementation(FTankServerMoveAck Ack)
{
	// Find the saved move that matches the server's ACK
	int32 AckedIndex = INDEX_NONE;
	for (int32 i = 0; i < SavedMovesBuffer.Num(); ++i)
	{
		if (SavedMovesBuffer[i].SequenceNumber == Ack.LastProcessedSequence)
		{
			AckedIndex = i;
			break;
		}
	}

	if (AckedIndex == INDEX_NONE) return;

	// Check if the server's result matches what we predicted
	const FSavedTankMove& AckedMove = SavedMovesBuffer[AckedIndex];
	const float ErrorDistSq = FVector::DistSquared(AckedMove.ResultingLocation, Ack.AuthoritativeLocation);

	// If error is larger than threshold (e.g. 2 cm), we must RECONCILE!
	if (ErrorDistSq > 4.0f)
	{
		// 1. REWIND: Snap back to the server's confirmed transform
		UpdatedComponent->SetWorldLocationAndRotation(Ack.AuthoritativeLocation, Ack.AuthoritativeRotation);

		// 2. REPLAY: Re-simulate all newer moves that the server hasn't seen yet
		for (int32 i = AckedIndex + 1; i < SavedMovesBuffer.Num(); ++i)
		{
			FSavedTankMove& MoveToReplay = SavedMovesBuffer[i];
			SimulateMovement(MoveToReplay.Intent, MoveToReplay.DeltaTime);

			// Update our buffer with the new corrected locations
			MoveToReplay.ResultingLocation = UpdatedComponent->GetComponentLocation();
			MoveToReplay.ResultingRotation = UpdatedComponent->GetComponentRotation();
		}
	}

	// Remove all acknowledged moves up to AckedIndex from buffer
	SavedMovesBuffer.RemoveAt(0, AckedIndex + 1);
}