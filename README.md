# GAS Prototype — Unreal Engine 5.6 C++

A focused C++ prototype built to study and implement the **Gameplay Ability System (GAS)** in Unreal Engine 5.6. The gameplay scenario is intentionally minimal — survive against waves of melee and ranged enemies spawning in increasing frequency — so the architecture remains the subject under study, not the game design.

Built following the [UE5 GAS Crash Course](https://www.udemy.com/course/ue5-gas-crash-course/) as a structural foundation, with my own C++ implementation throughout.

📺 [Gameplay Demo on YouTube](https://youtu.be/6nPvOgx6aTw)  
📝 [Full technical breakdown on my blog](https://nbertoa.com/2026/04/01/unreal-5-6-c-gas-prototype/)

---

## Tech Stack

| | |
|---|---|
| **Engine** | Unreal Engine 5.6 |
| **Language** | C++ with Blueprint subclassing for asset configuration |
| **Plugins** | Gameplay Ability System, Enhanced Input System |
| **Editor Plugins** | [Electronic Nodes](https://www.unrealengine.com/marketplace/en-US/product/electronic-nodes), [Blueprint Assist](https://www.unrealengine.com/marketplace/en-US/product/blueprint-assist) |

---

## Architecture Overview

### Character Hierarchy

All characters share an abstract base class that provides the common GAS initialization pipeline.
The two concrete subclasses implement a deliberate split in where the ASC lives:

```
AGP_BaseCharacter (Abstract — IAbilitySystemInterface)
├── AGP_PlayerCharacter   → ASC lives on AGP_PlayerState (persists across respawns)
└── AGP_EnemyCharacter    → ASC lives on the Pawn itself (destroyed with the actor)
```

**Why the split?** The player's ASC lives on `AGP_PlayerState` so that attributes, active effects, and cooldowns survive the Character's death and destruction cycle. Enemies are transient actors — their ASC on the Pawn is the correct choice, and their replication mode is set to `Minimal` (Tags + Cues only, no full GE data for clients).

### GAS Initialization Flow

```
SERVER                                  CLIENT
──────────────────────────────────────────────────────
PossessedBy()                           OnRep_PlayerState()
  │                                       │
  ├── InitAbilityActorInfo()              ├── InitAbilityActorInfo()
  ├── InitializeAttributes()             └── BroadcastInitialValues() → UI binds
  ├── GiveStartupAbilities()
  └── BroadcastInitialValues() → UI binds
```

The client never grants abilities or initializes attributes — it only links actor info for prediction and notifies the UI that pointers are ready.

---

## Key Systems

### Custom Ability System Component (`UGP_AbilitySystemComponent`)

Extends the engine ASC with two additions:

**Auto-activation of passive abilities.** Abilities tagged with `GPTags::Abilities::ActivateOnGiven` activate automatically when granted. The implementation handles a Listen Server edge case: `OnGiveAbility` fires on the authority side and `OnRep_ActivateAbilities` fires on the client side. Without a guard, the host would activate passives twice. An `IsOwnerActorAuthoritative()` check restricts each hook to its correct network side. A `TSet<FGameplayAbilitySpecHandle>` tracks already-activated handles at O(1) to prevent redundant activations during replication bursts.

**Runtime ability leveling.** `SetAbilityLevel` and `AddToAbilityLevel` are `BlueprintCallable`, server-only utilities that modify ability specs and call `MarkAbilitySpecDirty` to force replication.

### Attribute Set (`UGP_AttributeSet`)

Manages `Health`, `MaxHealth`, `Mana`, and `MaxMana`, all replicated with `REPNOTIFY_Always` for GAS prediction rollback support.

Clamping is implemented in **two places** for a specific reason:

- `PreAttributeChange` — clamps `CurrentValue` before it is applied. Keeps UI and gameplay queries within valid range but does **not** clamp `BaseValue`.
- `PostGameplayEffectExecute` — clamps `BaseValue`. A periodic GE using `Add` accumulates on `BaseValue` and can push it beyond `MaxHealth`. When the GE expires, `CurrentValue` snaps back to the unclamped `BaseValue`, causing a health spike. Both hooks are necessary; neither alone is sufficient.

A replicated `bAttributesInitialized` flag is set on the first GE execution and broadcast via `OnAttributesInitialized`. UI components listen for this to know when it is safe to bind to attribute delegates, solving the initialization race condition.

Kill scoring is handled inside `PostGameplayEffectExecute`: when Health reaches zero, `GPTags::Events::KillScored` is sent to the damage instigator with the victim as the event payload's Instigator.

### Native Gameplay Tags (`GPTags`)

All tags are declared as Native Gameplay Tags in a centralized `GPTags.h` using C++ namespaces that mirror the tag hierarchy:

```cpp
GPTags::Abilities::Player::Primary
GPTags::Events::Enemy::HitReact
GPTags::SetByCaller::Projectile
GPTags::Status::Dead
GPTags::Cooldown::Secondary
```

This eliminates unsafe string literals, makes tag references type-safe, and keeps the full tag vocabulary visible in one file. `GPTags::None` serves as a sentinel "no override" value for optional tag parameters, avoiding function overloads or nullable pointers.

### Input System (`AGP_PlayerController`)

Uses the Enhanced Input System exclusively. All ability inputs share a single generic handler using the payload pattern:

```cpp
// One handler for all abilities — tag identifies which one to activate
EnhancedInputComponent->BindAction(PrimaryAction, ETriggerEvent::Triggered,
    this, &ThisClass::Input_AbilityPressed,
    static_cast<FGameplayTag>(GPTags::Abilities::Player::Primary));
```

The ASC is retrieved from the PlayerState directly, bypassing the Pawn — safer during death/respawn when the Pawn may be null. Adding a new ability slot requires one new property, one new tag, and one `BindAction` line.

### Combat Utilities (`UGP_BlueprintLibrary`)

A static Blueprint Function Library centralizing shared combat logic:

- **`HitBoxOverlapTest`** — sphere overlap against the Pawn collision channel with deduplication and dead-character filtering. Configurable radius, forward offset, and elevation offset allow per-ability tuning without subclassing.
- **`SendDamageEventToPlayer`** — centralized damage application: GE spec creation, `SetByCallerMagnitude` assignment, context setup, lethality detection, and event routing. All damage sources (melee, projectile, area) use this function for consistency.
- **`ApplyKnockback`** — linear force falloff from `InnerRadius` (full force) to `OuterRadius` (zero), with upward rotation for arc effect. Integrates with enemy AI via `StopMovementUntilLanded`, which registers a one-shot `LandedDelegate` that re-enables navigation and sends `GPTags::Events::Enemy::EndAttack` to resume the Behavior Tree.

### Reactive UI

The UI layer is entirely event-driven — no polling, no Tick.

`UGP_WidgetComponent` discovers `UGP_AttributeWidget` instances in the widget tree, matches them to `(Current, Max)` attribute pairs configured in Blueprint via a `TMap`, and binds them to ASC delegates. All bindings use `TWeakObjectPtr` to avoid preventing GC of destroyed widgets. `EndPlay` explicitly unregisters all delegates to prevent dangling callbacks when an enemy is destroyed but the player's ASC (on PlayerState) survives.

`UGP_AttributeWidget` follows a strict push model: it holds no GAS references and fetches no data itself. The component pushes `(NewValue, MaxValue, OldValue)` on every change; Blueprint implements `BP_OnAttributeChange` to update progress bars or spawn floating damage numbers using the delta.

For Blueprint HUD widgets, `UGP_AttributeChangeTask` is an async `UBlueprintAsyncActionBase` node that wraps `GetGameplayAttributeValueChangeDelegate` and re-broadcasts through a Blueprint-friendly multicast delegate, unpacking `FOnAttributeChangeData` into individual floats.

### Gameplay Abilities

Base class `UGP_GameplayAbility` sets project-wide defaults:

| Policy | Value | Reason |
|---|---|---|
| Instancing | `InstancedPerActor` | Each character needs its own instance for safe state storage |
| Net Execution | `LocalPredicted` | Client runs immediately, server validates — best input feel |
| Replication | `ReplicateNo` | State synced via Tags/Effects, not by replicating the UObject |

Enemy abilities override `NetExecutionPolicy` to `ServerInitiated` — AI does not exist on clients, prediction is not applicable.

**`UGP_HitReact`** is triggered via Gameplay Event (not activated directly). It reads the `Instigator` from the event payload and resolves which montage section to play (`Front`, `Back`, `Left`, `Right`) using dot product math via `UGP_BlueprintLibrary::GetHitDirection`. `GPTags::Abilities::BlockHitReact` is used as an Activation Blocked Tag to prevent flinch interruptions during attack animations.

**`UGP_Primary`** uses `HitBoxOverlapTest` for hit detection triggered via AnimNotify, then dispatches `GPTags::Events::Enemy::HitReact` to each hit actor with the attacker as payload Instigator, allowing independent per-target direction calculation.

### Projectile System (`AGP_Projectile`)

Straight-line movement via `UProjectileMovementComponent` (zero gravity by default). The `Damage` property is `ExposeOnSpawn`, allowing per-instance damage values set at spawn time by the spawning ability. Damage is applied server-only in `NotifyActorBeginOverlap` via `SendDamageEventToPlayer` with `GPTags::SetByCaller::Projectile` as the magnitude key. A 10-second `InitialLifeSpan` acts as a safety net against orphaned projectiles.

---

## Project Structure

```
GASPrototype/
├── Config/
│   ├── DefaultGame.ini          # GAS Cue notify paths
│   ├── DefaultGameplayTags.ini  # Registered GameplayCue tags
│   └── DefaultInput.ini         # Enhanced Input configuration
├── Source/
│   └── GASPrototype/
│       ├── AbilitySystem/
│       │   ├── GP_AbilitySystemComponent  # Custom ASC (passive auto-activation, leveling)
│       │   ├── GP_AttributeSet            # Health/Mana with dual clamping
│       │   └── Abilities/
│       │       ├── GP_GameplayAbility     # Base ability class (instancing/net policies)
│       │       ├── Player/
│       │       │   └── GP_Primary         # Melee hit detection + HitReact event dispatch
│       │       └── Enemy/
│       │           └── GP_HitReact        # Direction-aware flinch ability
│       ├── Characters/
│       │   ├── GP_BaseCharacter           # Abstract base (GAS pipeline, death handling)
│       │   ├── GP_PlayerCharacter         # ASC on PlayerState pattern
│       │   └── GP_EnemyCharacter          # ASC on Pawn pattern, AI death/knockback
│       ├── Player/
│       │   ├── GP_PlayerController        # Enhanced Input + payload pattern
│       │   └── GP_PlayerState             # ASC + AttributeSet owner for player
│       ├── GameObjects/
│       │   └── GP_Projectile              # GAS damage via SetByCaller, ExposeOnSpawn
│       ├── UI/
│       │   ├── GP_WidgetComponent         # Event-driven GAS→UMG bridge
│       │   ├── GP_AttributeWidget         # Push-model attribute display widget
│       │   └── GP_AttributeChangeTask     # Async Blueprint attribute listener
│       ├── GameplayTags/
│       │   └── GPTags                     # Centralized Native Gameplay Tags
│       └── Utils/
│           └── GP_BlueprintLibrary        # HitBox, knockback, damage utilities
└── GASPrototype.uproject
```

---

## About

**Nicolás Bertoa** — Senior R&D Prototyping Engineer with 15+ years of professional experience, including R&D work for DreamWorks Animation and Sony Interactive Entertainment. Focused on Unreal Engine, C++, real-time systems, and technical prototyping.

🌐 [Portfolio](https://nbertoa.com/) | 🎬 [Demo Reels](https://nbertoa.com/demo-reels/)
