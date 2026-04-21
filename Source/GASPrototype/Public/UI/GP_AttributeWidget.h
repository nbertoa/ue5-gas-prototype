#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "Blueprint/UserWidget.h"
#include "GP_AttributeWidget.generated.h"

class UGP_AttributeSet;

/**
 * @class UGP_AttributeWidget
 * @brief Reusable UI Widget for displaying paired GAS Attributes (Current / Max).
 *
 * @section Architecture UI Decoupling
 * This widget does not fetch data itself. Instead, it expects an external controller
 * (like GP_WidgetComponent) to push data to it via OnAttributeChange.
 * This keeps the UI purely visual with no dependency on GAS internals.
 *
 * Designers subclass this in Blueprint, configure which attribute pair to track
 * via the Attribute/MaxAttribute properties, and implement BP_OnAttributeChange
 * to update progress bars, text blocks, or any visual representation.
 */
UCLASS(Abstract)
class GASPROTOTYPE_API UGP_AttributeWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * @brief The current value attribute (e.g., Health, Mana).
	 * Exposed to Blueprint so designers can configure which attribute this widget tracks.
	 */
	UPROPERTY(EditAnywhere,
		BlueprintReadOnly,
		Category = "GAS|Attributes")
	FGameplayAttribute Attribute;

	/**
	 * @brief The maximum value attribute (e.g., MaxHealth, MaxMana).
	 */
	UPROPERTY(EditAnywhere,
		BlueprintReadOnly,
		Category = "GAS|Attributes")
	FGameplayAttribute MaxAttribute;

	/**
	 * @brief Called by GP_WidgetComponent when the monitored attributes change.
	 * Extracts numeric values from the AttributeSet and forwards them to Blueprint
	 * along with the previous value for delta calculations (e.g., floating damage numbers).
	 *
	 * @param Pair The Current/Max attribute pair to read values from.
	 * @param InAttributeSet Pointer to the character's AttributeSet for value lookup.
	 * @param OldValue The attribute's value before the change, for delta calculation.
	 */
	void OnAttributeChange(const TTuple<FGameplayAttribute, FGameplayAttribute>& Pair,
	                       UGP_AttributeSet* InAttributeSet,
	                       float OldValue);

	/**
	 * @brief Checks if this widget is configured to track the given attribute pair.
	 * Used by GP_WidgetComponent to route attribute changes to the correct widgets.
	 *
	 * @param Pair The Current/Max attribute pair to test against.
	 * @return True if both Attribute and MaxAttribute match the pair.
	 */
	bool MatchesAttributes(const TTuple<FGameplayAttribute, FGameplayAttribute>& Pair) const;

	/**
	 * @brief Blueprint event triggered when attribute values update.
	 * Implement in Blueprint to update progress bars, text blocks, or animations.
	 *
	 * @param NewValue The current value (e.g., 50.f for Health).
	 * @param NewMaxValue The maximum value (e.g., 100.f for MaxHealth).
	 * @param OldValue The previous value before the change. Use NewValue - OldValue for delta
	 *        (e.g., floating damage numbers, heal indicators).
	 */
	UFUNCTION(BlueprintImplementableEvent,
		meta = (DisplayName = "On Attribute Change"))
	void BP_OnAttributeChange(float NewValue,
	                          float NewMaxValue,
	                          float OldValue);

	/**
	 * @brief Weak reference to the actor whose attributes this widget displays.
	 * Set by GP_WidgetComponent during binding. Useful in Blueprint for positioning
	 * floating text or directing visual effects to the correct actor.
	 */
	UPROPERTY(BlueprintReadOnly,
		Category = "GAS|Attributes")
	TWeakObjectPtr<AActor> AvatarActor;
};
