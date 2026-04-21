#include "AbilitySystem/Abilities/Enemy/GP_HitReact.h"
#include "GameplayTags/GPTags.h"
#include "Utils/GP_BlueprintLibrary.h"

UGP_HitReact::UGP_HitReact()
{
	// -------------------------------------------------------------------------
	// NETWORKING POLICY (AI SPECIFIC)
	// -------------------------------------------------------------------------
	// AI does not exist on clients, so prediction is impossible. 
	// ServerInitiated ensures the Authority runs the logic and replicates visuals.
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;

	// Required to store stateful variables like HitDirection during execution.
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// -------------------------------------------------------------------------
	// TAG & TRIGGER CONFIGURATION
	// -------------------------------------------------------------------------
	FGameplayTagContainer InitialTags;
	InitialTags.AddTag(GPTags::Abilities::Enemy::HitReact);
	SetAssetTags(InitialTags);

	// Setup the trigger so the ability wakes up automatically on this Event Tag.
	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = GPTags::Events::Enemy::HitReact;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);
}

void UGP_HitReact::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                   const FGameplayAbilityActorInfo* ActorInfo,
                                   const FGameplayAbilityActivationInfo ActivationInfo,
                                   const FGameplayEventData* TriggerEventData)
{
	// Soft Fail: Validate ActorInfo and Event Data
	if (!ensureMsgf(ActorInfo,
	                TEXT("UGP_HitReact::ActivateAbility - ActorInfo is null!")))
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

	// Extraction: If triggered by event, cache vectors using the Instigator from the payload.
	if (TriggerEventData && TriggerEventData->Instigator)
	{
		CacheHitDirectionInfo(TriggerEventData->Instigator.Get());
	}
	else
	{
		EndAbility(Handle,
		           ActorInfo,
		           ActivationInfo,
		           true,
		           false);
		return;
	}

	// Logic for playing the Montage based on HitDirectionName would follow here.
}

void UGP_HitReact::CacheHitDirectionInfo(const AActor* Instigator)
{
	if (!ensureMsgf(Instigator,
	                TEXT("UGP_HitReact::CacheHitDirectionInfo - Instigator is null!")))
	{
		return;
	}

	// AvatarActor remains non-const because it is "our" actor.
	// Future iterations may need to apply physics impulse, root motion, etc.
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!ensureMsgf(AvatarActor,
	                TEXT("UGP_HitReact::CacheHitDirectionInfo - AvatarActor is null!")))
	{
		return;
	}

	// 1. Calculate Vectors
	AvatarForward = AvatarActor->GetActorForwardVector();
	const FVector AvatarLocation = AvatarActor->GetActorLocation();

	// GetActorLocation() is const-qualified in AActor,
	// so it works seamlessly with a const AActor*.
	const FVector InstigatorLocation = Instigator->GetActorLocation();

	ToInstigator = (InstigatorLocation - AvatarLocation).GetSafeNormal();

	// 2. Resolve Direction Name using the Blueprint Library logic.
	// We reuse our centralized math logic to keep combat feeling consistent.
	const EHitDirection HitDir = UGP_BlueprintLibrary::GetHitDirection(AvatarForward,
	                                                                   ToInstigator);
	HitDirectionName = UGP_BlueprintLibrary::GetHitDirectionName(HitDir);

	if (bDrawDebugs && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1,
		                                 3.f,
		                                 FColor::Cyan,
		                                 FString::Printf(TEXT("HitReact: Hit received from %s direction."),
		                                                 *HitDirectionName.ToString()));
	}
}
