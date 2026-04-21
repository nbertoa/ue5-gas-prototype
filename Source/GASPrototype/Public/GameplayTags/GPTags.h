#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

/**
 * @file GPTags.h
 * @brief Centralized declaration of Native Gameplay Tags.
 *
 * Usage of Native Gameplay Tags allows us to reference tags in C++ 
 * without relying on unsafe string literals. These tags are registered 
 * during the exact static initialization timing required by the engine.
 */

namespace GPTags
{
	/**
	 * @brief Sentinel tag used as a "no override" value for optional tag parameters.
	 * When passed to functions like SendDamageEventToPlayer as EventTagOverride,
	 * signals that the function should use its default behavior (lethality check)
	 * instead of a caller-specified event tag.
	 * Actual Tag: "GPTags.None"
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(None);

	/**
	 * @brief Root-level tag identifying the Player character.
	 * Can be used for tag-based filtering in queries, collision, or AI perception.
	 * Actual Tag: "GPTags.Player"
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Player);

	/**
	 * @namespace SetByCaller
	 * @brief Tags used as keys for SetByCallerMagnitude in Gameplay Effect Specs.
	 * These tags allow GEs to have dynamic magnitudes that are set at runtime
	 * by the code that applies the effect, rather than being hardcoded in the GE asset.
	 */
	namespace SetByCaller
	{
		/**
		 * @brief Key for projectile damage magnitude.
		 * The spawning code assigns the projectile's Damage value to this tag
		 * via AssignTagSetByCallerMagnitude before applying the damage GE.
		 * Actual Tag: "GPTags.SetByCaller.Projectile"
		 */
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Projectile);

