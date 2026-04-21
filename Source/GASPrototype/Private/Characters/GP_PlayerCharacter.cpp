#include "Characters/GP_PlayerCharacter.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/GP_AttributeSet.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameplayTags/GPTags.h"
#include "Player/GP_PlayerState.h"

/**
 * Constructor: Configures capsule, movement defaults, camera boom,
 * and registers the Player gameplay tag as an Actor tag for runtime queries.
 */
AGP_PlayerCharacter::AGP_PlayerCharacter()
{
	// -------------------------------------------------------------------------
	// PERFORMANCE
	// -------------------------------------------------------------------------
	// Disable tick entirely — this character is event-driven through GAS.
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	// -------------------------------------------------------------------------
	// COLLISION & MOVEMENT
	// -------------------------------------------------------------------------
	GetCapsuleComponent()->InitCapsuleSize(42.f,
	                                       96.f);

	// Decouple controller rotation from character mesh to allow free-look.
	// The camera boom handles yaw follow via bUsePawnControlRotation instead.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* MoveComp = GetCharacterMovement();

	// Guard clause: MovementComponent should always exist on a Character,
	// but we guard defensively since this runs during CDO construction.
	if (ensureMsgf(MoveComp,
	               TEXT("AGP_PlayerCharacter: MovementComponent is null!")))
	{
		// Orient mesh toward movement direction instead of controller facing.
		MoveComp->bOrientRotationToMovement = true;
		MoveComp->RotationRate = FRotator(0.f,
		                                  540.f,
		                                  0.f);
		MoveComp->JumpZVelocity = 600.f;
		MoveComp->AirControl = 0.35f;
		MoveComp->MaxWalkSpeed = 500.f;
		MoveComp->BrakingDecelerationWalking = 2000.f;
	}

	// -------------------------------------------------------------------------
	// CAMERA SETUP
	// -------------------------------------------------------------------------
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>("CameraBoom");
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.f;
	// Boom rotates with the controller so the camera orbits the character.
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>("FollowCamera");
	FollowCamera->SetupAttachment(CameraBoom,
	                              USpringArmComponent::SocketName);
	// Camera itself does not rotate — the boom handles all rotation.
	FollowCamera->bUsePawnControlRotation = false;

	// -------------------------------------------------------------------------
	// ACTOR TAG REGISTRATION
	// -------------------------------------------------------------------------
	// Convert the native GameplayTag to FName and add it to the Actor's Tags
	// array. This allows systems that use UGameplayStatics::GetAllActorsWithTag
	// (which works with FName, not FGameplayTag) to find this character.
	FGameplayTag PlayerTag = GPTags::Player;
	Tags.Add(FName(PlayerTag.ToString()));
}

UAbilitySystemComponent* AGP_PlayerCharacter::GetAbilitySystemComponent() const
{
	// -------------------------------------------------------------------------
	// ARCHITECTURE: ASC LIVES ON PLAYER STATE
	// -------------------------------------------------------------------------
	// The ASC is owned by PlayerState for persistence across respawns.
	// Returns nullptr gracefully on Clients during the first frames while
	// PlayerState is still being replicated — callers must handle null.
	const AGP_PlayerState* GPPlayerState = GetPlayerState<AGP_PlayerState>();

	return GPPlayerState
		       ? GPPlayerState->GetAbilitySystemComponent()
		       : nullptr;
}

UAttributeSet* AGP_PlayerCharacter::GetAttributeSet() const
{
	// -------------------------------------------------------------------------
	// SAFETY: GRACEFUL NULL RETURN (NO CHECK)
	// -------------------------------------------------------------------------
	// Previous implementation used check(GPPlayerState) which caused hard
	// crashes on Client join when PlayerState hadn't replicated yet.
	// Now returns nullptr — UI and gameplay systems must handle null gracefully.
	const AGP_PlayerState* GPPlayerState = GetPlayerState<AGP_PlayerState>();

	return GPPlayerState
		       ? GPPlayerState->GetAttributeSet()
		       : nullptr;
}

void AGP_PlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// -------------------------------------------------------------------------
	// SERVER-SIDE GAS INITIALIZATION
	// -------------------------------------------------------------------------
	// PossessedBy runs ONLY on the Server. This is the authoritative place to
	// initialize the Ability System: link actor info, set attributes, grant abilities.

	AGP_PlayerState* GPPlayerState = GetPlayerState<AGP_PlayerState>();

	// Soft fail: PlayerState must exist on Server at this point.
	if (!ensureMsgf(GPPlayerState,
	                TEXT("PossessedBy: PlayerState is null or invalid type on %s"),
	                *GetName()))
	{
		return;
	}

	UAbilitySystemComponent* ASC = GPPlayerState->GetAbilitySystemComponent();

	if (ensureMsgf(ASC,
	               TEXT("PossessedBy: ASC is null on PlayerState for %s"),
	               *GetName()))
	{
		// 1. Link Owner (PlayerState) and Avatar (this Character) for GAS routing.
		ASC->InitAbilityActorInfo(GPPlayerState,
		                          this);

		// 2. Initialize attributes BEFORE granting abilities.
		//    Passive abilities may read attribute values on activation,
		//    so they need valid data from the start.
		InitializeAttributes();

		// 3. Grant startup abilities defined in the character's data configuration.
		GiveStartupAbilities();

		// 4. Bind to health changes for death detection / UI updates.
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

		// 5. Fire OnASCInitialized delegate so HUD/UI widgets can bind.
		BroadcastInitialValues();
	}
}

void AGP_PlayerCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	// -------------------------------------------------------------------------
	// CLIENT-SIDE GAS INITIALIZATION
	// -------------------------------------------------------------------------
	// OnRep_PlayerState fires on the Client when PlayerState finishes replicating.
	// The Client does NOT grant abilities or initialize attributes — the Server
	// is authoritative for those. We only link actor info for client-side
	// prediction and notify UI that pointers are ready.

	AGP_PlayerState* GPPlayerState = GetPlayerState<AGP_PlayerState>();

	// No ensure here intentionally: PlayerState replication can lag behind Pawn
	// replication, making null a normal (not erroneous) transient state.
	if (GPPlayerState)
	{
		UAbilitySystemComponent* ASC = GPPlayerState->GetAbilitySystemComponent();
		if (ASC)
		{
			// Link Owner and Avatar locally for ability prediction.
			ASC->InitAbilityActorInfo(GPPlayerState,
			                          this);

			// Notify HUD/UI that the ASC is available for binding.
			BroadcastInitialValues();
		}
	}
}
