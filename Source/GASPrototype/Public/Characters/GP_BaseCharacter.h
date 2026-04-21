#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "GP_BaseCharacter.generated.h"

struct FOnAttributeChangeData;
class UAttributeSet;
class UAbilitySystemComponent;
class UGameplayAbility;
class UGameplayEffect;

/**
 * @brief Delegate signature to notify systems (like UI or HUD) that the ASC is ready 
 * and attributes have been initialized.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FASCInitialized,
                                             UAbilitySystemComponent*,
                                             ASC,
                                             UAttributeSet*,
                                             AS);

/**
 * @class AGP_BaseCharacter
 * @brief Abstract foundational class for all characters utilizing the Gameplay Ability System.
 *
 * This class acts as the 'AvatarActor'. It provides shared logic for granting initial 
 * abilities and setting up attributes via Gameplay Effects.
 */
UCLASS(Abstract)
class GASPROTOTYPE_API AGP_BaseCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AGP_BaseCharacter();

	/** @brief Registers variables for network replication. */
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	/**
	 * @brief Returns the Ability System Component. 
	 * Must be overridden by subclasses to point to where the ASC is stored (Pawn or PlayerState).
	 */
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	/**
	 * @brief Returns the Attribute Set.
	 * Useful for UI to bind to attribute changes.
	 */
	virtual UAttributeSet* GetAttributeSet() const { return nullptr; }

	/** @brief Returns whether this character is currently alive. */
	bool IsAlive() const { return bAlive; }

	/** @brief Manually sets the alive status. Used internally by death/respawn logic. */
	void SetAlive(bool bAliveStatus) { bAlive = bAliveStatus; }

	/**
	 * @brief Broadcasts when the ASC and Attributes are fully initialized.
	 * Blueprint-assignable to allow the HUD to listen for this event.
	 */
	UPROPERTY(BlueprintAssignable,
		Category = "GAS|Events")
	FASCInitialized OnASCInitialized;

	/**
	 * @brief Resets the character to an alive state.
	 * Virtual to allow subclasses to implement respawn-specific logic
	 * (e.g., restoring attributes, re-enabling collision, resetting position).
	 */
	UFUNCTION(BlueprintCallable,
		Category = "GAS|Death")
	virtual void HandleRespawn();

	/**
	 * @brief Resets character attributes by applying the ResetAttributesEffect.
	 * Intended to be called during respawn to restore Health, Mana, etc.
	 * Virtual to allow subclasses to add additional reset logic
	 * (e.g., clearing active GE debuffs, resetting cooldowns).
	 * @warning Server only — attribute changes replicate to clients automatically.
	 */
	UFUNCTION(BlueprintCallable,
		Category = "GAS|Attributes")
	virtual void ResetAttributes();

	/**
	 * @brief Maximum distance this character will search for targets.
	 * Used by FindClosestActorWithTag to limit spatial queries.
	 * Configurable per character type (e.g., melee enemies search shorter distances than ranged).
	 */
	UPROPERTY(EditAnywhere,
		BlueprintReadOnly,
		Category="GAS|AI")
	float SearchRange{1000.0f};

	/**
	 * @brief Vertical offset (in world units) for spawning floating damage numbers.
	 * Controls how far above the character's position the damage number widget appears.
	 * Configurable per character type to account for different mesh heights
	 * (e.g., tall bosses need a higher offset than standard enemies).
	 */
	UPROPERTY(BlueprintReadOnly,
		EditAnywhere,
		Category = "GAS|Damage")
	float DamageNumberVerticalOffset{200.f};

protected:
	/**
	 * @brief Grants the startup abilities to the ASC. 
	 * @warning Must be called on the Server (HasAuthority) during the initialization lifecycle.
	 */
	void GiveStartupAbilities();

	/**
	 * @brief Applies the initialization Gameplay Effect to set base attribute values.
	 * This ensures attributes start at defined values (Health, Mana, etc.) rather than zero.
	 * @warning Must be called on the Server.
	 */
	void InitializeAttributes();

	/**
	 * @brief Helper to fire the OnASCInitialized delegate.
	 * Call this at the end of PossessedBy (Server) or OnRep_PlayerState (Client).
	 */
	void BroadcastInitialValues();

	/**
	 * @brief Callback bound to the ASC's Health attribute change delegate.
	 * Monitors Health transitions and triggers HandleDeath when Health reaches zero.
	 *
	 * @param AttributeChangeData Contains NewValue, OldValue, and the Attribute that changed.
	 */
	void OnHealthChanged(const FOnAttributeChangeData& AttributeChangeData);

	/**
	 * @brief Called when Health reaches zero. Sets bAlive to false.
	 * Virtual to allow subclasses to implement death-specific behavior
	 * (e.g., ragdoll, disable input, play death montage, drop loot, stop AI movement).
	 */
	virtual void HandleDeath();

private:
	/** @brief Abilities granted automatically upon character initialization. */
	UPROPERTY(EditDefaultsOnly,
		Category = "GAS|Abilities")
	TArray<TSubclassOf<UGameplayAbility>> StartupAbilities;

	/**
	 * @brief Gameplay Effect used to initialize base attributes.
	 * Usually an 'Instant' GE with modifiers for MaxHealth, Health, MaxMana, Mana.
	 * Modifier order matters: Max attributes must be listed before Current
	 * to ensure valid clamping values during initialization.
	 */
	UPROPERTY(EditDefaultsOnly,
		Category = "GAS|Effects")
	TSubclassOf<UGameplayEffect> DefaultAttributesEffect;

	/**
	 * @brief Gameplay Effect used to reset attributes during respawn.
	 * Typically an 'Instant' GE with 'Override' modifiers that restore
	 * Health, Mana, etc. to their base values.
	 * Follows the same modifier ordering as DefaultAttributesEffect
	 * (Max attributes before Current) to ensure proper clamping.
	 */
	UPROPERTY(EditDefaultsOnly,
		Category = "GAS|Effects")
	TSubclassOf<UGameplayEffect> ResetAttributesEffect;

	/**
	 * @brief Tracks whether this character is alive.
	 * Replicated so all clients can query death state for visual/gameplay decisions.
	 * Set to false in HandleDeath, restored to true in HandleRespawn.
	 */
	UPROPERTY(BlueprintReadOnly,
		meta = (AllowPrivateAccess = "true"),
		Replicated)
	bool bAlive = true;
};
