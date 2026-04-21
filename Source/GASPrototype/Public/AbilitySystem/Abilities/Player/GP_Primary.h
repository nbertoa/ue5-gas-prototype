#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GP_GameplayAbility.h"
#include "GP_Primary.generated.h"

class AActor;

/**
 * @class UGP_Primary
 * @brief Primary melee ability for the player character.
 *
 * Uses GP_BlueprintLibrary::HitBoxOverlapTest for hit detection (shared with enemy melee).
 * Configurable sphere parameters allow per-ability tuning of reach and height.
 * Hit detection is triggered via AnimNotifies in the attack montage.
 */
UCLASS()
class GASPROTOTYPE_API UGP_Primary : public UGP_GameplayAbility
{
	GENERATED_BODY()

public:
	UGP_Primary();

	/**
	 * @brief Sends a HitReact Gameplay Event to each actor in the hit list.
	 * The event payload includes the attacker as Instigator, allowing
	 * the target's HitReact ability to calculate impact direction.
	 *
	 * @param ActorsHit List of actors to receive the HitReact event.
	 */
	UFUNCTION(BlueprintCallable,
		Category = "GAS|Abilities|Events")
	void SendHitReactEventToActors(const TArray<AActor*>& ActorsHit);

protected:
	/** @brief Radius of the detection sphere in centimeters. */
	UPROPERTY(EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "GAS|Abilities")
	float HitBoxRadius;

	/** @brief Distance from the character's pivot point to the sphere center. */
	UPROPERTY(EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "GAS|Abilities")
	float HitBoxForwardOffset;

	/** @brief Vertical adjustment to align the sphere with specific attack heights (e.g., high/low swings). */
	UPROPERTY(EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "GAS|Abilities")
	float HitBoxElevationOffset;
};
