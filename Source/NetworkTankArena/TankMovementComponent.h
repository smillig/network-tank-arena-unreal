#pragma once

#include "CoreMinimal.h"
#include "TankControlIntent.h"
#include "GameFramework/PawnMovementComponent.h"
#include "TankMovementComponent.generated.h"

// Saved move stored on the client
struct FSavedTankMove
{
	uint32 SequenceNumber = 0;
	float DeltaTime = 0.f;
	FTankControlIntent Intent;
	FVector ResultingLocation = FVector::ZeroVector;
	FRotator ResultingRotation = FRotator::ZeroRotator;
};

// Response sent by the server back to the owning client
USTRUCT()
struct FTankServerMoveAck
{
	GENERATED_BODY()

	UPROPERTY()
	uint32 LastProcessedSequence = 0;
	UPROPERTY()
	FVector AuthoritativeLocation = FVector::ZeroVector;
	UPROPERTY()
	FRotator AuthoritativeRotation = FRotator::ZeroRotator;
};

struct FTankControlIntent;

UCLASS(ClassGroup = Movement, meta = (BlueprintSpawnableComponent))
class NETWORKTANKARENA_API UTankMovementComponent : public UPawnMovementComponent
{
	GENERATED_BODY()

public:
	UTankMovementComponent();
	void InitializeGameplayPlane();
	void SimulateMovement(const FTankControlIntent& Intent, float DeltaTime);
	double GetGameplayPlaneZ() const { return GetPlaneConstraintOrigin().Z; }
	virtual float GetMaxSpeed() const override;
	float GetTurnRateDegreesPerSecond() const { return TurnRateDegreesPerSecond; }
	// Change: tracking 
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
private:
	UPROPERTY(EditDefaultsOnly, Category = "Tank|Movement", meta = (ClampMin = "0", Units = "cm/s"))
	float ForwardSpeed = 500.f;

	UPROPERTY(EditDefaultsOnly, Category = "Tank|Movement", meta = (ClampMin = "0", Units = "cm/s"))
	float ReverseSpeed = 300.f;

	UPROPERTY(EditDefaultsOnly, Category = "Tank|Movement", meta = (ClampMin = "0", Units = "deg/s"))
	float TurnRateDegreesPerSecond = 120.f;

	bool bGameplayPlaneInitialized = false;
	
	// Change: sequence, buffer and server client messages for prediction on the client and reconciliation on the server 
	uint32 NextMoveSequence;
	TArray<FSavedTankMove> SavedMovesBuffer;
	
	UFUNCTION(NetMulticast, Reliable)
	void ServerSendMove(uint32 Sequence, float ClientDeltaTime, FTankControlIntent Intent);
	
	UFUNCTION(Client, Reliable)
	void ClientAcknowledgeMove(FTankServerMoveAck Ack);
};
