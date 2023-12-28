// Copyright Kerem Avcil.

#pragma once

#include "CoreMinimal.h"
#include "AuraGameplayTags.h"
#include "UI/WidgetController/AuraWidgetController.h"
#include "GameplayTagContainer.h"
#include "AbilityMenuWidgetController.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAbilitySlotSelectedSignature, bool, bSpendPointsButtonEnabled, bool, bEquipButtonEnabled);

struct FSelectedAbility
{
	FGameplayTag Ability = FGameplayTag();
	FGameplayTag Status = FGameplayTag();
};

/**
 * 
 */
UCLASS(BlueprintType, Blueprintable)
class AURA_API UAbilityMenuWidgetController : public UAuraWidgetController
{
	GENERATED_BODY()
public:
	
	virtual void BroadcastInitialValues() override;
	virtual void BindCallbacksToDependencies() override;

	UPROPERTY(BlueprintAssignable)
	FOnPlayerStatChangedSignature AbilityPointsChanged;

	UPROPERTY(BlueprintAssignable)
	FAbilitySlotSelectedSignature AbilitySlotSelectedDelegate;

	UFUNCTION(BlueprintCallable)
	void AbilitySlotSelected(const FGameplayTag& AbilityTag);

	UFUNCTION(BlueprintCallable)
	void SpendPointButtonPressed();
	
private:

	static void ShouldEnableButtons(const FGameplayTag& AbilityStatus, int32 AbilityPoints, bool& bShouldEnableAbilityPointsButton, bool& bShouldEnableEquipButton);

	FSelectedAbility SelectedAbility = { FAuraGameplayTags::Get().Abilities_None,  FAuraGameplayTags::Get().Abilities_Status_Locked };
	int32 CurrentAbilityPoints = 0;
};
