#include "TankWeaponComponent.h"

#include "Engine/World.h"
#include "TankGameplayMath.h"
#include "TankPawn.h"
#include "TankProjectile.h"
#include "Net/UnrealNetwork.h"

UTankWeaponComponent::UTankWeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

NetworkTankArena::LocalCombat::FFiringPose UTankWeaponComponent::GetFiringPose() const
{
	const ATankPawn* Tank = CastChecked<ATankPawn>(GetOwner());
	return GetFiringPose(Tank->GetCanonicalTurretWorldYaw());
}

NetworkTankArena::LocalCombat::FFiringPose UTankWeaponComponent::GetFiringPose(float ShotWorldYaw) const
{
	const ATankPawn* Tank = CastChecked<ATankPawn>(GetOwner());
	return NetworkTankArena::LocalCombat::CalculateFiringPose(Tank->GetActorLocation(),
		ShotWorldYaw, GameplayTurretHeight, GameplayMuzzleDistance);
}

bool UTankWeaponComponent::TryFire(const FVector& AimWorldPoint)
{
	ATankPawn* Tank = Cast<ATankPawn>(GetOwner());
	if (!IsValid(Tank) || !Tank->HasAuthority() || GetNetMode() == NM_Client
		|| Tank->IsActorBeingDestroyed() || !Tank->HasActorBegunPlay()
		|| !GetWorld() || GetWorld()->bIsTearingDown)
	{
		return false;
	}
	float ShotYaw;
	if (!NetworkTankArena::LocalCombat::TryCalculateFireYaw(Tank->GetActorLocation(), AimWorldPoint, ShotYaw)) { return false; }
	if (!FMath::IsFinite(GameplayTurretHeight) || !FMath::IsFinite(GameplayMuzzleDistance)
		|| GameplayMuzzleDistance < 0.f) { return false; }
	
	// Change: add cooldown for rapid fire or normal fire rate
	const double CurrentCooldown = (CurrentUpgrade == ETankWeaponUpgradeType::RapidFire) 
			? NetworkTankArena::LocalCombat::RapidFireCooldownSeconds
			: NetworkTankArena::LocalCombat::CooldownSeconds;
	
	if (!Cooldown.TryConsume(GetWorld()->GetTimeSeconds(), CurrentCooldown)) { return false; }
	
	// Change: move hit check to function so it can be invoked multiple times.
	if  (CurrentUpgrade == ETankWeaponUpgradeType::SpreadShot)
	{
		const float LeftYaw = ShotYaw - NetworkTankArena::LocalCombat::SpreadShotAngleDegrees;
		const float RightYaw = ShotYaw + NetworkTankArena::LocalCombat::SpreadShotAngleDegrees;
		
		FireProjectileAtYaw(ShotYaw, Tank);
		FireProjectileAtYaw(RightYaw, Tank);
		FireProjectileAtYaw(LeftYaw, Tank);
	}
	else
	{
		FireProjectileAtYaw(ShotYaw, Tank);
	}
	
	return true;
}

void UTankWeaponComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UTankWeaponComponent, CurrentUpgrade);
}

bool UTankWeaponComponent::FireProjectileAtYaw(float ShotYaw, ATankPawn* Tank)
{
	// Immutable gameplay snapshot: firing never writes CanonicalTurretWorldYaw.
	const auto Pose = GetFiringPose(ShotYaw);
	if (!NetworkTankArena::LocalTank::IsFiniteVector(Pose.Muzzle)
		|| !NetworkTankArena::LocalTank::IsFiniteVector(Pose.TurretOrigin)
		|| !NetworkTankArena::LocalTank::IsFiniteVector(Pose.Forward)) { return false; }
		// Change: moved cooldown check to before upgrade check in TryFire();

	FHitResult Hit;
	if (ATankProjectile::SweepPath(GetWorld(), Pose.TurretOrigin, Pose.Muzzle, Tank, nullptr, Hit))
	{
		// Obstruction consumes the shot and receives its one damage result. Never spawn beyond cover.
		ATankProjectile::ApplyAuthoritativeHit(Hit, Pose.Forward, Tank, Tank);
		return true;
	}
	const FTransform Transform(Pose.Forward.Rotation(), Pose.Muzzle);
	ATankProjectile* Projectile = GetWorld()->SpawnActorDeferred<ATankProjectile>(
		ATankProjectile::StaticClass(), Transform, Tank, Tank, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (Projectile)
	{
		Projectile->Initialize(Tank, Pose.Forward);
		Projectile->FinishSpawning(Transform);
	}
	return true; // The shot/cooldown is consumed even if spawning failed.
}

void UTankWeaponComponent::ClearUpgrade()
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority()) return;
	
	GetWorld()->GetTimerManager().ClearTimer(UpgradeExpirationTimer);
	CurrentUpgrade = ETankWeaponUpgradeType::None;
}

void UTankWeaponComponent::ApplyUpgrade(ETankWeaponUpgradeType NewUpgrade, float Duration)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority()) return;
	
	CurrentUpgrade = NewUpgrade;
	
	GetWorld()->GetTimerManager().ClearTimer(UpgradeExpirationTimer);
	
	if (CurrentUpgrade != ETankWeaponUpgradeType::None && Duration > 0.0f)
	{
		GetWorld()->GetTimerManager().SetTimer(UpgradeExpirationTimer, this, &UTankWeaponComponent::ClearUpgrade, Duration, false);
	}
}

void UTankWeaponComponent::OnRep_CurrentUpgrade()
{
	// For testing purposes to see if upgrade is applied.
    FString Weapon = TEXT("");
    switch (static_cast<uint8>(CurrentUpgrade))
    {
    case 0:
        Weapon = TEXT("Current Upgrade Type: None");
        break;
    case 1:
        Weapon = TEXT("Current Upgrade Type: SpreadShot");
        break;
    case 2:
        Weapon = TEXT("Current Upgrade Type: RapidFire");
        break;
    default:
        Weapon = TEXT("Current Upgrade Type: Unknown");
    }
    GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::White,  Weapon);
    // Trigger for visual FX or SFX.
}