		/**
		 * @brief Key for enemy melee damage magnitude.
		 * Used by enemy melee abilities to inject damage values into the GE Spec
		 * via AssignTagSetByCallerMagnitude before applying to the player.
		 * Actual Tag: "GPTags.SetByCaller.Melee"
		 */
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Melee);

		/**
		 * @namespace Player
		 * @brief SetByCaller keys specific to player abilities.
		 */
		namespace Player
		{
			/**
			 * @brief Key for player secondary ability damage magnitude.
			 * Actual Tag: "GPTags.SetByCaller.Player.Secondary"
			 */
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Secondary);
		}
	}

	/**
	 * @namespace Abilities
	 * @brief General Ability tags (Identity, Slots, Meta-tags).
	 */
	namespace Abilities
	{
		/**
		 * @brief Meta-tag for Passive Abilities.
		 * Abilities with this tag auto-activate when granted to the ASC.
		 * Usage: GPTags::Abilities::ActivateOnGiven
		 */
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(ActivateOnGiven);

		/**
		 * @brief Identity tag for the Death ability.
		 * Used to identify and trigger the death handling ability
		 * when a character's Health reaches zero.
		 * Actual Tag: "GPTags.Abilities.Death"
		 */
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Death);

		/**
		 * @brief Tag used to block HitReact abilities from activating.
		 * Applied as an Activation Blocked Tag on HitReact abilities so that
		 * characters performing certain actions (e.g., attacking, casting)
		 * are not interrupted by flinch animations.
		 * Actual Tag: "GPTags.Abilities.BlockHitReact"
		 */
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(BlockHitReact);

		/**
		 * @namespace Player
		 * @brief Specific input slots or ability identities for the Player Character.
		 */
		namespace Player
		{
			/**
			 * @brief Tag identifying the Primary Ability Slot.
			 * Actual Tag: "GPTags.Abilities.Player.Primary"
			 */
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Primary);

			/**
			 * @brief Tag identifying the Secondary Ability Slot.
			 * Actual Tag: "GPTags.Abilities.Player.Secondary"
			 */
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Secondary);

			/**
			 * @brief Tag identifying the Tertiary Ability Slot.
			 * Actual Tag: "GPTags.Abilities.Player.Tertiary"
			 */
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tertiary);
		}

		/**
		 * @namespace Enemy
		 * @brief Specific ability identities for AI/Enemies.
		 */
		namespace Enemy
		{
			/**
			 * @brief Tag identifying the Hit Reaction ability (Stun/Flinch).
			 * Actual Tag: "GPTags.Abilities.Enemy.HitReact"
			 */
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(HitReact);

			/**
			 * @brief Tag identifying the Enemy Attack ability.
			 * Used by the AI Behavior Tree to trigger attacks via TryActivateAbilitiesByTag.
			 * Actual Tag: "GPTags.Abilities.Enemy.Attack"
			 */
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attack);
		}
	}

	/**
	 * @namespace Events
	 * @brief Tags used for Gameplay Event Triggers (Payloads).
	 */
	namespace Events
	{
		/**
		 * @brief Event sent to the damage instigator when their target's Health reaches zero.
		 * The payload's Instigator field contains the actor that died (the victim),
		 * allowing the killer to react (score tracking, kill streaks, UI feedback).
		 * Actual Tag: "GPTags.Events.KillScored"
		 */
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(KillScored);

		namespace Player
		{
			/**
			 * @brief Event trigger for the Primary action moment.
			 * Actual Tag: "GPTags.Events.Player.Primary"
			 */
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Primary);

			/**
			 * @brief Event trigger for the Secondary action moment.
			 * Actual Tag: "GPTags.Events.Player.Secondary"
			 */
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Secondary);

			/**
			 * @brief Event trigger for the Tertiary action moment.
			 * Actual Tag: "GPTags.Events.Player.Tertiary"
			 */
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tertiary);

			/**
			 * @brief Event trigger for when the player takes damage.
			 * Used to activate the player's hit reaction ability (flinch, screen shake, etc.).
			 * Actual Tag: "GPTags.Events.Player.HitReact"
			 */
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(HitReact);

			/**
			 * @brief Event trigger for when the player's Health reaches zero.
			 * Used to activate the player's death ability (death animation, disable input, etc.).
			 * Actual Tag: "GPTags.Events.Player.Death"
			 */
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Death);
		}

		namespace Enemy
		{
			/**
			 * @brief Event trigger for when an enemy takes damage.
			 * Actual Tag: "GPTags.Events.Enemy.HitReact"
			 */
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(HitReact);

			/**
			 * @brief Event trigger signaling the end of an enemy attack sequence.
			 * Fired from an AnimNotify at the end of the attack montage,
			 * allowing the Behavior Tree to resume its decision loop.
			 * Actual Tag: "GPTags.Events.Enemy.EndAttack"
			 */
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(EndAttack);

			/**
			 * @brief Event triggered when an enemy melee trace detects a hit.
			 * Fired from the melee detection system (e.g., weapon trace or sweep)
			 * to signal that a valid target was struck during a melee attack.
			 * Actual Tag: "GPTags.Events.Enemy.MeleeTraceHit"
			 */
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(MeleeTraceHit);
		}
	}

	/**
	 * @namespace Status
	 * @brief Tags representing persistent character states.
	 * These tags are applied to the ASC and can be queried by any system
	 * (Abilities, AI, UI) to check current character status.
	 * They also integrate with GAS blocking/cancellation tag policies.
	 */
	namespace Status
	{
		/**
		 * @brief Applied to the ASC when a character's Health reaches zero.
		 * Can be used as an Activation Blocked Tag on abilities to prevent
		 * dead characters from activating new abilities.
		 * Actual Tag: "GPTags.Status.Dead"
		 */
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Dead);
	}

	/**
	 * @namespace Cooldown
	 * @brief Tags applied by Cooldown Gameplay Effects.
	 * GAS uses these tags to track active cooldowns on the ASC.
	 * When a cooldown GE is active, its tag is present on the ASC,
	 * and abilities can check for it to prevent re-activation.
	 */
	namespace Cooldown
	{
		/**
		 * @brief Cooldown tag for the player's Secondary ability.
		 * Applied by the Secondary ability's cooldown GE while active.
		 * Actual Tag: "GPTags.Cooldown.Secondary"
		 */
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Secondary);
	}
}
