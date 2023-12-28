// Copyright Kerem Avcil.


#include "UI/WidgetController/AbilityMenuWidgetController.h"

#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "AbilitySystem/Data/AbilityInfo.h"
#include "Player/AuraPlayerState.h"

void UAbilityMenuWidgetController::BroadcastInitialValues()
{
	BroadcastAbilityInfo();
	AbilityPointsChanged.Broadcast(GetAuraPS()->GetAbilityPoints());
}

void UAbilityMenuWidgetController::BindCallbacksToDependencies()
{
	GetAuraASC()->AbilityStatusChanged.AddLambda([this](const FGameplayTag& AbilityTag, const FGameplayTag& StatusTag)
	{
		if (AbilityInfo)
		{
			FAuraAbilityInfo Info = AbilityInfo->FindAbilityInfoForTag(AbilityTag);
			Info.StatusTag = StatusTag;
			AbilityInfoDelegate.Broadcast(Info);
		}
	});

	GetAuraPS()->OnAbilityPointsChangedDelegate.AddLambda([this](int32 AbilityPoints)
	{
		AbilityPointsChanged.Broadcast(AbilityPoints);
	});
}
