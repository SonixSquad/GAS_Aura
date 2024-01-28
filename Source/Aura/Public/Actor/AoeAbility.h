// Copyright Kerem Avcil

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AoeAbility.generated.h"

UCLASS()
class AURA_API AAoeAbility : public AActor
{
	GENERATED_BODY()

public:	
	AAoeAbility();
	virtual void Tick(float DeltaTime) override;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UDecalComponent> AoeAbilityDecal;

protected:
	virtual void BeginPlay() override;

};