#include "AbilitySystem/Abilities/Player/GP_Primary.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Actor.h"
#include "GameplayTags/GPTags.h"

UGP_Primary::UGP_Primary()
{
	// Default values for standard melee reach.
	HitBoxRadius = 80.0f;
	HitBoxForwardOffset = 150.0f;
	HitBoxElevationOffset = 0.0f;
}

void UGP_Primary::SendHitReactEventToActors(const TArray<AActor*>& ActorsHit)
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();

	// Early exit: nothing to do if there's no attacker or no targets.
	if (!AvatarActor || ActorsHit.IsEmpty())
	{
		return;
	}

	// Build payload with the attacker as Instigator.
	// The HitReact ability on the target reads this to calculate impact direction.
	FGameplayEventData Payload;
	Payload.Instigator = AvatarActor;

	for (AActor* HitActor : ActorsHit)
	{
		if (IsValid(HitActor))
		{
			UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(HitActor,
			                                                         GPTags::Events::Enemy::HitReact,
			                                                         Payload);
		}
	}
}
