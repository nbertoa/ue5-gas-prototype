#include "AbilitySystem/GP_AttributeSet.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffectExtension.h"
#include "GameplayTags/GPTags.h"
#include "Math/UnrealMathUtility.h"
#include "Net/UnrealNetwork.h"

UGP_AttributeSet::UGP_AttributeSet()
{
}

void UGP_AttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// COND_None: Replicate to all clients (not just the owner).
	// REPNOTIFY_Always: Fire OnRep even if the value hasn't changed,
	// which is required for GAS prediction rollbacks.
	DOREPLIFETIME_CONDITION_NOTIFY(UGP_AttributeSet,
	                               Health,
	                               COND_None,
	                               REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UGP_AttributeSet,
	                               MaxHealth,
	                               COND_None,
	                               REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UGP_AttributeSet,
	                               Mana,
	                               COND_None,
	                               REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UGP_AttributeSet,
	                               MaxMana,
	                               COND_None,
	                               REPNOTIFY_Always);

	// bAttributesInitialized does not need REPNOTIFY_Always —
	// it only transitions once from false to true.
	DOREPLIFETIME(UGP_AttributeSet,
	              bAttributesInitialized);
}

void UGP_AttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute,
                                          float& NewValue)
{
	Super::PreAttributeChange(Attribute,
	                          NewValue);

	// -------------------------------------------------------------------------
	// CLAMPING (Pre-Modification / CurrentValue)
	// -------------------------------------------------------------------------
	// Clamps the CurrentValue before it's applied. This ensures queries and UI
	// always see values within valid range. However, PreAttributeChange does NOT
	// clamp BaseValue — periodic GEs with Add can accumulate BaseValue beyond Max.
	// PostGameplayEffectExecute handles BaseValue clamping separately.
	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue,
		                        0.0f,
		                        GetMaxHealth());
	}
	else if (Attribute == GetManaAttribute())
	{
		NewValue = FMath::Clamp(NewValue,
		                        0.0f,
		                        GetMaxMana());
	}
}

void UGP_AttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	// -------------------------------------------------------------------------
	// HEALTH CLAMPING (Post-Modification / BaseValue)
	// -------------------------------------------------------------------------
	// PreAttributeChange only clamps CurrentValue, not BaseValue.
	// Periodic GEs with Add accumulate on BaseValue, which can exceed Max.
	// Without this clamp, when a periodic GE expires, CurrentValue snaps back
	// to the unclamped BaseValue, causing Health/Mana to spike above Max.
	//
	// IsNearlyEqual guard: SetHealth/SetMana triggers the attribute change delegate.
	// If the value is already within range, calling Set would fire the delegate
	// a second time for the same GE execution, causing double UI updates.
	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		const float MaxVal = GetMaxHealth();
		if (MaxVal > 0.0f)
		{
			const float BaseValue = GetHealthAttribute().GetGameplayAttributeData(this)->GetBaseValue();
			if (BaseValue > MaxVal)
			{
				SetHealth(MaxVal);
			}
		}
	}
	// If MaxHealth shrinks (e.g., a debuff), clamp Health down to the new cap.
	else if (Data.EvaluatedData.Attribute == GetMaxHealthAttribute())
	{
		const float CurrentHealth = GetHealth();
		const float MaxVal = GetMaxHealth();
		if (MaxVal > 0.0f && CurrentHealth > MaxVal)
		{
			SetHealth(MaxVal);
		}
	}
	else if (Data.EvaluatedData.Attribute == GetManaAttribute())
	{
		const float MaxVal = GetMaxMana();
		if (MaxVal > 0.0f)
		{
			const float BaseValue = GetManaAttribute().GetGameplayAttributeData(this)->GetBaseValue();
			if (BaseValue > MaxVal)
			{
				SetMana(MaxVal);
			}
		}
	}
	// If MaxMana shrinks, clamp Mana down to the new cap.
	else if (Data.EvaluatedData.Attribute == GetMaxManaAttribute())
	{
		const float CurrentMana = GetMana();
		const float MaxVal = GetMaxMana();
		if (MaxVal > 0.0f && CurrentMana > MaxVal)
		{
			SetMana(MaxVal);
		}
	}

	// -------------------------------------------------------------------------
	// KILL SCORING
	// -------------------------------------------------------------------------
	// NOTE: This is intentionally NOT an else-if. Health clamping and kill scoring
	// can both be true simultaneously — Health was modified AND reached zero.
	// Keeping this as a separate if ensures kill scoring always fires when Health
	// drops to zero, regardless of whether clamping was needed.
	//
	// When a character's Health reaches zero, notify the damage instigator
	// by sending a KillScored event. This allows the killer to react
	// (score tracking, kill streaks, UI feedback) through a listener ability.
	//
	// Payload breakdown:
	//   Target of SendGameplayEventToActor = the attacker (who gets the kill credit)
	//   Payload.Instigator = the victim (the character that just died)
	if (Data.EvaluatedData.Attribute == GetHealthAttribute() && GetHealth() <= 0.0f)
	{
		FGameplayEventData Payload;
		Payload.Instigator = Data.Target.GetAvatarActor();
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Data.EffectSpec.GetEffectContext().GetInstigator(),
		                                                         GPTags::Events::KillScored,
		                                                         Payload);
	}

	// -------------------------------------------------------------------------
	// INITIALIZATION CHECK
	// -------------------------------------------------------------------------
	// Marks attributes as initialized after the first GE execution.
	// The DefaultAttributesEffect is always the first GE applied,
	// so this flag activates at the right moment for UI binding.
	if (!bAttributesInitialized)
	{
		bAttributesInitialized = true;
		OnAttributesInitialized.Broadcast();
	}
}

void UGP_AttributeSet::OnRep_AttributesInitialized()
{
	// Client-side broadcast: when the replicated flag arrives,
	// notify local listeners (UI widgets) that attributes are ready.
	if (bAttributesInitialized)
	{
		OnAttributesInitialized.Broadcast();
	}
}

// -------------------------------------------------------------------------
// REP NOTIFIES
// -------------------------------------------------------------------------
// These use the GAMEPLAYATTRIBUTE_REPNOTIFY macro which handles
// GAS internal bookkeeping for prediction rollback support.

void UGP_AttributeSet::OnRep_Health(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGP_AttributeSet,
	                            Health,
	                            OldValue);
}

void UGP_AttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGP_AttributeSet,
	                            MaxHealth,
	                            OldValue);
}

void UGP_AttributeSet::OnRep_Mana(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGP_AttributeSet,
	                            Mana,
	                            OldValue);
}

void UGP_AttributeSet::OnRep_MaxMana(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGP_AttributeSet,
	                            MaxMana,
	                            OldValue);
}
