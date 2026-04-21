#pragma once

#include "CoreMinimal.h"
#include "Characters/GP_BaseCharacter.h"
#include "GP_EnemyCharacter.generated.h"

class UGP_AbilitySystemComponent;
class UGP_AttributeSet;

/**
 * @class AGP_EnemyCharacter
 * @brief Pawn class for AI-controlled agents using GAS.
 *
 * @section Architecture
 * The ASC and AttributeSet live directly on the Pawn, unlike PlayerCharacter
 * where they reside on PlayerState for persistence across respawns.
 * This is optimized for non-persistent actors (AI minions/monsters) that do not
 * need to carry state between different bodies or across level transitions.
 *
 * @see AGP_PlayerCharacter for the player-side ASC architecture.
 */
UCLASS()
class GASPROTOTYPE_API AGP_EnemyCharacter : public AGP_BaseCharacter
{
	GENERATED_BODY()

public:
	AGP_EnemyCharacter();

	/** @brief Registers variables for network replication. */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/**
	 * @brief Returns the Ability System Component managed by this character.
	 * Implements IAbilitySystemInterface.
	 */
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	/**
	 * @brief Returns the AttributeSet for this character.
	 * Useful for accessing Health/Mana directly or binding delegates.
	 */
	virtual UAttributeSet* GetAttributeSet() const override;

	/**
	 * @brief Maximum distance at which the AI considers itself "in range" for attacks.
	 * Used by the Behavior Tree to decide when to stop approaching and start attacking.
	 */
	UPROPERTY(EditAnywhere,
		BlueprintReadOnly,
		Category = "AI")
	float AcceptanceRadius{500.0f};

	/**
	 * @brief Minimum delay (seconds) between consecutive AI attacks.
	 * Combined with MaxAttackDelay to add randomized cadence variation,
	 * preventing all enemies from attacking in perfect sync.
	 */
	UPROPERTY(EditAnywhere,
		BlueprintReadOnly,
		Category = "AI")
	float MinAttackDelay{0.1f};

	/**
	 * @brief Maximum delay (seconds) between consecutive AI attacks.
	 * @see MinAttackDelay
	 */
	UPROPERTY(EditAnywhere,
		BlueprintReadOnly,
		Category = "AI")
	float MaxAttackDelay{0.5f};

	/**
	 * @brief Tracks whether this enemy is currently airborne from a knockback.
	 * Replicated so clients can reflect the launched state visually.
	 * Used by the Behavior Tree to suppress navigation while airborne.
	 */
	UPROPERTY(VisibleAnywhere,
		BlueprintReadOnly,
		Replicated)
	bool bIsBeingLaunched{false};

	/**
	 * @brief Stops AI movement and waits for the character to land before resuming.
	 * Called by ApplyKnockback before LaunchCharacter to prevent the AI
	 * from navigating mid-air. Registers a one-shot LandedDelegate callback
	 * that re-enables movement and signals the BT to resume.
	 */
	void StopMovementUntilLanded();

protected:
	/**
	 * @brief Handles GAS initialization sequence for AI.
	 * Sets ActorInfo, initializes attributes, grants startup abilities,
	 * binds the Health change delegate, and broadcasts initial values.
	 */
	virtual void BeginPlay() override;

	/**
	 * @brief Enemy-specific death behavior.
	 * Calls Super::HandleDeath() to set bAlive, then stops AI movement
	 * to prevent the corpse from continuing to navigate toward targets.
	 */
	virtual void HandleDeath() override;

private:
	/**
	 * @brief Callback fired when the character lands after being launched.
	 * Resets bIsBeingLaunched, sends EndAttack event to resume the BT,
	 * and unregisters itself from the LandedDelegate to prevent stale callbacks.
	 *
	 * @param Hit The hit result from the landing collision.
	 */
	UFUNCTION()
	void EnableMovementOnLanded(const FHitResult& Hit);

	/**
	 * @brief Core GAS component for handling abilities, effects, and tags.
	 * Owned directly by this Pawn (not by a PlayerState).
	 */
	UPROPERTY(VisibleAnywhere,
		BlueprintReadOnly,
		Category = "GAS",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UGP_AbilitySystemComponent> AbilitySystemComponent;

	/**
	 * @brief Numerical attributes (Health, Mana, Damage, etc.).
	 * Stored as a subobject created in the constructor, allowing
	 * default values to be overridden in Blueprint-derived classes.
	 */
	UPROPERTY(VisibleAnywhere,
		BlueprintReadOnly,
		Category = "GAS",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UGP_AttributeSet> AttributeSet;
};
