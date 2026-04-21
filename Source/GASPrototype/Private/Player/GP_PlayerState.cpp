#include "Player/GP_PlayerState.h"
#include "AbilitySystem/GP_AbilitySystemComponent.h"
#include "AbilitySystem/GP_AttributeSet.h"

AGP_PlayerState::AGP_PlayerState()
{
	// -------------------------------------------------------------------------
	// NETWORK PERFORMANCE
	// -------------------------------------------------------------------------
	// PlayerStates replicate rarely by default (Adaptive Frequency).
	// For GAS (Health bars, Cooldowns), we need high responsiveness.
	// 100Hz ensures the UI feels "snappy" when taking damage.
	SetNetUpdateFrequency(100.f);

	// -------------------------------------------------------------------------
	// GAS COMPONENT CREATION
	// -------------------------------------------------------------------------
	AbilitySystemComponent = CreateDefaultSubobject<UGP_AbilitySystemComponent>("AbilitySystemComponent");

	// Enable Replication explicitly (Components don't replicate by default).
	AbilitySystemComponent->SetIsReplicated(true);

	/**
	 * REPLICATION MODE: Mixed
	 * Standard for Player-Controlled characters (Heroes).
	 * * - Owner (The Player): Receives Full GameplayEffects. This allows for predictive 
	 * calculations (e.g., predicting cooldowns or cost deduction immediately).
	 * - Simulated Proxies (Other Players): Receive only GameplayTags and GameplayCues.
	 * They don't need to know the math of your damage buff, just that you are "Enraged" (Tag)
	 * and have a red particle effect (Cue).
	 */
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	// AttributeSet Initialization
	// We construct the SPECIFIC custom class, but store it in the BASE pointer variable.
	AttributeSet = CreateDefaultSubobject<UGP_AttributeSet>("AttributeSet");
}

UAbilitySystemComponent* AGP_PlayerState::GetAbilitySystemComponent() const
{
	// -------------------------------------------------------------------------
	// SAFETY CHECK
	// -------------------------------------------------------------------------
	// Soft Fail: ensureMsgf allows us to detect initialization errors without crashing.
	if (!ensureMsgf(AbilitySystemComponent,
	                TEXT("AGP_PlayerState::GetAbilitySystemComponent - Component is null on %s!"),
	                *GetName()))
	{
		return nullptr;
	}

	return AbilitySystemComponent;
}

UAttributeSet* AGP_PlayerState::GetAttributeSet() const
{
	// Soft Fail check for Attributes as well.
	if (!ensureMsgf(AttributeSet,
	                TEXT("AGP_PlayerState::GetAttributeSet - AttributeSet is null on %s!"),
	                *GetName()))
	{
		return nullptr;
	}

	return AttributeSet;
}
