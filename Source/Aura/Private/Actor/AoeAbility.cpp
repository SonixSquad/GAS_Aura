// Copyright Kerem Avcil


#include "Actor/AoeAbility.h"
#include "Components/DecalComponent.h"

AAoeAbility::AAoeAbility()
{
	PrimaryActorTick.bCanEverTick = true;
	
	
	AoeAbilityDecal = CreateDefaultSubobject<UDecalComponent>("AoeAbilityDecal");
	AoeAbilityDecal->SetupAttachment(GetRootComponent());
}

void AAoeAbility::BeginPlay()
{
	Super::BeginPlay();

}

void AAoeAbility::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}