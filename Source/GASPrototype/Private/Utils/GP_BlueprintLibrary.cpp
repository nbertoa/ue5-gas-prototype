#include "Utils/GP_BlueprintLibrary.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayTagContainer.h"
#include "AbilitySystem/GP_AttributeSet.h"
#include "Characters/GP_BaseCharacter.h"
#include "Characters/GP_EnemyCharacter.h"
#include "Engine/OverlapResult.h"
#include "GameplayTags/GPTags.h"
#include "Kismet/GameplayStatics.h"

EHitDirection UGP_BlueprintLibrary::GetHitDirection(const FVector& TargetForward,
                                                    const FVector& ToInstigator)
{
	// Defensive: reject zero-length vectors to avoid NaN from normalization.
	if (TargetForward.IsNearlyZero() || ToInstigator.IsNearlyZero())
	{
		ensureMsgf(false,
		           TEXT("GetHitDirection: Provided vectors are nearly zero. Check input logic."));
		return EHitDirection::None;
	}

	// Normalize inputs for accurate Dot/Cross Product results.
	const FVector Forward = TargetForward.GetSafeNormal();
	const FVector DirectionToInstigator = ToInstigator.GetSafeNormal();

	// DOT PRODUCT determines Front vs Back using a 60-degree cone threshold:
	//   ~1.0 = vectors aligned (instigator in front of target)
	//  ~-1.0 = vectors opposite (instigator behind target)
	const float Dot = FVector::DotProduct(Forward,
	                                      DirectionToInstigator);

	if (Dot < -0.5f)
	{
		return EHitDirection::Back;
	}

	if (Dot > 0.5f)
	{
		return EHitDirection::Front;
	}

	// CROSS PRODUCT determines Left vs Right for lateral hits.
	// In Unreal's Z-up coordinate system, a negative Z component of
	// Forward × ToInstigator means the instigator is to the Left.
	const FVector Cross = FVector::CrossProduct(Forward,
	                                            DirectionToInstigator);

	if (Cross.Z < 0.0f)
	{
		return EHitDirection::Left;
	}

	return EHitDirection::Right;
}

FName UGP_BlueprintLibrary::GetHitDirectionName(EHitDirection HitDirection)
{
	// Static FNames avoid repeated string-to-name conversion at runtime.
	// NOTE: NameForward maps to "Front" string to match the EHitDirection::Front enum.
	static const FName NameLeft("Left");
	static const FName NameRight("Right");
	static const FName NameForward("Front");
	static const FName NameBack("Back");
	static const FName NameNone("None");

	switch (HitDirection)
	{
	case EHitDirection::Left: return NameLeft;
	case EHitDirection::Right: return NameRight;
	case EHitDirection::Front: return NameForward;
	case EHitDirection::Back: return NameBack;
	default: return NameNone;
	}
}

FClosestActorWithTagResult UGP_BlueprintLibrary::FindClosestActorWithTag(const UObject* WorldContextObject,
                                                                         const FVector& Origin,
                                                                         const FGameplayTag& GameplayTag,
                                                                         float SearchRange)
{
	// NOTE: This uses Unreal's Actor Tags system (AActor::Tags), not GAS Gameplay Tags.
	// The FGameplayTag is converted to FName for compatibility with GetAllActorsWithTag.
	// Actors must have the tag string (e.g., "GPTags.Player") added to their Tags array
	// in the editor Details panel for this lookup to find them.
	const FName Tag(GameplayTag.ToString());

	TArray<AActor*> ActorsWithTag;
	UGameplayStatics::GetAllActorsWithTag(WorldContextObject,
	                                      Tag,
	                                      ActorsWithTag);

	float ClosestDistance = TNumericLimits<float>::Max();
	AActor* ClosestActor = nullptr;

	// Resolve the effective search range.
	// If the caller is a BaseCharacter, use its per-character SearchRange property
	// (configured in Blueprint, allows melee vs ranged enemies to have different ranges).
	// Otherwise, fall back to the SearchRange parameter passed by the caller.
	const AGP_BaseCharacter* SearchingCharacter = Cast<AGP_BaseCharacter>(WorldContextObject);
	const float EffectiveSearchRange = IsValid(SearchingCharacter)
		                                   ? SearchingCharacter->SearchRange
		                                   : SearchRange;

	for (AActor* Actor : ActorsWithTag)
	{
		if (!IsValid(Actor))
		{
			continue;
		}

		// Filter out dead characters — AI should not target corpses.
		AGP_BaseCharacter* BaseCharacter = Cast<AGP_BaseCharacter>(Actor);
		if (!BaseCharacter || !BaseCharacter->IsAlive())
		{
			continue;
		}

		const float Distance = FVector::Dist(Origin,
		                                     Actor->GetActorLocation());

		// Skip actors beyond the effective search range.
		if (Distance > EffectiveSearchRange)
		{
			continue;
		}

		if (Distance < ClosestDistance)
		{
			ClosestDistance = Distance;
			ClosestActor = Actor;
		}
	}

	FClosestActorWithTagResult Result;
	Result.Actor = ClosestActor;

	// Only set distance if an actor was found.
	// If no actor matched, distance stays at the struct default (0.0f).
	Result.Distance = ClosestActor
		                  ? ClosestDistance
		                  : 0.0f;

	return Result;
}

