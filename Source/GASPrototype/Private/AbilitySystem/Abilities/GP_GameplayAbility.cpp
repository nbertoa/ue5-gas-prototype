#include "AbilitySystem/Abilities/GP_GameplayAbility.h"
#include "Characters/GP_BaseCharacter.h"
#include "Player/GP_PlayerController.h"

UGP_GameplayAbility::UGP_GameplayAbility()
{
	// -------------------------------------------------------------------------
	// INSTANCING POLICY: InstancedPerActor
	// -------------------------------------------------------------------------
	// We use InstancedPerActor so each character gets its own unique UObject instance.
	// This allows abilities to store internal state (hit counts, timers) safely
	// without clashing with other characters using the same ability class.
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// -------------------------------------------------------------------------
	// NET EXECUTION POLICY: LocalPredicted
	// -------------------------------------------------------------------------
	// Defaulting to LocalPredicted allows the client to execute logic immediately
	// while the server validates it. This provides the best "feel" for players.
	// IMPORTANT: AI abilities should override this to 'ServerInitiated' in subclasses.
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	// -------------------------------------------------------------------------
	// REPLICATION POLICY: ReplicateNo
	// -------------------------------------------------------------------------
	// We avoid replicating the Ability UObject itself to save bandwidth.
	// Ability state is typically synced via GameplayTags or GameplayEffects.
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateNo;
}

void UGP_GameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                          const FGameplayAbilityActorInfo* ActorInfo,
                                          const FGameplayAbilityActivationInfo ActivationInfo,
                                          const FGameplayEventData* TriggerEventData)
{
	// Soft Fail: Ensure ActorInfo is valid before calling Super
	if (!ensureMsgf(ActorInfo,
	                TEXT("UGP_GameplayAbility::ActivateAbility - ActorInfo is null!")))
	{
		EndAbility(Handle,
		           ActorInfo,
		           ActivationInfo,
		           true,
		           true);
		return;
	}

	Super::ActivateAbility(Handle,
	                       ActorInfo,
	                       ActivationInfo,
	                       TriggerEventData);

	// Standard Debugging Logic
	if (bDrawDebugs && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1,
		                                 3.f,
		                                 FColor::Cyan,
		                                 FString::Printf(TEXT("Ability Activated: %s"),
		                                                 *GetName()));
	}
}

AGP_BaseCharacter* UGP_GameplayAbility::GetGPCharacterFromActorInfo() const
{
	// Returns nullptr if cast fails; the caller is responsible for null checking.
	// This is the standard "Soft Fail" for pure getters.
	return Cast<AGP_BaseCharacter>(GetAvatarActorFromActorInfo());
}

AGP_PlayerController* UGP_GameplayAbility::GetGPPlayerControllerFromActorInfo() const
{
	// Safe access via ActorInfo. Note that AI controllers will cause this cast to return nullptr.
	if (CurrentActorInfo && CurrentActorInfo->PlayerController.IsValid())
	{
		return Cast<AGP_PlayerController>(CurrentActorInfo->PlayerController.Get());
	}
	else
	{
		return nullptr;
	}
}
