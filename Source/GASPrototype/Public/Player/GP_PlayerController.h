#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/PlayerController.h"
#include "GP_PlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
struct FInputActionValue;
class UAbilitySystemComponent;

/**
 * @class AGP_PlayerController
 * @brief Custom Player Controller handling Enhanced Input and GAS ability activation.
 *
 * @section Architecture Scalable Input Binding
 * Uses a payload-based pattern where each Input Action binding passes its
 * corresponding Gameplay Tag directly to a single generic handler (Input_AbilityPressed).
 * This eliminates per-ability boilerplate functions and makes adding new abilities
 * a one-line change in SetupInputComponent.
 */
UCLASS()
class GASPROTOTYPE_API AGP_PlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	/** @brief Registers Input Mapping Contexts with the Enhanced Input Subsystem. */
	virtual void BeginPlay() override;

protected:
	/** @brief Binds Input Actions to movement handlers and ability activation. */
	virtual void SetupInputComponent() override;

private:
	// -------------------------------------------------------------------------
	// INPUT CONFIGURATION
	// -------------------------------------------------------------------------

	/** @brief Input Mapping Contexts to register at BeginPlay. Supports multiple contexts (e.g., gameplay + UI). */
	UPROPERTY(EditDefaultsOnly,
		Category = "Settings|Input")
	TArray<TObjectPtr<UInputMappingContext>> InputMappingContexts;

	/** @brief Input Action for jumping. */
	UPROPERTY(EditDefaultsOnly,
		Category = "Settings|Input|Movement")
	TObjectPtr<UInputAction> JumpAction;

	/** @brief Input Action for movement (WASD / left stick). */
	UPROPERTY(EditDefaultsOnly,
		Category = "Settings|Input|Movement")
	TObjectPtr<UInputAction> MoveAction;

	/** @brief Input Action for camera look (mouse / right stick). */
	UPROPERTY(EditDefaultsOnly,
		Category = "Settings|Input|Movement")
	TObjectPtr<UInputAction> LookAction;

	/** @brief Input Action for Primary Ability. Maps to GPTags::Abilities::Player::Primary. */
	UPROPERTY(EditDefaultsOnly,
		Category = "Settings|Input|Ability")
	TObjectPtr<UInputAction> PrimaryAction;

	/** @brief Input Action for Secondary Ability. Maps to GPTags::Abilities::Player::Secondary. */
	UPROPERTY(EditDefaultsOnly,
		Category = "Settings|Input|Ability")
	TObjectPtr<UInputAction> SecondaryAction;

	/** @brief Input Action for Tertiary Ability. Maps to GPTags::Abilities::Player::Tertiary. */
	UPROPERTY(EditDefaultsOnly,
		Category = "Settings|Input|Ability")
	TObjectPtr<UInputAction> TertiaryAction;

	// -------------------------------------------------------------------------
	// INPUT HANDLERS
	// -------------------------------------------------------------------------

	/** @brief Triggers jump on the controlled character. */
	void Jump();

	/** @brief Stops jumping on the controlled character. */
	void StopJumping();

	/**
	 * @brief Moves the controlled pawn relative to the camera's yaw orientation.
	 * Uses only the Yaw component of the control rotation to prevent
	 * the character from moving vertically when looking up/down.
	 */
	void Move(const FInputActionValue& Value);

	/** @brief Applies yaw and pitch input from mouse/stick for camera control. */
	void Look(const FInputActionValue& Value);

	/**
	 * @brief Generic ability activation handler using the payload pattern.
	 * Receives the ability's Gameplay Tag directly from the Enhanced Input binding,
	 * eliminating the need for individual handler functions per ability.
	 * Adding a new ability only requires one new BindAction line in SetupInputComponent.
	 *
	 * @param InputTag The Gameplay Tag identifying which ability to activate.
	 */
	void Input_AbilityPressed(FGameplayTag InputTag);

	/**
	 * @brief O(1) retrieval of the ASC from the PlayerState.
	 * Bypasses the Pawn entirely, which is safer during death/respawn
	 * when the Pawn may be null or pending destruction.
	 *
	 * @return The ASC owned by this controller's PlayerState, or nullptr if unavailable.
	 */
	UAbilitySystemComponent* GetGASComponent() const;

	/**
	 * @brief Checks if the controlled character is alive.
	 * Used as a guard in all input handlers to prevent dead players
	 * from moving, jumping, looking, or activating abilities.
	 *
	 * @return True if the controlled Pawn is a valid, alive BaseCharacter.
	 */
	bool IsAlive() const;
};