void UGP_BlueprintLibrary::SendDamageEventToPlayer(AActor* Target,
                                                   const TSubclassOf<UGameplayEffect>& DamageEffect,
                                                   FGameplayEventData& Payload,
                                                   const FGameplayTag& DataTag,
                                                   float Damage,
                                                   const FGameplayTag& EventTagOverride,
                                                   UObject* OptionalParticleSystem)
{
	// -------------------------------------------------------------------------
	// TARGET VALIDATION
	// -------------------------------------------------------------------------
	AGP_BaseCharacter* PlayerCharacter = Cast<AGP_BaseCharacter>(Target);
	if (!IsValid(PlayerCharacter) || !PlayerCharacter->IsAlive())
	{
		return;
	}

	// -------------------------------------------------------------------------
	// EVENT TAG RESOLUTION
	// -------------------------------------------------------------------------
	// If the caller provides a specific event tag override, use it directly.
	// Otherwise, perform the automatic lethality check to determine whether
	// to send HitReact (non-lethal) or Death (lethal) based on remaining Health.
	FGameplayTag EventTag;
	if (!EventTagOverride.MatchesTagExact(GPTags::None))
	{
		EventTag = EventTagOverride;
	}
	else
	{
		UGP_AttributeSet* AttributeSet = Cast<UGP_AttributeSet>(PlayerCharacter->GetAttributeSet());
		if (!ensureMsgf(AttributeSet,
		                TEXT("SendDamageEventToPlayer: AttributeSet is null on %s"),
		                *PlayerCharacter->GetName()))
		{
			return;
		}

		const bool bLethal = AttributeSet->GetHealth() - Damage <= 0.0f;
		EventTag = bLethal
			           ? GPTags::Events::Player::Death
			           : GPTags::Events::Player::HitReact;
	}

	// Attach optional VFX reference to the payload so the receiving ability
	// (HitReact or Death) can spawn impact-specific particle effects.
	Payload.OptionalObject = OptionalParticleSystem;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(PlayerCharacter,
	                                                         EventTag,
	                                                         Payload);

	// -------------------------------------------------------------------------
	// APPLY DAMAGE GE
	// -------------------------------------------------------------------------
	UAbilitySystemComponent* TargetASC = PlayerCharacter->GetAbilitySystemComponent();
	if (!ensureMsgf(TargetASC,
	                TEXT("SendDamageEventToPlayer: ASC is null on %s"),
	                *PlayerCharacter->GetName()))
	{
		return;
	}

	FGameplayEffectContextHandle ContextHandle = TargetASC->MakeEffectContext();

	FGameplayEffectSpecHandle SpecHandle = TargetASC->MakeOutgoingSpec(DamageEffect,
	                                                                   1.0f,
	                                                                   ContextHandle);

	// Negate the Damage value because the caller passes a positive number
	// (e.g., 5.0 = "deal 5 damage") but the GE modifier uses Add on Health,
	// which requires a negative value to reduce it.
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle,
	                                                              DataTag,
	                                                              -Damage);

	TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}

void UGP_BlueprintLibrary::SendDamageEventToPlayers(TArray<AActor*> Targets,
                                                    const TSubclassOf<UGameplayEffect>& DamageEffect,
                                                    FGameplayEventData& Payload,
                                                    const FGameplayTag& DataTag,
                                                    float Damage,
                                                    const FGameplayTag& EventTagOverride,
                                                    UObject* OptionalParticleSystem)
{
	// Delegate to the single-target version for each actor.
	// Each call independently validates the target, checks lethality,
	// and applies the GE, so one invalid target doesn't affect the others.
	for (AActor* Target : Targets)
	{
		SendDamageEventToPlayer(Target,
		                        DamageEffect,
		                        Payload,
		                        DataTag,
		                        Damage,
		                        EventTagOverride,
		                        OptionalParticleSystem);
	}
}

