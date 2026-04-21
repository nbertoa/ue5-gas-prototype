#include "Player/GP_PlayerController.h"
#include "AbilitySystemComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Characters/GP_BaseCharacter.h"
#include "GameFramework/Character.h"
#include "GameplayTags/GPTags.h"
#include "Player/GP_PlayerState.h"

void AGP_PlayerController::BeginPlay()
{
	Super::BeginPlay();

	// -------------------------------------------------------------------------
	// MAPPING CONTEXT INITIALIZATION
	// -------------------------------------------------------------------------
	// Register all configured Input Mapping Contexts with the Enhanced Input Subsystem.
	// Priority 0 is standard — higher values would override lower ones for conflicting bindings.
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(
		GetLocalPlayer()))
	{
		for (UInputMappingContext* Context : InputMappingContexts)
		{
			if (Context)
			{
				Subsystem->AddMappingContext(Context,
				                             0);
			}
		}
	}
}

void AGP_PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(InputComponent);

	// -------------------------------------------------------------------------
	// MOVEMENT & LOOK
	// -------------------------------------------------------------------------
	EnhancedInputComponent->BindAction(JumpAction,
	                                   ETriggerEvent::Started,
	                                   this,
	                                   &ThisClass::Jump);
	EnhancedInputComponent->BindAction(JumpAction,
	                                   ETriggerEvent::Completed,
	                                   this,
	                                   &ThisClass::StopJumping);
	EnhancedInputComponent->BindAction(MoveAction,
	                                   ETriggerEvent::Triggered,
	                                   this,
	                                   &ThisClass::Move);
	EnhancedInputComponent->BindAction(LookAction,
	                                   ETriggerEvent::Triggered,
	                                   this,
	                                   &ThisClass::Look);

	// -------------------------------------------------------------------------
	// SCALABLE ABILITY BINDING (Payload Pattern)
	// -------------------------------------------------------------------------
	// Each binding passes its corresponding Gameplay Tag as a payload to a single
	// generic handler. The static_cast from FNativeGameplayTag to FGameplayTag
	// ensures the C++ template deduction matches the function signature exactly.
	//
	// To add a new ability slot:
	//   1. Declare a new UInputAction* property.
	//   2. Define a new tag in GPTags::Abilities::Player.
	//   3. Add one BindAction line here.
	EnhancedInputComponent->BindAction(PrimaryAction,
	                                   ETriggerEvent::Triggered,
	                                   this,
	                                   &ThisClass::Input_AbilityPressed,
	                                   static_cast<FGameplayTag>(GPTags::Abilities::Player::Primary));

	EnhancedInputComponent->BindAction(SecondaryAction,
	                                   ETriggerEvent::Started,
	                                   this,
	                                   &ThisClass::Input_AbilityPressed,
	                                   static_cast<FGameplayTag>(GPTags::Abilities::Player::Secondary));

	EnhancedInputComponent->BindAction(TertiaryAction,
	                                   ETriggerEvent::Started,
	                                   this,
	                                   &ThisClass::Input_AbilityPressed,
	                                   static_cast<FGameplayTag>(GPTags::Abilities::Player::Tertiary));
}

void AGP_PlayerController::Jump()
{
	ACharacter* ControlledCharacter = GetCharacter();
	if (ControlledCharacter && IsAlive())
	{
		ControlledCharacter->Jump();
	}
}

void AGP_PlayerController::StopJumping()
{
	ACharacter* ControlledCharacter = GetCharacter();
	if (ControlledCharacter && IsAlive())
	{
		ControlledCharacter->StopJumping();
	}
}

void AGP_PlayerController::Move(const FInputActionValue& Value)
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn || !IsAlive())
	{
		return;
	}

	const FVector2D MovementVector = Value.Get<FVector2D>();

	// Use only Yaw from the control rotation to prevent vertical movement
	// when the camera is looking up or down.
	const FRotator Rotation = GetControlRotation();
	const FRotator YawRotation(0.0f,
	                           Rotation.Yaw,
	                           0.0f);

	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	ControlledPawn->AddMovementInput(ForwardDirection,
	                                 MovementVector.Y);
	ControlledPawn->AddMovementInput(RightDirection,
	                                 MovementVector.X);
}

void AGP_PlayerController::Look(const FInputActionValue& Value)
{
	if (!IsAlive())
	{
		return;
	}

	const FVector2D LookAxisVector = Value.Get<FVector2D>();
	AddYawInput(LookAxisVector.X);
	AddPitchInput(LookAxisVector.Y);
}

void AGP_PlayerController::Input_AbilityPressed(FGameplayTag InputTag)
{
	if (!IsAlive())
	{
		return;
	}

	// Retrieve the ASC via PlayerState (not via Pawn) for safety during death/respawn.
	if (UAbilitySystemComponent* ASC = GetGASComponent())
	{
		// GetSingleTagContainer wraps the tag into the FGameplayTagContainer
		// required by TryActivateAbilitiesByTag.
		ASC->TryActivateAbilitiesByTag(InputTag.GetSingleTagContainer());
	}
}

UAbilitySystemComponent* AGP_PlayerController::GetGASComponent() const
{
	// O(1) retrieval: access the ASC directly from the PlayerState,
	// bypassing the Pawn. This is safer during death/respawn when
	// the Pawn may be null or pending destruction.
	if (AGP_PlayerState* GPPlayerState = GetPlayerState<AGP_PlayerState>())
	{
		return GPPlayerState->GetAbilitySystemComponent();
	}

	return nullptr;
}

bool AGP_PlayerController::IsAlive() const
{
	AGP_BaseCharacter* BaseCharacter = Cast<AGP_BaseCharacter>(GetPawn());
	return IsValid(BaseCharacter) && BaseCharacter->IsAlive();
}
