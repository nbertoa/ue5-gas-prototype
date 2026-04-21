#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GP_Projectile.generated.h"

class UProjectileMovementComponent;
class UGameplayEffect;

/**
 * @class AGP_Projectile
 * @brief Base projectile actor that applies GAS damage on overlap with a player.
 *
 * Uses UProjectileMovementComponent for physics-free straight-line movement.
 * On overlap with a valid player target, applies a Gameplay Effect for damage
 * and triggers Blueprint-implementable impact effects before self-destructing.
 *
 * Designed to be spawned by enemy abilities (e.g., ranged attacks).
 * The Damage property is exposed on spawn for per-instance tuning.
 */
UCLASS()
class GASPROTOTYPE_API AGP_Projectile : public AActor
{
	GENERATED_BODY()

public:
	AGP_Projectile();

	/**
	 * @brief Handles collision with other actors.
	 * Filters for valid, alive player targets, applies the damage GE,
	 * spawns impact effects, and destroys this projectile.
	 * Only executes damage logic on the server (HasAuthority check).
	 *
	 * @param OtherActor The actor this projectile overlapped with.
	 */
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

	/**
	 * @brief Damage value applied to the target via SetByCallerMagnitude.
	 * Positive value representing damage dealt (e.g., 5.0 = "deal 5 damage").
	 * SendDamageEventToPlayer handles the negation internally before applying
	 * the GE, so this value should always be positive.
	 * ExposeOnSpawn allows setting this at spawn time from Blueprint.
	 */
	UPROPERTY(EditAnywhere,
		BlueprintReadOnly,
		Category = "Settings|Damage",
		meta = (ExposeOnSpawn))
	float Damage{5.0f};

	/**
	 * @brief Blueprint event called just before the projectile is destroyed.
	 * Implement in Blueprint to spawn particle effects, play sounds,
	 * or trigger camera shakes at the impact location.
	 */
	UFUNCTION(BlueprintImplementableEvent,
		Category = "Settings|Projectile")
	void SpawnImpactEffects();

private:
	/**
	 * @brief Handles projectile movement (velocity, trajectory, homing).
	 * Configured for straight-line movement with no gravity by default.
	 */
	UPROPERTY(VisibleAnywhere,
		Category = "Settings|Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	/**
	 * @brief Gameplay Effect applied to the target on impact.
	 * Should be an Instant GE with a modifier on Health (e.g., Health Add -X).
	 * The magnitude can optionally be driven by the Damage property
	 * via SetByCallerMagnitude for dynamic per-instance damage.
	 */
	UPROPERTY(EditDefaultsOnly,
		Category = "Settings|Damage")
	TSubclassOf<UGameplayEffect> DamageEffect;
};