TArray<AActor*> UGP_BlueprintLibrary::HitBoxOverlapTest(AActor* AvatarActor,
                                                        float HitBoxRadius,
                                                        float HitBoxForwardOffset,
                                                        float HitBoxElevationOffset,
                                                        bool bDrawDebugs)
{
	if (!IsValid(AvatarActor))
	{
		return TArray<AActor*>();
	}

	// -------------------------------------------------------------------------
	// QUERY SETUP
	// -------------------------------------------------------------------------
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(AvatarActor);

	// Filter: ignore all channels except Pawn to avoid detecting walls/floors.
	FCollisionResponseParams ResponseParams;
	ResponseParams.CollisionResponse.SetAllChannels(ECR_Ignore);
	ResponseParams.CollisionResponse.SetResponse(ECC_Pawn,
	                                             ECR_Block);

	// -------------------------------------------------------------------------
	// SPATIAL CALCULATION
	// -------------------------------------------------------------------------
	const FVector Forward = AvatarActor->GetActorForwardVector() * HitBoxForwardOffset;
	const FVector HitBoxLocation = AvatarActor->GetActorLocation() + Forward + FVector(0.0f,
		0.0f,
		HitBoxElevationOffset);

	FCollisionShape Sphere = FCollisionShape::MakeSphere(HitBoxRadius);

	// -------------------------------------------------------------------------
	// EXECUTE OVERLAP
	// -------------------------------------------------------------------------
	UWorld* World = GEngine->GetWorldFromContextObject(AvatarActor,
	                                                   EGetWorldErrorMode::LogAndReturnNull);
	if (!IsValid(World))
	{
		return TArray<AActor*>();
	}

	TArray<FOverlapResult> OverlapResults;
	World->OverlapMultiByChannel(OverlapResults,
	                             HitBoxLocation,
	                             FQuat::Identity,
	                             ECC_Visibility,
	                             Sphere,
	                             QueryParams,
	                             ResponseParams);

	// -------------------------------------------------------------------------
	// FILTER & DEDUPLICATE
	// -------------------------------------------------------------------------
	// Only include alive BaseCharacters. AddUnique handles the case where
	// one actor has multiple collision components registering separate overlaps.
	TArray<AActor*> ActorsHit;
	for (const FOverlapResult& Result : OverlapResults)
	{
		AGP_BaseCharacter* BaseCharacter = Cast<AGP_BaseCharacter>(Result.GetActor());
		if (!IsValid(BaseCharacter) || !BaseCharacter->IsAlive())
		{
			continue;
		}

		ActorsHit.AddUnique(BaseCharacter);
	}

	// -------------------------------------------------------------------------
	// DEBUG VISUALIZATION
	// -------------------------------------------------------------------------
	if (bDrawDebugs)
	{
		DrawHitBoxOverlapDebugs(AvatarActor,
		                        OverlapResults,
		                        HitBoxLocation,
		                        HitBoxRadius);
	}

	return ActorsHit;
}

void UGP_BlueprintLibrary::DrawHitBoxOverlapDebugs(const UObject* WorldContextObject,
                                                   const TArray<FOverlapResult>& OverlapResults,
                                                   const FVector& HitBoxLocation,
                                                   float HitBoxRadius)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject,
	                                                   EGetWorldErrorMode::LogAndReturnNull);
	if (!IsValid(World))
	{
		return;
	}

	// Red sphere: the detection volume.
	DrawDebugSphere(World,
	                HitBoxLocation,
	                HitBoxRadius,
	                16,
	                FColor::Red,
	                false,
	                3.0f);

	// Green spheres: each actor detected within the volume.
	for (const FOverlapResult& Result : OverlapResults)
	{
		if (IsValid(Result.GetActor()))
		{
			FVector DebugLocation = Result.GetActor()->GetActorLocation();
			DebugLocation.Z += 100.0f;
			DrawDebugSphere(World,
			                DebugLocation,
			                30.0f,
			                10,
			                FColor::Green,
			                false,
			                3.0f);
		}
	}
}

