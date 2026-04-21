#include "GameObjects/GP_Projectile.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Characters/GP_PlayerCharacter.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameplayTags/GPTags.h"
#include "Utils/GP_BlueprintLibrary.h"

AGP_Projectile::AGP_Projectile()
{
	// Projectile movement is handled entirely by UProjectileMovementComponent.
	// No per-frame logic needed on the actor itself.
	PrimaryActorTick.bCanEverTick = false;

	// -------------------------------------------------------------------------
	// MOVEMENT SETUP
	// -------------------------------------------------------------------------
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>("ProjectileMovement");
	ProjectileMovement->InitialSpeed = 3000.0f;

	// Zero gravity: projectile travels in a straight line.
	// Override in Blueprint for arcing projectiles (e.g., grenades).
	ProjectileMovement->ProjectileGravityScale = 0.0f;

	// -------------------------------------------------------------------------
	// REPLICATION
	// -------------------------------------------------------------------------
	// Projectiles must replicate so clients can see them moving through the world.
	// The server handles damage logic; clients only see the visual representation.
	bReplicates = true;

	// Safety net: destroy the projectile after 10 seconds if it doesn't hit anything.
	// Prevents orphaned projectiles from accumulating in the world.
	InitialLifeSpan = 10.0f;
}

void AGP_Projectile::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	// -------------------------------------------------------------------------
	// TARGET VALIDATION
	// -------------------------------------------------------------------------
	// 1. Must be a player character (enemies ignore friendly projectiles).
	// 2. Must be alive (don't apply damage to dead characters).
	// 3. Must have authority (damage logic runs on server only).
	AGP_PlayerCharacter* PlayerCharacter = Cast<AGP_PlayerCharacter>(OtherActor);
	if (!IsValid(PlayerCharacter) || !PlayerCharacter->IsAlive() || !HasAuthority())
	{
		return;
	}

	// -------------------------------------------------------------------------
	// APPLY DAMAGE
	// -------------------------------------------------------------------------
	UAbilitySystemComponent* AbilitySystemComponent = PlayerCharacter->GetAbilitySystemComponent();
	if (!ensureMsgf(AbilitySystemComponent,
	                TEXT("GP_Projectile::NotifyActorBeginOverlap - ASC is null on %s"),
	                *PlayerCharacter->GetName()))
	{
		return;
	}

	// Build the event payload with instigator/target context.
	// GetOwner() returns the enemy that spawned this projectile, providing
	// proper attribution for kill scoring and damage source tracking.
	FGameplayEventData Payload;
	Payload.Instigator = GetOwner();
	Payload.Target = PlayerCharacter;

	// Delegate damage application to the centralized utility function.
	// This ensures consistent GE creation, SetByCaller assignment,
	// and context setup across all damage sources in the project.
	UGP_BlueprintLibrary::SendDamageEventToPlayer(PlayerCharacter,
	                                              DamageEffect,
	                                              Payload,
	                                              GPTags::SetByCaller::Projectile,
	                                              Damage,
	                                              GPTags::None);

	// -------------------------------------------------------------------------
	// IMPACT & CLEANUP
	// -------------------------------------------------------------------------
	// Trigger Blueprint-side visual effects (particles, sounds)
	// before destroying the projectile actor.
	SpawnImpactEffects();
	Destroy();
}
