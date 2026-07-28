// Copyright (c) 2024 Sentry. All Rights Reserved.

#include "SentryTowerTurret.h"

#include "SentryTowerProjectile.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Sound/SoundBase.h"

ASentryTowerTurret::ASentryTowerTurret()
{
	PrimaryActorTick.bCanEverTick = true;

	TurretBase = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TurretBase"));

	RootComponent = TurretBase;

	TurretWeapon = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TurretWeapon"));
	TurretWeapon->SetupAttachment(RootComponent);

	ProjectileSocket = CreateDefaultSubobject<USceneComponent>(TEXT("ProjectileSocket"));
	ProjectileSocket->SetupAttachment(TurretWeapon);
}

void ASentryTowerTurret::RotateTurret(const FVector& Target)
{
	FVector Direction = Target - GetActorLocation();
	Direction = Direction.GetSafeNormal();

	FRotator TargetRotation = UKismetMathLibrary::MakeRotFromX(Direction);
	TargetRotation.Pitch = FMath::Clamp(TargetRotation.Pitch, -40.0f, 0.0f);

	FRotator CurrentRotation = TurretWeapon->GetComponentRotation();

	FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, GetWorld()->GetDeltaSeconds(), RotationSpeed);

	TurretWeapon->SetWorldRotation(NewRotation);
}

bool ASentryTowerTurret::IsFacingTarget(const FVector& Target, float Tolerance) const
{
	FVector TargetDirection = (Target - GetActorLocation()).GetSafeNormal();
	FVector CurrentDirection = TurretWeapon->GetForwardVector();

	float Dot = FVector::DotProduct(TargetDirection, CurrentDirection);
	float Angle = FMath::RadiansToDegrees(FMath::Acos(Dot));

	return Angle <= Tolerance;
}

void ASentryTowerTurret::BeginPlay()
{
	Super::BeginPlay();
}

void ASentryTowerTurret::Shoot(AActor* TargetActor, const FVector& TargetLocation)
{
	float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastShotTime < AttackCooldown)
	{
		return;
	}

	LastShotTime = CurrentTime;

	FVector SpawnLocation = ProjectileSocket->GetComponentLocation();
	FRotator SpawnRotation = TurretWeapon->GetComponentRotation();

	auto Projectile = 
		Cast<ASentryTowerProjectile>(GetWorld()->SpawnActor(ProjectileType, &SpawnLocation, &SpawnRotation));

	if (Projectile)
	{
		Projectile->TargetToFollow = TargetActor;
		Projectile->TargetStationary = TargetLocation;

		Projectile->Init();

		UGameplayStatics::PlaySound2D(GetWorld(), ShootSound);
	}
}

void ASentryTowerTurret::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

