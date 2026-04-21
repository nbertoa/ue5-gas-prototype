#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "GP_AbilitySystemComponent.generated.h"

/**
 * @class UGP_AbilitySystemComponent
 * @brief Custom ASC handling automatic execution of passive abilities and network-safe leveling.
 *
 * @section Architecture Auto-Activation
 * Passive abilities are identified by the GPTags::Abilities::ActivateOnGiven tag.
 * When granted, the ASC automatically activates them on the appropriate network side:
 * - Server: via OnGiveAbility
 * - Client: via OnRep_ActivateAbilities (skipped on Listen Server to prevent duplicates)
 */
UCLASS(ClassGroup=(Custom),
	meta=(BlueprintSpawnableComponent))
class GASPROTOTYPE_API UGP_AbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	UGP_AbilitySystemComponent();

	/**
	 * @brief Server-side hook called immediately when GiveAbility is executed.
	 * Attempts auto-activation of passive abilities on the authority side.
	 */
	virtual void OnGiveAbility(FGameplayAbilitySpec& AbilitySpec) override;

	/**
	 * @brief Client-side hook called when the activatable abilities list is replicated.
	 * Attempts auto-activation of passive abilities on the client side.
	 * Skipped on Listen Server hosts to prevent duplicate activations.
	 */
	virtual void OnRep_ActivateAbilities() override;

	/**
	 * @brief Sets the level of a specific ability to an absolute value.
	 * Server-only: validates authority and ability ownership before modifying.
	 *
	 * @param AbilityClass The ability class to modify.
	 * @param Level The new absolute level to set.
	 */
	UFUNCTION(BlueprintCallable,
		Category = "GAS|Abilities")
	void SetAbilityLevel(TSubclassOf<UGameplayAbility> AbilityClass,
	                     int32 Level);

	/**
	 * @brief Adds to the current level of a specific ability.
	 * Server-only: validates authority and ability ownership before modifying.
	 *
	 * @param AbilityClass The ability class to modify.
	 * @param Level The amount to add to the current level (default: 1).
	 */
	UFUNCTION(BlueprintCallable,
		Category = "GAS|Abilities")
	void AddToAbilityLevel(TSubclassOf<UGameplayAbility> AbilityClass,
	                       int32 Level = 1);

protected:
	/**
	 * @brief Evaluates a granted ability and activates it if it has the ActivateOnGiven tag.
	 * Protected to allow derived ASCs (e.g., BossASC) to override auto-activation rules.
	 * Uses a TSet to track already-activated handles and prevent double activation.
	 *
	 * @param AbilitySpec The ability spec to evaluate for auto-activation.
	 */
	void HandleAutoActivatedAbility(FGameplayAbilitySpec& AbilitySpec);

private:
	/**
	 * @brief Tracks passive abilities that have already been auto-activated.
	 * TSet provides O(1) lookup to quickly skip already-processed abilities
	 * during OnRep_ActivateAbilities iterations.
	 */
	TSet<FGameplayAbilitySpecHandle> AutoActivatedAbilities;
};
