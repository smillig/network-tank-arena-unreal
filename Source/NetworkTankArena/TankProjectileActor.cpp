// Fill out your copyright notice in the Description page of Project Settings.


#include "TankProjectileActor.h"
#include "GameFramework/ProjectileMovementComponent.h"

// Sets default values
ATankProjectileActor::ATankProjectileActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));
	SetRootComponent(StaticMeshComponent);
	StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	StaticMeshComponent->SetCollisionProfileName(TEXT("Projectile"));
	
	ProjectileComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileComponent->UpdatedComponent = StaticMeshComponent;
	ProjectileComponent->InitialSpeed = 800.0f;
	ProjectileComponent->MaxSpeed = 3000.0f;
	ProjectileComponent->bRotationFollowsVelocity = true;
	ProjectileComponent->bShouldBounce = false;
	
	bReplicates = true;
	SetReplicatingMovement(true);
}

// Called when the game starts or when spawned
void ATankProjectileActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ATankProjectileActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

