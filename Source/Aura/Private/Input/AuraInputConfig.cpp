// Copyright Kerem Avcil.


#include "Input/AuraInputConfig.h"



const UInputAction* UAuraInputConfig::FindAbilityInputActionForTag(const FGameplayTag& InputTag, bool bLogNotFound) const
{
	// Iterate through the InputActions array
	for (const FAuraInputAction& Action : AbilityInputActions)
	{
		// Check if the ActionTag of the current InputAction matches the desired ActionTag
		if (Action.InputAction && Action.InputTag == InputTag)
		{
			return Action.InputAction;
		}
	}

	if (bLogNotFound)
	{
		UE_LOG(LogTemp, Error, TEXT("Cant find AbilityInputAction for InputTag [%s], on InputConfig [%s]"), *InputTag.ToString(), *GetNameSafe(this));
	}

	return nullptr;
}
