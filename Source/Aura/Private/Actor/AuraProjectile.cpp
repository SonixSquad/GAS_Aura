// Copyright Kerem Avcil.

// #include ""

#include "Actor/AuraProjectile.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Aura/Aura.h"
#include "Components/AudioComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"


AAuraProjectile::AAuraProjectile()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	Sphere = CreateDefaultSubobject<USphereComponent>("Sphere");
	SetRootComponent(Sphere);
	Sphere->SetCollisionObjectType(ECC_PROJECTILE);
	Sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Sphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	Sphere->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	Sphere->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Overlap);
	Sphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>("ProjectileMovement");
	ProjectileMovement->InitialSpeed = 550.f;
	ProjectileMovement->MaxSpeed = 550.f;
	ProjectileMovement->ProjectileGravityScale = 0.f;
}

void AAuraProjectile::BeginPlay()
{
	Super::BeginPlay();
	SetLifeSpan(LifeSpan);
	Sphere->OnComponentBeginOverlap.AddDynamic(this, &AAuraProjectile::OnSphereOverlap);

	LoopingSoundComponent = UGameplayStatics::SpawnSoundAttached(LoopingSound, GetRootComponent());
}

// on destyroyed being called before the server authority replicates, checks if bHit was set to true by the server otherwise does nothing on client

void AAuraProjectile::Destroyed()
{
	if (!bHit && !HasAuthority())
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation(), FRotator::ZeroRotator);
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ImpactEffect, GetActorLocation());
		if (LoopingSoundComponent != nullptr)
		{
			LoopingSoundComponent->Stop();
			LoopingSoundComponent->DestroyComponent();
		}
	}
	Super::Destroyed();
}

void AAuraProjectile::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation(), FRotator::ZeroRotator);
	UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ImpactEffect, GetActorLocation());
	
	if (LoopingSoundComponent != nullptr)
	{
		LoopingSoundComponent->Stop();
		LoopingSoundComponent->DestroyComponent(); 
	}
	
	// checks if server authority otherwise just set bHit to true
	if (HasAuthority())
	{

		if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor))
		{
			TargetASC->ApplyGameplayEffectSpecToSelf(*DamageEffectSpecHandle.Data.Get());
		}
		
		Destroy();
	}
	else
	{
		bHit = true;
	}
}
