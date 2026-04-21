#include "Characters/GP_BaseCharacter.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "Abilities/GameplayAbility.h"
#include "Net/UnrealNetwork.h"

AGP_BaseCharacter::AGP_BaseCharacter()
{
	// -------------------------------------------------------------------------
	// PERFORMANCE
	// -------------------------------------------------------------------------
	// GAS-driven characters are event-based; polling via Tick is usually unnecessary.
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	// -------------------------------------------------------------------------
	// ANIMATION
	// -------------------------------------------------------------------------
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		/**
		 * SAFETY: GAS heavily relies on AnimNotifies (Montage callbacks). 
		 * If the mesh is off-screen and stops ticking, abilities using 'PlayMontageAndWait' may hang.
		 * We force bone updates to ensure logic-critical notifies always fire.
		 */
		MeshComp->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	}
}

void AGP_BaseCharacter::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass,
	              bAlive);
}

UAbilitySystemComponent* AGP_BaseCharacter::GetAbilitySystemComponent() const
{
	// Base implementation returns null. Subclasses (Player/Enemy) must override this.
	return nullptr;
}

void AGP_BaseCharacter::HandleRespawn()
{
	bAlive = true;
}

void AGP_BaseCharacter::ResetAttributes()
{
	// -------------------------------------------------------------------------
	// AUTHORITY CHECK
	// -------------------------------------------------------------------------
	// Attribute modifications must happen on the Server.
	// Updated values will replicate to clients automatically.
	if (!HasAuthority())
	{
		return;
	}

	// -------------------------------------------------------------------------
	// VALIDATION
	// -------------------------------------------------------------------------
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();

	if (!ensureMsgf(ASC,
	                TEXT("ResetAttributes: ASC is null on %s"),
	                *GetName()))
	{
		return;
	}

	if (!ensureMsgf(ResetAttributesEffect,
	                TEXT("ResetAttributes: ResetAttributesEffect not assigned on %s"),
	                *GetName()))
	{
		return;
	}

	// -------------------------------------------------------------------------
	// APPLY RESET EFFECT
	// -------------------------------------------------------------------------
	// Follows the same pattern as InitializeAttributes().
	// The GE should use 'Override' modifiers to forcefully set attributes
	// back to base values regardless of current state.
	FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
	ContextHandle.AddSourceObject(this);

	FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(ResetAttributesEffect,
	                                                             1.0f,
	                                                             ContextHandle);

	if (SpecHandle.IsValid())
	{
		ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}

void AGP_BaseCharacter::GiveStartupAbilities()
{
	// -------------------------------------------------------------------------
	// AUTHORITY CHECK
	// -------------------------------------------------------------------------
	// Granting abilities on clients causes desynchronization and security risks.
	// Abilities exist on the server and are replicated to the owning client.
	if (!HasAuthority())
	{
		return;
	}

	// -------------------------------------------------------------------------
	// VALIDATION
	// -------------------------------------------------------------------------
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();

	if (!ensureMsgf(ASC,
	                TEXT(
		                "GiveStartupAbilities: ASC is null on %s. Ensure GetAbilitySystemComponent is correctly overridden."
	                ),
	                *GetName()))
	{
		return;
	}

	// -------------------------------------------------------------------------
	// GRANT LOGIC
	// -------------------------------------------------------------------------
	for (const TSubclassOf<UGameplayAbility>& AbilityClass : StartupAbilities)
	{
		if (AbilityClass)
		{
			// Create the Spec (Ability Handle). Level 1 is standard for initialization.
			FGameplayAbilitySpec AbilitySpec(AbilityClass,
			                                 1);

			// Grant the ability to the ASC.
			ASC->GiveAbility(AbilitySpec);
		}
	}
}

void AGP_BaseCharacter::InitializeAttributes()
{
	// -------------------------------------------------------------------------
	// AUTHORITY CHECK
	// -------------------------------------------------------------------------
	// Attributes should only be initialized on the Server.
	// The AttributeSet will replicate the values down to clients.
	if (!HasAuthority())
	{
		return;
	}

	// -------------------------------------------------------------------------
	// VALIDATION
	// -------------------------------------------------------------------------
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();

	if (!ensureMsgf(ASC,
	                TEXT("InitializeAttributes: ASC is null on %s"),
	                *GetName()))
	{
		return;
	}

	if (!ensureMsgf(DefaultAttributesEffect,
	                TEXT("InitializeAttributes: DefaultAttributesEffect not assigned on %s"),
	                *GetName()))
	{
		return;
	}

	// -------------------------------------------------------------------------
	// APPLY EFFECT
	// -------------------------------------------------------------------------
	// Create a context for the effect (includes information about the instigator and causer).
	FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
	ContextHandle.AddSourceObject(this);

	// Create the Spec (the instance of the effect application).
	FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(DefaultAttributesEffect,
	                                                             1.f,
	                                                             ContextHandle);

	if (SpecHandle.IsValid())
	{
		// Apply the effect to self. This effectively runs the modifiers in the GE
		// to set Health, MaxHealth, etc.
		ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}

void AGP_BaseCharacter::BroadcastInitialValues()
{
	// Broadcast is safe to call with no listeners bound.
	// Parameters are resolved through the virtual overrides of each subclass.
	OnASCInitialized.Broadcast(GetAbilitySystemComponent(),
	                           GetAttributeSet());
}

void AGP_BaseCharacter::OnHealthChanged(const FOnAttributeChangeData& AttributeChangeData)
{
	// Only trigger death on the actual transition from alive to dead.
	// The bAlive guard prevents multiple HandleDeath calls from
	// simultaneous damage sources hitting in the same frame.
	if (bAlive && AttributeChangeData.NewValue <= 0.f)
	{
		HandleDeath();
	}
}

void AGP_BaseCharacter::HandleDeath()
{
	// Base implementation only sets the alive flag.
	// Subclasses override this to add death-specific behavior
	// (stop AI movement, play death montage, disable collision, etc.)
	// and must call Super::HandleDeath() to ensure bAlive is updated.
	bAlive = false;
}
