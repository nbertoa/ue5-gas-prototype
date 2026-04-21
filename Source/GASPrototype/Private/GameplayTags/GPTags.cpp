#include "GameplayTags/GPTags.h"

// -----------------------------------------------------------------------------
// ROOT
// -----------------------------------------------------------------------------
namespace GPTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(None,
	                               "GPTags.None",
	                               "Sentinel tag for optional parameters. Signals 'no override' / 'use default behavior'.")
	;

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Player,
	                               "GPTags.Player",
	                               "Root-level identity tag for the Player character. Used for tag-based filtering.");
}

// -----------------------------------------------------------------------------
// SET BY CALLER
// -----------------------------------------------------------------------------
namespace GPTags::SetByCaller
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Projectile,
	                               "GPTags.SetByCaller.Projectile",
	                               "SetByCaller key for projectile damage. Assigned at spawn time from the Damage property.")
	;

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Melee,
	                               "GPTags.SetByCaller.Melee",
	                               "SetByCaller key for enemy melee damage. Assigned by melee abilities before applying the damage GE.")
	;

	namespace GPTags::SetByCaller::Player
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Secondary,
		                               "GPTags.SetByCaller.Player.Secondary",
		                               "SetByCaller key for player secondary ability damage magnitude.");
	}
}

// -----------------------------------------------------------------------------
// ABILITIES :: UTILITY
// -----------------------------------------------------------------------------
namespace GPTags::Abilities
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(ActivateOnGiven,
	                               "GPTags.Abilities.ActivateOnGiven",
	                               "Meta-Tag: Abilities with this tag should try to activate immediately when granted (Passive Abilities).")
	;

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Death,
	                               "GPTags.Abilities.Death",
	                               "Identity tag for the Death ability. Triggered when Health reaches zero.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(BlockHitReact,
	                               "GPTags.Abilities.BlockHitReact",
	                               "Applied to abilities that should suppress HitReact activation (e.g., during attack windups or heavy abilities).")
	;
}

// -----------------------------------------------------------------------------
// ABILITIES :: PLAYER
// -----------------------------------------------------------------------------
namespace GPTags::Abilities::Player
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Primary,
	                               "GPTags.Abilities.Player.Primary",
	                               "Input slot for the Player's Primary Ability (e.g., Weapon Fire).");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Secondary,
	                               "GPTags.Abilities.Player.Secondary",
	                               "Input slot for the Player's Secondary Ability (e.g., Aim / Alt-Fire).");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Tertiary,
	                               "GPTags.Abilities.Player.Tertiary",
	                               "Input slot for the Player's Tertiary Ability (e.g., Ultimate).");
}

// -----------------------------------------------------------------------------
// ABILITIES :: ENEMY
// -----------------------------------------------------------------------------
namespace GPTags::Abilities::Enemy
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(HitReact,
	                               "GPTags.Abilities.Enemy.HitReact",
	                               "Tag for the Enemy HitReact Ability identity.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attack,
	                               "GPTags.Abilities.Enemy.Attack",
	                               "Identity tag for the Enemy Attack ability. Activated by the AI Behavior Tree.");
}

// -----------------------------------------------------------------------------
// EVENTS :: GENERAL
// -----------------------------------------------------------------------------
namespace GPTags::Events
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(KillScored,
	                               "GPTags.Events.KillScored",
	                               "Sent to the damage instigator when their target dies. Payload Instigator = the victim.")
	;
}

// -----------------------------------------------------------------------------
// EVENTS :: PLAYER
// -----------------------------------------------------------------------------
namespace GPTags::Events::Player
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Primary,
	                               "GPTags.Events.Player.Primary",
	                               "Event payload tag. Used to signal specific timing moments (AnimNotify) for the Primary Ability.")
	;

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Secondary,
	                               "GPTags.Events.Player.Secondary",
	                               "Event payload tag. Used to signal specific timing moments (AnimNotify) for the Secondary Ability.")
	;

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Tertiary,
	                               "GPTags.Events.Player.Tertiary",
	                               "Event payload tag. Used to signal specific timing moments (AnimNotify) for the Tertiary Ability.")
	;

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(HitReact,
	                               "GPTags.Events.Player.HitReact",
	                               "Event trigger for player hit reactions. Fired when the player takes damage.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Death,
	                               "GPTags.Events.Player.Death",
	                               "Event trigger for player death. Fired when the player's Health reaches zero.");
}

// -----------------------------------------------------------------------------
// EVENTS :: ENEMY
// -----------------------------------------------------------------------------
namespace GPTags::Events::Enemy
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(HitReact,
	                               "GPTags.Events.Enemy.HitReact",
	                               "Tag for the Enemy HitReact Event. Trigger this when taking damage.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(EndAttack,
	                               "GPTags.Events.Enemy.EndAttack",
	                               "Fired from AnimNotify when the enemy attack montage finishes. Signals the BT to resume.")
	;

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(MeleeTraceHit,
	                               "GPTags.Events.Enemy.MeleeTraceHit",
	                               "Event fired when an enemy melee trace detects a valid hit target.");
}

// -----------------------------------------------------------------------------
// STATUS
// -----------------------------------------------------------------------------
namespace GPTags::Status
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Dead,
	                               "GPTags.Status.Dead",
	                               "Persistent state tag. Applied to the ASC when a character dies. Used to block ability activation on dead characters.")
	;
}

// -----------------------------------------------------------------------------
// COOLDOWN
// -----------------------------------------------------------------------------
namespace GPTags::Cooldown
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Secondary,
	                               "GPTags.Cooldown.Secondary",
	                               "Active cooldown tag for the player's Secondary ability. Applied by its cooldown GE.")
	;
}
