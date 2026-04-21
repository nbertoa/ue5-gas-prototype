#pragma once

#include "CoreMinimal.h"
#include "Characters/GP_BaseCharacter.h"
#include "GP_PlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UAbilitySystemComponent;
class UAttributeSet;

/**
 * @class AGP_PlayerCharacter
 * @brief Hero character controlled by a human player.
 *
 * @section Architecture GAS Hero Pattern
 * Implements the "Owner = PlayerState / Avatar = Character" pattern.
 * This ensures that attributes (Health, Mana) and Abilities persist even if this Pawn dies.
 */
UCLASS()
class GASPROTOTYPE_API AGP_PlayerCharacter : public AGP_BaseCharacter
{
	GENERATED_BODY()

public:
	AGP_PlayerCharacter();

	/**
	 * @brief Retrieves the ASC from the PlayerState.
	 * @return Pointer to the ASC, or nullptr if PlayerState is not yet valid (e.g. early client init).
	 */
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	/**
	 * @brief Retrieves the AttributeSet from the PlayerState.
	 * @return Pointer to the AttributeSet, or nullptr if PlayerState is not yet valid.
	 */
	virtual UAttributeSet* GetAttributeSet() const override;

	/**
	 * @brief Server-Side GAS Initialization.
	 * Entry point for setting up the ASC when the Controller possesses this Pawn.
	 */
	virtual void PossessedBy(AController* NewController) override;

	/**
	 * @brief Client-Side GAS Initialization.
	 * Entry point for setting up the ASC when the PlayerState is replicated to this client.
	 */
	virtual void OnRep_PlayerState() override;

private:
	/** Camera boom positioning the camera behind the character. */
	UPROPERTY(VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Camera",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	/** Follow camera providing the third-person view. */
	UPROPERTY(VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Camera",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FollowCamera;
};
