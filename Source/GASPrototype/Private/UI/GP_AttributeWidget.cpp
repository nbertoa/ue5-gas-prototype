#include "UI/GP_AttributeWidget.h"
#include "AbilitySystem/GP_AttributeSet.h"

void UGP_AttributeWidget::OnAttributeChange(const TTuple<FGameplayAttribute, FGameplayAttribute>& Pair,
                                            UGP_AttributeSet* InAttributeSet,
                                            float OldValue)
{
	// Soft Fail: UI can receive delayed updates from destroyed actors
	// or during seamless travel. Ensure the AttributeSet is still valid.
	if (!ensureMsgf(InAttributeSet,
	                TEXT("UGP_AttributeWidget::OnAttributeChange - Received null AttributeSet!")))
	{
		return;
	}

	// GetNumericValue resolves the current value of the attribute
	// (BaseValue + active modifiers).
	const float AttributeValue = Pair.Key.GetNumericValue(InAttributeSet);
	const float MaxAttributeValue = Pair.Value.GetNumericValue(InAttributeSet);

	// Push values to the visual layer (UMG) including OldValue for delta calculations.
	// Blueprint can use (OldValue - NewValue) for floating damage numbers,
	// or (NewValue / MaxValue) for progress bar percentages.
	BP_OnAttributeChange(AttributeValue,
	                     MaxAttributeValue,
	                     OldValue);
}

bool UGP_AttributeWidget::MatchesAttributes(const TTuple<FGameplayAttribute, FGameplayAttribute>& Pair) const
{
	return Attribute == Pair.Key && MaxAttribute == Pair.Value;
}
