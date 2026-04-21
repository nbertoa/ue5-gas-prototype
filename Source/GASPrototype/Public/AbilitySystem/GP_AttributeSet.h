#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "GP_AttributeSet.generated.h"

// Standard macros for generating getters, setters, and initters.
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
    GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAttributesInitialized);

/**
 * @class UGP_AttributeSet
 * @brief Manages the numerical data (Attributes) for the Ability System.
 *
 * Handles replication, clamping (min/max limits), and reacts to attribute changes
 * triggered by Gameplay Effects.
 */
UCLASS()
class GASPROTOTYPE_API UGP_AttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UGP_AttributeSet();

	/** @brief Registers variables for network replication. */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/**
	 * @brief Called just before a Gameplay Effect modifies an attribute.
	 * Use this to enforce clamping on the *incoming* modification (e.g., prevent Health > MaxHealth).
	 */
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute,
	                                float& NewValue) override;

	/**
	 * @brief Called after a Gameplay Effect has executed.
	 * Use this to handle logic like "If Health <= 0, Die" or "If Damaged, Show Floating Text".
	 */
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	// -------------------------------------------------------------------------
	// INITIALIZATION STATE
	// -------------------------------------------------------------------------

	UPROPERTY(BlueprintAssignable,
		Category = "GAS|Events")
	FAttributesInitialized OnAttributesInitialized;

	/** Tracks if the initial attribute set has been applied. Replicated to ensure clients know when to update UI. */
	UPROPERTY(ReplicatedUsing = OnRep_AttributesInitialized)
	bool bAttributesInitialized = false;

	UFUNCTION()
	void OnRep_AttributesInitialized();

	// -------------------------------------------------------------------------
	// ATTRIBUTES
	// -------------------------------------------------------------------------

	UPROPERTY(BlueprintReadOnly,
		Category = "GAS|Attributes",
		ReplicatedUsing = OnRep_Health)
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UGP_AttributeSet,
	                    Health);

	UPROPERTY(BlueprintReadOnly,
		Category = "GAS|Attributes",
		ReplicatedUsing = OnRep_MaxHealth)
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UGP_AttributeSet,
	                    MaxHealth);

	UPROPERTY(BlueprintReadOnly,
		Category = "GAS|Attributes",
		ReplicatedUsing = OnRep_Mana)
	FGameplayAttributeData Mana;
	ATTRIBUTE_ACCESSORS(UGP_AttributeSet,
	                    Mana);

	UPROPERTY(BlueprintReadOnly,
		Category = "GAS|Attributes",
		ReplicatedUsing = OnRep_MaxMana)
	FGameplayAttributeData MaxMana;
	ATTRIBUTE_ACCESSORS(UGP_AttributeSet,
	                    MaxMana);

protected:
	/** Replication Notifies */
	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_Mana(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxMana(const FGameplayAttributeData& OldValue);
};
