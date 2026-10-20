// Fill out your copyright notice in the Description page of Project Settings.


#include "WeaponUpgradeSpawnerActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "WeaponUpgradeActor.h"
#include "TankCombat.h"

// Sets default values
AWeaponUpgradeSpawnerActor::AWeaponUpgradeSpawnerActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
	SetRootComponent(BaseMesh);
	BaseMesh->SetCollisionProfileName(TEXT("BlockAll"));
	BaseMesh->SetWorldScale3D(FVector(1.0f, 1.0f, 0.1f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (Cylinder.Succeeded())
	{
		BaseMesh->SetStaticMesh(Cylinder.Object);
	}

}

// Called when the game starts or when spawned
void AWeaponUpgradeSpawnerActor::BeginPlay()
{
	Super::BeginPlay();
	
	if (HasAuthority())
	{
		SpawnUpgrade();
	}
}

void AWeaponUpgradeSpawnerActor::SpawnUpgrade()
{
	if (!HasAuthority() || CurrentPickup.IsValid()) return;

	const ETankWeaponUpgradeType ChosenType = FMath::RandBool() 
		? ETankWeaponUpgradeType::SpreadShot 
		: ETankWeaponUpgradeType::RapidFire;

	const FVector SpawnLoc = GetActorLocation() + FVector(0.0, 0.0, 60.0);
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AWeaponUpgradeActor* UpgradeActor = GetWorld()->SpawnActor<AWeaponUpgradeActor>(
		AWeaponUpgradeActor::StaticClass(), SpawnLoc, FRotator::ZeroRotator, Params);

	if (UpgradeActor)
	{
		UpgradeActor->InitializeUpgrade(ChosenType);
		UpgradeActor->OnUpgradePickedUp.AddDynamic(this, &AWeaponUpgradeSpawnerActor::HandleUpgradePickedUp);
		CurrentPickup = UpgradeActor;
	}
}

void AWeaponUpgradeSpawnerActor::HandleUpgradePickedUp(AWeaponUpgradeActor* PickedUpActor)
{
	CurrentPickup.Reset();
	GetWorld()->GetTimerManager().SetTimer(RespawnTimerHandle, this, 
		&AWeaponUpgradeSpawnerActor::SpawnUpgrade, NetworkTankArena::LocalCombat::UpgradeRespawnSeconds, false);
}
