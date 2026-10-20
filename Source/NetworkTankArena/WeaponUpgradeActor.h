// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TankCombat.h"
#include "WeaponUpgradeActor.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUpgradePickedUp, AWeaponUpgradeActor*, PickedUpUpgrade);

class UStaticMeshComponent;
class UStaticMesh;

UCLASS()
class NETWORKTANKARENA_API AWeaponUpgradeActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AWeaponUpgradeActor();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaTime) override;
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;
	
	void InitializeUpgrade(ETankWeaponUpgradeType InType);
	ETankWeaponUpgradeType GetUpgradeType() const { return UpgradeType; }
	
	FOnUpgradePickedUp OnUpgradePickedUp;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	UFUNCTION()
	void OnRep_UpgradeType();
	
	UPROPERTY(ReplicatedUsing = OnRep_UpgradeType, VisibleAnywhere, Category = "Tank|Upgrade")
	ETankWeaponUpgradeType UpgradeType = ETankWeaponUpgradeType::None;
	
	UPROPERTY(VisibleAnywhere, Category = "Tank|Upgrade")
	TObjectPtr<UStaticMeshComponent> StaticMeshComp;
	
	UPROPERTY()
	TObjectPtr<UStaticMesh> RapidFireStaticMesh;
	
	UPROPERTY()
	TObjectPtr<UStaticMesh> SpreadShotStaticMesh;

	bool bConsumed = false;
	FVector InitialLocation = FVector::ZeroVector;
	double RunningTime = 0.0;
};
