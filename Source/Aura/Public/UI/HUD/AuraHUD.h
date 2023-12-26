// Copyright Kerem Avcil.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "UI/WidgetController/AttributeMenuWidgetController.h"
#include "AuraHUD.generated.h"

class UAbilitySystemComponent;
class UAttributeSet;
class UOverlayWidgetController;
class UAttributeMenuWidgetController;
class UAuraUserWidget;
struct FWidgetControllerParams;
class UAbilityMenuWidgetController;
/**
 * 
 */
UCLASS()
class AURA_API AAuraHUD : public AHUD
{
	GENERATED_BODY()
public:

	UOverlayWidgetController* GetOverlayWidgetController(const FWidgetControllerParams& WCParams);
	UAttributeMenuWidgetController* GetAttributeMenuWidgetController(const FWidgetControllerParams& WCParams);
	UAbilityMenuWidgetController* GetAbilityMenuWidgetController(const FWidgetControllerParams& WCParams);
	
	void InitOverlay(APlayerController* PC, APlayerState* PS, UAbilitySystemComponent* ASC, UAttributeSet* AS);
	
protected:
	

private:
	

	UPROPERTY()
	TObjectPtr<UAuraUserWidget>  OverlayWidget;	

	UPROPERTY(EditAnywhere)
	TSubclassOf<UAuraUserWidget> OverlayWidgetClass;

	UPROPERTY()
	TObjectPtr<UOverlayWidgetController> OverlayWidgetController;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UOverlayWidgetController> OverlayWidgetControllerClass;

	UPROPERTY()
	TObjectPtr<UAttributeMenuWidgetController> AttributeMenuWidgetController;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UAttributeMenuWidgetController> AttributeMenuWidgetControllerClass;

	UPROPERTY()
	TObjectPtr<UAbilityMenuWidgetController> AbilityMenuWidgetController;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UAbilityMenuWidgetController> AbilityMenuWidgetControllerClass;
	
};