TArray<AActor*> UGP_BlueprintLibrary::ApplyKnockback(AActor* AvatarActor,
                                                     const TArray<AActor*>& HitActors,
                                                     float InnerRadius,
                                                     float OuterRadius,
                                                     float LaunchForceMagnitude,
                                                     float RotationAngle,
                                                     bool bDrawDebugs)
{
	for (AActor* HitActor : HitActors)
	{
		ACharacter* HitCharacter = Cast<ACharacter>(HitActor);
		if (!ensureMsgf(HitCharacter,
		                TEXT("ApplyKnockback: HitActor %s is not a Character. Skipping."),
		                *GetNameSafe(HitActor)))
		{
			continue;
		}

		const FVector HitCharacterLocation = HitCharacter->GetActorLocation();
		const FVector AvatarLocation = AvatarActor->GetActorLocation();

		const FVector ToHitActor = HitCharacterLocation - AvatarLocation;
		const float Distance = FVector::Dist(AvatarLocation,
		                                     HitCharacterLocation);

		// -------------------------------------------------------------------------
		// DISTANCE-BASED FORCE CALCULATION
		// -------------------------------------------------------------------------
		// Three zones:
		//   1. Beyond OuterRadius → skip (no knockback)
		//   2. Within InnerRadius → full force
		//   3. Between Inner and Outer → linear falloff from full to zero
		float LaunchForce = 0.0f;
		if (Distance > OuterRadius)
		{
			continue;
		}

		if (Distance <= InnerRadius)
		{
			LaunchForce = LaunchForceMagnitude;
		}
		else
		{
			// Linear interpolation: full force at InnerRadius, zero at OuterRadius.
			const FVector2D FalloffRange(InnerRadius,
			                             OuterRadius);
			const FVector2D LaunchForceRange(LaunchForceMagnitude,
			                                 0.0f);
			LaunchForce = FMath::GetMappedRangeValueClamped(FalloffRange,
			                                                LaunchForceRange,
			                                                Distance);
		}

		if (bDrawDebugs)
		{
			GEngine->AddOnScreenDebugMessage(-1,
			                                 3.0f,
			                                 FColor::Red,
			                                 FString::Printf(TEXT("LaunchForce: %f"),
			                                                 LaunchForce));
		}

		// -------------------------------------------------------------------------
		// DIRECTION CALCULATION WITH UPWARD ARC
		// -------------------------------------------------------------------------
		// Start with flat direction from attacker to target (Z zeroed).
		// Then rotate upward around the perpendicular axis by RotationAngle
		// to create an arcing knockback trajectory instead of a flat push.
		FVector KnockbackForce = ToHitActor.GetSafeNormal();
		KnockbackForce.Z = 0.0f;

		// Find the perpendicular axis (right vector relative to knockback direction).
		const FVector Right = KnockbackForce.RotateAngleAxis(90.0f,
		                                                     FVector::UpVector);

		// Rotate the knockback direction upward around the right axis.
		// Negative angle because rotating "up" around the right axis requires negative pitch.
		KnockbackForce = KnockbackForce.RotateAngleAxis(-RotationAngle,
		                                                Right) * LaunchForce;

		if (bDrawDebugs)
		{
			UWorld* World = GEngine->GetWorldFromContextObject(AvatarActor,
			                                                   EGetWorldErrorMode::LogAndReturnNull);
			DrawDebugDirectionalArrow(World,
			                          HitCharacterLocation,
			                          HitCharacterLocation + KnockbackForce,
			                          100.0f,
			                          FColor::Green,
			                          false,
			                          3.0f);
		}

		// If the hit target is an AI enemy, disable navigation during airtime.
		// StopMovementUntilLanded handles the full cycle: stop movement → wait for
		// landing → re-enable movement and signal the BT via EndAttack event.
		AGP_EnemyCharacter* EnemyCharacter = Cast<AGP_EnemyCharacter>(HitCharacter);
		if (IsValid(EnemyCharacter))
		{
			EnemyCharacter->StopMovementUntilLanded();
		}

		// Apply the force via LaunchCharacter.
		// Both bXYOverride and bZOverride are true, meaning the launch force
		// completely replaces current velocity instead of adding to it.
		HitCharacter->LaunchCharacter(KnockbackForce,
		                              true,
		                              true);
	}

	// Pass-through: return the same array for Blueprint node chaining.
	return HitActors;
}
