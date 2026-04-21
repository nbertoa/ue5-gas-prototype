#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GP_BlueprintLibrary.generated.h"

struct FGameplayEventData;
class UGameplayEffect;

/**
 * @struct FClosestActorWithTagResult
 * @brief Return value for spatial tag-based actor searches.
 * Uses TWeakObjectPtr to avoid holding a strong reference to the found actor,
 * which could prevent garbage collection if the actor is destroyed after the search.
 */
USTRUCT(BlueprintType)
struct FClosestActorWithTagResult
{
	GENERATED_BODY()

	/** @brief The closest actor found, or null if none matched. */
	UPROPERTY(BlueprintReadWrite)
	TWeakObjectPtr<AActor> Actor;

	/** @brief Distance from the origin to the found actor. Zero if no actor was found. */
	UPROPERTY(BlueprintReadWrite)
	float Distance{0.0f};
};

/**
 * @enum EHitDirection
 * @brief Represents the cardinal direction from which an impact originated relative to the target.
 */
UENUM(BlueprintType)
enum class EHitDirection : uint8
{
	Left,
	Right,
	Front,
	Back,
	None // Fallback for invalid calculations
};

/**
 * @class UGP_BlueprintLibrary
 * @brief Static utility functions for combat math and GAS-related helpers.
 */
