#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerState.h"
#include "GP_PlayerState.generated.h"

class UGP_AbilitySystemComponent;
class UAttributeSet; // Forward declaration of the base class

/**
 * @class AGP_PlayerState
 * @brief Custom PlayerState acting as the persistent owner for the Gameplay Ability System (GAS).
 *
 * @section Architecture GAS Ownership
 * This class implements IAbilitySystemInterface, making the ASC discoverable by the framework.
 * In multiplayer, the PlayerState is the ideal OwnerActor for the ASC because:
 * 1. It persists across Pawn possession/death cycles (keeping buffs/cooldowns active).
 * 2. It is always relevant to the owning client.
 */
UCLASS()
class GASPROTOTYPE_API AGP_PlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	/**
	 * @brief Constructor.
	 * Initializes the GAS components and sets high-frequency replication policies.
	 */
	AGP_PlayerState();

	/**
	 * @brief IAbilitySystemInterface implementation.
	 * @return UAbilitySystemComponent* The ASC managed by this PlayerState.
	 */
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	/**
	 * @brief Getter for the AttributeSet.
	 * Returns the base class UAttributeSet pointer to maintain loose coupling.
	 * To access specific attributes (like Health), casting to UGP_AttributeSet is required.
	 * * @return Pointer to the attribute set (base class).
	 */
	UAttributeSet* GetAttributeSet() const;

protected:
	/** * @brief Core component for GAS. 
	 * Handles replication of Abilities, Tags, and Effects.
	 */
	UPROPERTY(VisibleAnywhere,
		BlueprintReadOnly,
		Category = "GAS")
	TObjectPtr<UGP_AbilitySystemComponent> AbilitySystemComponent;

	/**
	 * @brief Container for gameplay attributes.
	 * Stored as a base class pointer for polymorphism.
	 */
	UPROPERTY(VisibleAnywhere,
		BlueprintReadOnly,
		Category = "GAS")
	TObjectPtr<UAttributeSet> AttributeSet;
};
