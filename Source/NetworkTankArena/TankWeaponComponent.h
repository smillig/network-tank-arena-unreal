#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TankCombat.h"
#include "TankWeaponComponent.generated.h"

class ATankPawn;

UCLASS(ClassGroup = (Tank))
class NETWORKTANKARENA_API UTankWeaponComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UTankWeaponComponent();
	// Synchronous server processing. No cooldown retry or queued gameplay request.
	bool TryFire(const FVector& AimWorldPoint);
	NetworkTankArena::LocalCombat::FFiringPose GetFiringPose() const;
	NetworkTankArena::LocalCombat::FFiringPose GetFiringPose(float ShotWorldYaw) const;
	
	// Change: Need to replicate new upgrade state
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void ApplyUpgrade(ETankWeaponUpgradeType NewUpgrade, float Duration = NetworkTankArena::LocalCombat::UpgradeDurationSeconds);
	void ClearUpgrade();
	ETankWeaponUpgradeType GetCurrentUpgrade() const { return CurrentUpgrade; }

private:
	friend class FTankLocalWeaponTest;
	friend class FTankLocalCombatContentTest;
	
	// Change: properties and functions for the weapon to track
	bool FireProjectileAtYaw(float ShotYaw, ATankPawn* Tank);
	UPROPERTY(ReplicatedUsing = OnRep_CurrentUpgrade, VisibleAnywhere, Category = "Tank|Weapon")
	ETankWeaponUpgradeType CurrentUpgrade = ETankWeaponUpgradeType::None;
	
	UFUNCTION()
	void OnRep_CurrentUpgrade();
	
	FTimerHandle UpgradeExpirationTimer;
	
	// Fixed gameplay offsets, independent of the visual hierarchy and actor scale.
	UPROPERTY(EditDefaultsOnly, Category = "Tank|Weapon", meta = (Units = "cm"))
	float GameplayTurretHeight = -25.f;
	UPROPERTY(EditDefaultsOnly, Category = "Tank|Weapon", meta = (ClampMin = "0", Units = "cm"))
	float GameplayMuzzleDistance = 110.f;
	NetworkTankArena::LocalCombat::FWeaponCooldown Cooldown;
};