UCLASS()
class GASPROTOTYPE_API UGP_BlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * @brief Calculates the hit direction based on the target's orientation and the vector to the instigator.
	 * @param TargetForward The forward vector of the character being hit.
	 * @param ToInstigator The normalized direction vector from the Target to the Instigator.
	 * @return EHitDirection enum representing the hit quadrant.
	 */
	UFUNCTION(BlueprintPure,
		Category = "GP|Math|Combat")
	static EHitDirection GetHitDirection(const FVector& TargetForward,
	                                     const FVector& ToInstigator);

	/**
	 * @brief Converts the EHitDirection enum to an FName for use in Animation Montage selection or Blackboard keys.
	 * @param HitDirection The enum value to convert.
	 * @return FName corresponding to the direction (e.g., "Back", "Left").
	 */
	UFUNCTION(BlueprintPure,
		Category = "GP|Math|Combat")
	static FName GetHitDirectionName(EHitDirection HitDirection);

	/**
	 * @brief Finds the closest alive character with a specific Actor Tag.
	 * Uses Unreal's Actor Tags system (AActor::Tags), not GAS Gameplay Tags.
	 * The FGameplayTag is converted to FName internally for GetAllActorsWithTag compatibility.
	 * Filters out dead characters and respects the SearchRange property on the caller
	 * if the WorldContextObject is a BaseCharacter.
	 *
	 * @param WorldContextObject The searching actor. If a BaseCharacter, its SearchRange property limits results.
	 * @param Origin The world-space point to measure distance from.
	 * @param GameplayTag The tag to search for (converted to FName for Actor Tags lookup).
	 * @param SearchRange Maximum search distance. Only enforced when WorldContextObject is a BaseCharacter.
	 * @return Struct containing the closest matching actor and its distance, or null/zero if none found.
	 */
	UFUNCTION(BlueprintCallable)
	static FClosestActorWithTagResult FindClosestActorWithTag(const UObject* WorldContextObject,
	                                                          const FVector& Origin,
	                                                          const FGameplayTag& GameplayTag,
	                                                          float SearchRange);

	/**
	 * @brief Applies a damage Gameplay Effect to a player target using SetByCallerMagnitude.
	 * Centralizes the damage application pattern so any system (projectiles, traps, area damage)
	 * can deal damage consistently without duplicating GE creation logic.
	 *
	 * @param Target The actor receiving the damage (must have an ASC).
	 * @param DamageEffect The GE class to apply (should have a SetByCaller modifier).
	 * @param Payload The event data containing Instigator and Target references.
	 *        Passed by mutable reference because OptionalObject is injected before sending.
	 * @param DataTag The SetByCaller tag key used to inject the damage magnitude into the GE.
	 * @param Damage The damage value to apply (positive — negated internally).
	 * @param EventTagOverride If not GPTags::None, bypasses the automatic lethality check
	 *        and sends this tag as the event directly. Use GPTags::None for default
	 *        HitReact/Death auto-detection based on remaining Health.
	 * @param OptionalParticleSystem Optional VFX reference passed to the receiving ability
	 *        via Payload.OptionalObject for impact-specific visual effects.
	 */
	UFUNCTION(BlueprintCallable)
	static void SendDamageEventToPlayer(AActor* Target,
	                                    const TSubclassOf<UGameplayEffect>& DamageEffect,
	                                    UPARAM(ref) FGameplayEventData& Payload,
	                                    const FGameplayTag& DataTag,
	                                    float Damage,
	                                    const FGameplayTag& EventTagOverride,
	                                    UObject* OptionalParticleSystem = nullptr);

	/**
	 * @brief Batch version of SendDamageEventToPlayer for multiple targets.
	 * Iterates the target array and applies damage to each one individually.
	 * Useful for area-of-effect abilities that hit multiple players simultaneously.
	 *
	 * @param Targets Array of actors to damage.
	 * @param DamageEffect The GE class to apply.
	 * @param Payload Event data (shared across all targets — Instigator stays the same).
	 * @param DataTag The SetByCaller tag key for damage magnitude.
	 * @param Damage The damage value (positive — negated internally per target).
	 * @param EventTagOverride Event tag override (GPTags::None for auto lethality check).
	 * @param OptionalParticleSystem Optional VFX reference.
	 */
	UFUNCTION(BlueprintCallable)
	static void SendDamageEventToPlayers(TArray<AActor*> Targets,
	                                     const TSubclassOf<UGameplayEffect>& DamageEffect,
	                                     UPARAM(ref) FGameplayEventData& Payload,
	                                     const FGameplayTag& DataTag,
	                                     float Damage,
	                                     const FGameplayTag& EventTagOverride,
	                                     UObject* OptionalParticleSystem = nullptr);

	/**
	 * @brief Performs a sphere overlap test to detect alive Pawn targets.
	 * Filters out dead characters and deduplicates actors with multiple collision components.
	 *
	 * @param AvatarActor The attacking actor (used for position, orientation, and self-exclusion).
	 * @param HitBoxRadius Radius of the detection sphere in centimeters.
	 * @param HitBoxForwardOffset Distance from the avatar's pivot to the sphere center along the forward vector.
	 * @param HitBoxElevationOffset Vertical offset for the sphere center (e.g., high/low swings).
	 * @param bDrawDebugs If true, draws debug spheres for the detection volume and hit actors.
	 * @return Array of unique, alive actors detected within the sphere.
	 */
	UFUNCTION(BlueprintCallable,
		Category = "GAS|Abilities")
	static TArray<AActor*> HitBoxOverlapTest(AActor* AvatarActor,
	                                         float HitBoxRadius,
	                                         float HitBoxForwardOffset = 0.0f,
	                                         float HitBoxElevationOffset = 0.0f,
	                                         bool bDrawDebugs = false);

	/**
	 * @brief Draws debug visualization for melee overlap detection.
	 * Red sphere = detection volume, green spheres = actors detected.
	 * Not exposed to Blueprint — called internally by HitBoxOverlapTest when bDrawDebugs is true.
	 *
	 * @param WorldContextObject Object used to retrieve the world reference.
	 * @param OverlapResults Raw overlap results from the sphere query.
	 * @param HitBoxLocation World-space center of the detection sphere.
	 * @param HitBoxRadius Radius used for the debug sphere visualization.
	 */
	static void DrawHitBoxOverlapDebugs(const UObject* WorldContextObject,
	                                    const TArray<FOverlapResult>& OverlapResults,
	                                    const FVector& HitBoxLocation,
	                                    float HitBoxRadius);

	/**
	 * @brief Applies distance-based knockback to hit actors using LaunchCharacter.
	 * Force scales linearly from full magnitude at InnerRadius to zero at OuterRadius.
	 * The launch direction is rotated upward by RotationAngle to create an arc effect.
	 *
	 * @param AvatarActor The attacking actor (knockback originates from its position).
	 * @param HitActors Array of actors to apply knockback to (must be ACharacter-derived).
	 * @param InnerRadius Distance within which full knockback force is applied.
	 * @param OuterRadius Distance beyond which no knockback is applied. Force falls off linearly between inner and outer.
	 * @param LaunchForceMagnitude Maximum launch force applied at InnerRadius or closer.
	 * @param RotationAngle Upward angle (degrees) applied to the knockback direction for arc effect. Default: 45.
	 * @param bDrawDebugs If true, prints force values and draws directional arrows.
	 * @return The input HitActors array (pass-through for Blueprint chaining).
	 */
	UFUNCTION(BlueprintCallable,
		Category = "GAS|Abilities")
	static TArray<AActor*> ApplyKnockback(AActor* AvatarActor,
	                                      const TArray<AActor*>& HitActors,
	                                      float InnerRadius,
	                                      float OuterRadius,
	                                      float LaunchForceMagnitude,
	                                      float RotationAngle = 45.0f,
	                                      bool bDrawDebugs = false);
};
