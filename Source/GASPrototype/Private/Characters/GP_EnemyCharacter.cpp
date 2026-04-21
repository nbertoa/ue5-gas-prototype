#include "Characters/GP_EnemyCharacter.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AIController.h"
#include "AbilitySystem/GP_AbilitySystemComponent.h"
#include "AbilitySystem/GP_AttributeSet.h"
#include "GameplayTags/GPTags.h"
#include "Net/UnrealNetwork.h"

AGP_EnemyCharacter::AGP_EnemyCharacter()
{
	// -------------------------------------------------------------------------
	// PERFORMANCE OPTIMIZATION
	// -------------------------------------------------------------------------
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	// -------------------------------------------------------------------------
	// COMPONENT CREATION
	// -------------------------------------------------------------------------
	AbilitySystemComponent = CreateDefaultSubobject<UGP_AbilitySystemComponent>("AbilitySystemComponent");
	AbilitySystemComponent->SetIsReplicated(true);

	/**
	 * REPLICATION MODE: Minimal
	 * Replicates ONLY GameplayTags and GameplayCues — not full GE data.
	 * Clients only need visual feedback (Tags/Cues), not AI math internals.
	 */
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	AttributeSet = CreateDefaultSubobject<UGP_AttributeSet>("AttributeSet");
}

void AGP_EnemyCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass,
	              bIsBeingLaunched);
}

void AGP_EnemyCharacter::StopMovementUntilLanded()
{
	bIsBeingLaunched = true;

	// Stop AI navigation immediately to prevent pathing during airtime.
	AAIController* AIController = GetController<AAIController>();
	if (!IsValid(AIController))
	{
		return;
	}
	AIController->StopMovement();

	// Register a one-shot callback for when the character lands.
	// IsAlreadyBound guard prevents duplicate registrations if the enemy
	// receives multiple knockbacks before landing from the first one.
	if (!LandedDelegate.IsAlreadyBound(this,
	                                   &ThisClass::EnableMovementOnLanded))
	{
		LandedDelegate.AddDynamic(this,
		                          &ThisClass::EnableMovementOnLanded);
	}
}

void AGP_EnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	// -------------------------------------------------------------------------
	// GAS INITIALIZATION (AI PATTERN)
	// -------------------------------------------------------------------------
	if (!ensureMsgf(AbilitySystemComponent,
	                TEXT("AGP_EnemyCharacter::BeginPlay - ASC is missing on %s"),
	                *GetName()))
	{
		return;
	}

	// For AI, Owner and Avatar are the same actor.
	AbilitySystemComponent->InitAbilityActorInfo(this,
	                                             this);

	// -------------------------------------------------------------------------
	// STARTUP LOGIC (AUTHORITY ONLY)
	// -------------------------------------------------------------------------
	if (HasAuthority())
	{
		InitializeAttributes();
		GiveStartupAbilities();

		// Bind Health change delegate for death detection.
		UGP_AttributeSet* GP_AttributeSet = Cast<UGP_AttributeSet>(GetAttributeSet());
		if (!ensureMsgf(GP_AttributeSet,
		                TEXT("%s: AttributeSet is null after InitializeAttributes!"),
		                *GetName()))
		{
			return;
		}

		GetAbilitySystemComponent()->GetGameplayAttributeValueChangeDelegate(GP_AttributeSet->GetHealthAttribute()).
		                             AddUObject(this,
		                                        &ThisClass::OnHealthChanged);
	}

	BroadcastInitialValues();
}

void AGP_EnemyCharacter::HandleDeath()
{
	Super::HandleDeath();

	// Stop AI navigation to prevent the corpse from continuing to move.
	AAIController* AIController = GetController<AAIController>();
	if (!IsValid(AIController))
	{
		return;
	}
	AIController->StopMovement();
}

void AGP_EnemyCharacter::EnableMovementOnLanded(const FHitResult& Hit)
{
	// Reset the launched state so the BT can resume navigation.
	bIsBeingLaunched = false;

	// Signal the Behavior Tree that the knockback sequence is complete.
	// This uses the same EndAttack event that attack montages use,
	// allowing the BT to treat knockback recovery identically to attack recovery.
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this,
	                                                         GPTags::Events::Enemy::EndAttack,
	                                                         FGameplayEventData());

	// Clean up: unregister to prevent stale callbacks on future landings
	// that aren't from knockback (e.g., normal jumps, falls).
	LandedDelegate.RemoveAll(this);
}

UAbilitySystemComponent* AGP_EnemyCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

UAttributeSet* AGP_EnemyCharacter::GetAttributeSet() const
{
	return AttributeSet;
}
