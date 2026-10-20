// Fill out your copyright notice in the Description page of Project Settings.


#include "WeaponUpgradeActor.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"
#include "TankPawn.h"
#include "TankWeaponComponent.h"

// Sets default values
AWeaponUpgradeActor::AWeaponUpgradeActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicatingMovement(false);
	bAlwaysRelevant = true;
	
	StaticMeshComp = CreateDefaultSubobject<UStaticMeshComponent>(FName("StaticMesh"));
	SetRootComponent(StaticMeshComp);
	StaticMeshComp->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	StaticMeshComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	StaticMeshComp->SetGenerateOverlapEvents(true);
	StaticMeshComp->SetSimulatePhysics(false);
	
	static ConstructorHelpers::FObjectFinder<UStaticMesh> LoadedRapidFireStaticMesh(TEXT("/Game/Meshes/TankPickupRapid"));
	if (LoadedRapidFireStaticMesh.Succeeded())
	{
		RapidFireStaticMesh = LoadedRapidFireStaticMesh.Object;
	}
	static ConstructorHelpers::FObjectFinder<UStaticMesh> LoadedSpreadShotStaticMesh(TEXT("/Game/Meshes/TankPickupSpread"));
	if (LoadedSpreadShotStaticMesh.Succeeded())
	{
		SpreadShotStaticMesh = LoadedSpreadShotStaticMesh.Object;
	}
}

// Called when the game starts or when spawned
void AWeaponUpgradeActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AWeaponUpgradeActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (InitialLocation.IsZero())
	{
		InitialLocation = GetActorLocation();
	}
	RunningTime += DeltaTime;
	const double BobZ = FMath::Sin(RunningTime * 3.0) * 10.0;
	SetActorLocation(InitialLocation + FVector(0.0, 0.0, BobZ));
	AddActorLocalRotation(FRotator(0.0, 90.0 * DeltaTime, 0.0));
}

void AWeaponUpgradeActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AWeaponUpgradeActor, UpgradeType);
}

void AWeaponUpgradeActor::InitializeUpgrade(ETankWeaponUpgradeType InType)
{
	if (HasAuthority())
	{
		UpgradeType = InType;
		OnRep_UpgradeType();
	}
}

void AWeaponUpgradeActor::OnRep_UpgradeType()
{
	if (UpgradeType == ETankWeaponUpgradeType::RapidFire)
	{
		if (RapidFireStaticMesh != nullptr)
		{
			StaticMeshComp->SetStaticMesh(RapidFireStaticMesh);
		}
		
	}
	else if (UpgradeType == ETankWeaponUpgradeType::SpreadShot)
	{
		if (SpreadShotStaticMesh != nullptr)
		{
			StaticMeshComp->SetStaticMesh(SpreadShotStaticMesh);
		}
	}
}

void AWeaponUpgradeActor::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	if (!HasAuthority() || GetNetMode() == NM_Client || bConsumed) return;

	ATankPawn* Tank = Cast<ATankPawn>(OtherActor);
	if (!Tank || Tank->IsActorBeingDestroyed()) return;

	UTankWeaponComponent* Weapon = Tank->GetWeaponComponent();
	if (!Weapon) return;

	bConsumed = true;
	Weapon->ApplyUpgrade(UpgradeType, NetworkTankArena::LocalCombat::UpgradeDurationSeconds);

	OnUpgradePickedUp.Broadcast(this);
	Destroy();
}