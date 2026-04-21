#include "AbilitySystem/GP_AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayTags/GPTags.h"
#include "GASPrototype/GASPrototype.h"

UGP_AbilitySystemComponent::UGP_AbilitySystemComponent()
{
	// Components do not replicate by default. Enable explicitly for multiplayer.
	SetIsReplicatedByDefault(true);
}

void UGP_AbilitySystemComponent::OnGiveAbility(FGameplayAbilitySpec& AbilitySpec)
{
	Super::OnGiveAbility(AbilitySpec);

	// SERVER-SIDE LOGIC: Only the authority should auto-activate passives here.
	// This prevents Listen Server hosts from double-activating (once here, once in OnRep).
	if (IsOwnerActorAuthoritative())
	{
		HandleAutoActivatedAbility(AbilitySpec);
	}
}

void UGP_AbilitySystemComponent::OnRep_ActivateAbilities()
{
	Super::OnRep_ActivateAbilities();

	// CLIENT-SIDE LOGIC: Auto-activate passives that arrived via replication.
	// CRITICAL FIX: Skip on Listen Server hosts — they already ran this in OnGiveAbility.
	// Without this guard, passive abilities activate twice on Listen Servers,
	// causing duplicate delegate registrations and double-fired callbacks.
	if (!IsOwnerActorAuthoritative())
	{
		// SCOPED LOCK: Prevents the activatable abilities array from being compacted
		// or modified while we iterate, which could crash or skip entries.
		FScopedAbilityListLock ActiveScopeLock(*this);

		for (FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
		{
			HandleAutoActivatedAbility(AbilitySpec);
		}
	}
}

void UGP_AbilitySystemComponent::SetAbilityLevel(TSubclassOf<UGameplayAbility> AbilityClass,
                                                 int32 Level)
{
	// Guard: authority check + null class protection for Blueprint safety.
	if (!IsOwnerActorAuthoritative() || !AbilityClass)
	{
		return;
	}

	if (FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromClass(AbilityClass))
	{
		AbilitySpec->Level = Level;

		// Force replication of the updated spec to clients.
		MarkAbilitySpecDirty(*AbilitySpec);
	}
	else
	{
		UE_LOG(LogGP,
		       Warning,
		       TEXT("SetAbilityLevel: Attempted to level up %s, but %s does not own this ability."),
		       *GetNameSafe(AbilityClass),
		       *GetOwner()->GetName());
	}
}

void UGP_AbilitySystemComponent::AddToAbilityLevel(TSubclassOf<UGameplayAbility> AbilityClass,
                                                   int32 Level)
{
	if (!IsOwnerActorAuthoritative() || !AbilityClass)
	{
		return;
	}

	if (FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromClass(AbilityClass))
	{
		AbilitySpec->Level += Level;
		MarkAbilitySpecDirty(*AbilitySpec);
	}
}

void UGP_AbilitySystemComponent::HandleAutoActivatedAbility(FGameplayAbilitySpec& AbilitySpec)
{
	// Validate that the ability CDO is valid before accessing its tags.
	if (!AbilitySpec.Ability)
	{
		return;
	}

	// Fast exit O(1): skip if already active or previously auto-activated.
	// This prevents redundant TryActivateAbility calls during replication bursts.
	if (AbilitySpec.IsActive() || AutoActivatedAbilities.Contains(AbilitySpec.Handle))
	{
		return;
	}

	// Check if the CDO has the ActivateOnGiven tag, indicating it should
	// auto-activate immediately upon being granted (passive ability pattern).
	if (AbilitySpec.Ability->GetAssetTags().HasTagExact(GPTags::Abilities::ActivateOnGiven))
	{
		if (TryActivateAbility(AbilitySpec.Handle))
		{
			// Track the handle so this specific ability is never auto-activated again,
			// even if OnRep fires multiple times.
			AutoActivatedAbilities.Add(AbilitySpec.Handle);
		}
	}
}
