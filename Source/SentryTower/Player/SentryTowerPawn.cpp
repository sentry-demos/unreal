// Copyright (c) 2024 Sentry. All Rights Reserved.

#include "SentryTowerPawn.h"
#include "SentryTowerProjectile.h"
#include "SentryTowerTurret.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "SentryTower/Enemy/SentryTowerEnemyBase.h"
#include "Sound/SoundBase.h"

ASentryTowerPawn::ASentryTowerPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	Collision = CreateDefaultSubobject<UCapsuleComponent>(TEXT("RootComponent"));
	Collision->SetCollisionResponseToAllChannels(ECR_Overlap);
	Collision->SetCapsuleSize(300.0f, 600.0f);

	RootComponent = Collision;

	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComponent"));
	SpringArmComponent->SetupAttachment(RootComponent);
	SpringArmComponent->SetRelativeRotation(FRotator(-45.0f, -45.0f, 0.0));
	SpringArmComponent->TargetArmLength = 2000.0f;
	SpringArmComponent->bDoCollisionTest = false;

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->SetupAttachment(SpringArmComponent);

	TowerBase = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TowerBase"));
	TowerBase->SetupAttachment(RootComponent);
	TowerMid = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TowerMid"));
	TowerMid->SetupAttachment(TowerBase, FName("TopSocket"));
	TowerTop = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TowerTop"));
	TowerTop->SetupAttachment(TowerMid, FName("TopSocket"));

	Turret = CreateDefaultSubobject<UChildActorComponent>(TEXT("Turret"));
	Turret->SetupAttachment(TowerTop, FName("TopSocket"));

	DetectionSphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("DetectionSphere"));
	DetectionSphereComponent->SetupAttachment(RootComponent);
	DetectionSphereComponent->SetSphereRadius(1000.0f);
	DetectionSphereComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	DetectionSphereComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	DetectionSphereComponent->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	DetectionSphereComponent->OnComponentBeginOverlap.AddDynamic(this, &ASentryTowerPawn::OnEnemyEnterRange);
	DetectionSphereComponent->OnComponentEndOverlap.AddDynamic(this, &ASentryTowerPawn::OnEnemyExitRange);
}

void ASentryTowerPawn::SetProjectileType(TSubclassOf<ASentryTowerProjectile> ProjectileType)
{
	if (!ProjectileType)
	{
		return;
	}

	auto TurretActor = Cast<ASentryTowerTurret>(Turret->GetChildActor());
	if (!TurretActor)
	{
		return;
	}

	TurretActor->ProjectileType = ProjectileType;
}

void ASentryTowerPawn::OnEnemyEnterRange(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	ASentryTowerEnemyBase* Enemy = Cast<ASentryTowerEnemyBase>(OtherActor);
	if (Enemy && !EnemiesInRange.Contains(Enemy))
	{
		EnemiesInRange.Add(Enemy);
	}
}

void ASentryTowerPawn::OnEnemyExitRange(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	ASentryTowerEnemyBase* Enemy = Cast<ASentryTowerEnemyBase>(OtherActor);
	if (Enemy)
	{
		EnemiesInRange.Remove(Enemy);
	}
}

ASentryTowerEnemyBase* ASentryTowerPawn::GetClosestEnemy()
{
	ASentryTowerEnemyBase* Closest = nullptr;
	float MinDistSq = FLT_MAX;

	for (ASentryTowerEnemyBase* Enemy : EnemiesInRange)
	{
		if (!IsValid(Enemy))
		{
			continue;
		}

		float DistSq = FVector::DistSquared(GetActorLocation(), Enemy->GetActorLocation());
		if (DistSq < MinDistSq)
		{
			MinDistSq = DistSq;
			Closest = Enemy;
		}
	}

	return Closest;
}

void ASentryTowerPawn::BeginPlay()
{
	Super::BeginPlay();

	Health = MaxHealth;
}

float ASentryTowerPawn::TakeDamage(float Damage, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	Health -= Damage;

	if(Health <= 0.0f)
	{
		OnTowerDies.Broadcast();
	}

	UGameplayStatics::PlayWorldCameraShake(this, CameraShake, GetActorLocation(), 0.0f, 3000.0f);
	UGameplayStatics::PlaySound2D(GetWorld(), DamageSound);

	return Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
}

void ASentryTowerPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	auto Target = GetClosestEnemy();
	if (!Target)
	{
		// No enemies in range
		return;
	}

	auto TurretActor = Cast<ASentryTowerTurret>(Turret->GetChildActor());
	if (!TurretActor)
	{
		return;
	}

	TurretActor->RotateTurret(Target->GetActorLocation());

	if (TurretActor->IsFacingTarget(Target->GetActorLocation(), 10.0))
	{
		TurretActor->Shoot(Target, Target->GetActorLocation());
	}
}

void ASentryTowerPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void ASentryTowerPawn::GrantExperience(int32 Exp)
{
	CurrentExp += Exp;

	if(CurrentExp >= ExpForNextLevel)
	{
		CurrentLevel += 1;
		ExpForCurrentLevel = ExpForNextLevel;
		ExpForNextLevel = 100.0f * CurrentLevel + CurrentExp;

		OnTowerLevelUp.Broadcast();
	}
}
