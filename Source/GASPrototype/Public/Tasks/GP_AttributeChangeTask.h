#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "GP_AttributeChangeTask.generated.h"

struct FOnAttributeChangeData;
class UAbilitySystemComponent;

/**
 * @brief Delegate signature for attribute change notifications in Blueprint.
 * Unpacks the FOnAttributeChangeData struct into individual values
 * so Blueprint designers don't need to interact with the C++ struct directly.
 *
 * @param Attribute  The gameplay attribute that changed.
 * @param NewValue   The attribute's value after the change.
 * @param OldValue   The attribute's value before the change.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnAttributeChanged,
                                               FGameplayAttribute,
                                               Attribute,
                                               float,
                                               NewValue,
                                               float,
                                               OldValue);

/**
 * @class UGP_AttributeChangeTask
 * @brief Async Blueprint task that listens for changes to a specific Gameplay Attribute.
 *
 * @section Architecture Blueprint Async Action
 * This class extends UBlueprintAsyncActionBase to create an asynchronous Blueprint node
 * that stays alive and fires its output pin every time the monitored attribute changes.
 * It is the Blueprint equivalent of binding to GetGameplayAttributeValueChangeDelegate in C++.
 *
 * @section Lifecycle
 * 1. Blueprint calls ListenForAttributeChange (factory function) → creates the task and binds to the ASC delegate.
 * 2. Every time the attribute changes → OnAttributeChanged fires with Attribute, NewValue, OldValue.
 * 3. Blueprint calls EndTask → unregisters the delegate and marks the task for garbage collection.
 *
 * ExposedAsyncProxy = AsyncTask exposes the task instance as an output pin on the Blueprint node,
 * allowing designers to store a reference and call EndTask later for cleanup.
 */
UCLASS(BlueprintType,
	meta = (ExposedAsyncProxy = AsyncTask))
class GASPROTOTYPE_API UGP_AttributeChangeTask : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	/**
	 * @brief Output delegate that fires every time the monitored attribute changes.
	 * Exposed as an output execution pin on the async Blueprint node.
	 */
	UPROPERTY(BlueprintAssignable)
	FOnAttributeChanged OnAttributeChanged;

	/**
	 * @brief Factory function that creates the task and binds to the attribute change delegate.
	 * BlueprintInternalUseOnly ensures this appears as an async node with output pins
	 * rather than a regular function call in the Blueprint palette.
	 *
	 * @param AbilitySystemComponent  The ASC to monitor for attribute changes.
	 * @param Attribute               The specific attribute to listen for (e.g., Health, Mana).
	 * @return The task instance, or nullptr if the ASC is invalid.
	 */
	UFUNCTION(BlueprintCallable,
		meta = (BlueprintInternalUseOnly = "true"))
	static UGP_AttributeChangeTask* ListenForAttributeChange(UAbilitySystemComponent* AbilitySystemComponent,
	                                                         FGameplayAttribute Attribute);

	/**
	 * @brief Cleans up the task by unregistering the delegate and marking for garbage collection.
	 * Must be called when the listener is no longer needed (e.g., in OnEndAbility)
	 * to prevent orphaned tasks from leaking memory.
	 */
	UFUNCTION(BlueprintCallable)
	void EndTask();

	/** @brief Weak reference to the ASC being monitored. Weak to avoid preventing GC if the ASC is destroyed. */
	TWeakObjectPtr<UAbilitySystemComponent> ASC;

	/** @brief Cached copy of the attribute this task is listening for. Used to unbind the correct delegate in EndTask. */
	FGameplayAttribute AttributeToListenFor;

	/**
	 * @brief Internal callback bound to the ASC's attribute change delegate.
	 * Receives the raw FOnAttributeChangeData from GAS and re-broadcasts
	 * the values through the Blueprint-friendly OnAttributeChanged delegate.
	 *
	 * @param Data The attribute change data provided by GAS (Attribute, NewValue, OldValue).
	 */
	void AttributeChanged(const FOnAttributeChangeData& Data);
};
