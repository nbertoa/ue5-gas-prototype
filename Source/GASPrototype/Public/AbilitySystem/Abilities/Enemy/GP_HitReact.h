#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GP_GameplayAbility.h"
#include "GP_HitReact.generated.h"

/**
 * @class UGP_HitReact
 * @brief Handles the physical reaction logic when an AI agent is struck.
 * * This ability is reactive and triggered via Gameplay Events. It calculates
 * the impact direction to determine which animation montage to play.
 */
UCLASS()
class GASPROTOTYPE_API UGP_HitReact : public UGP_GameplayAbility
{
	GENERATED_BODY()

public:
	UGP_HitReact();

	/**
	 * @brief GAS override to handle logic when the ability is triggered by an event.
	 */
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	                             const FGameplayAbilityActorInfo* ActorInfo,
	                             const FGameplayAbilityActivationInfo ActivationInfo,
	                             const FGameplayEventData* TriggerEventData) override;

	/**
	 * @brief Calculates and stores direction vectors based on the attacker's position.
	 * 
	 * @param Instigator The actor who caused the damage/impact. 
	 *        Const because this function only reads the instigator's position —
	 *        the hit reaction should never modify the attacker.
	 */
	UFUNCTION(BlueprintCallable,
		Category = "GAS|Abilities|HitReact")
	void CacheHitDirectionInfo(const AActor* Instigator);

protected:
	/** * @brief The primary montage containing hit reaction animations. 
	 * Expected to have sections named: "Front", "Back", "Left", "Right".
	 */
	UPROPERTY(EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "GAS|Config|Animations")
	TObjectPtr<UAnimMontage> MontageToPlay;

	/** Stores the forward vector of the avatar at the moment of impact. */
	UPROPERTY(BlueprintReadOnly,
		Category = "GAS|Abilities|HitReact")
	FVector AvatarForward;

	/** Stores the normalized vector pointing towards the instigator. */
	UPROPERTY(BlueprintReadOnly,
		Category = "GAS|Abilities|HitReact")
	FVector ToInstigator;

	/** Stores the calculated direction name (e.g., "Back", "Left") for Montage selection. */
	UPROPERTY(BlueprintReadOnly,
		Category = "GAS|Abilities|HitReact")
	FName HitDirectionName;
};
