#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GP_GameplayAbility.generated.h"

class AGP_BaseCharacter;
class AGP_PlayerController;

/**
 * @class UGP_GameplayAbility
 * @brief Foundational base class for all Gameplay Abilities in the project.
 *
 * Provides project-specific helpers for casting ActorInfo to custom types and
 * enforces consistent network/instancing policies across the ability system.
 */
UCLASS()
class GASPROTOTYPE_API UGP_GameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	/**
	 * @brief Sets project-wide default policies for new abilities.
	 */
	UGP_GameplayAbility();

	/**
	 * @brief Entry point for the ability execution logic.
	 */
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	                             const FGameplayAbilityActorInfo* ActorInfo,
	                             const FGameplayAbilityActivationInfo ActivationInfo,
	                             const FGameplayEventData* TriggerEventData) override;

	// -------------------------------------------------------------------------
	// ACCESSOR HELPERS
	// -------------------------------------------------------------------------

	/**
	 * @brief Retrieves the Avatar Actor cast to the project's Base Character type.
	 * @return Pointer to AGP_BaseCharacter or nullptr if the avatar is not a valid character.
	 */
	UFUNCTION(BlueprintPure,
		Category = "GAS|Ability|Data")
	AGP_BaseCharacter* GetGPCharacterFromActorInfo() const;

	/**
	 * @brief Retrieves the Player Controller for the character owning this ability.
	 * Note: This will return nullptr if the ability is owned by an AI controlled character.
	 * @return Pointer to AGP_PlayerController or nullptr.
	 */
	UFUNCTION(BlueprintPure,
		Category = "GAS|Ability|Data")
	AGP_PlayerController* GetGPPlayerControllerFromActorInfo() const;

protected:
	// -------------------------------------------------------------------------
	// DEBUGGING & VISUALIZATION
	// -------------------------------------------------------------------------

	/** * @brief Toggle for visual debug data (Screen messages, spheres, etc.). 
	 * Useful for tuning collision and timing during development.
	 */
	UPROPERTY(EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "GAS|Debug")
	bool bDrawDebugs = false;
};
