# Decoupled State Machine System Plan for FloppyTurd (Extended from EnemySystem)

## Overview
We'll create a modular, decoupled Finite State Machine (FSM) system by extracting and generalizing the state handling logic from EnemySystem. This new system will be re-usable for complex entities like the Rat King boss, while leaving the existing EnemySystem unchanged for simpler minions/enemies. The goal is modularity without disrupting working code—EnemySystem can stay enum-based, and we add a new FSM component/system for bosses/others.

Benefits:
- Decoupled: FSM lives in its own classes/components, attachable via ECS.
- Re-usable: Can be used for RatKing, future bosses, or even player states.
- Builds on existing: Reuse patterns like state enums, ChangeState, and animation tying from EnemySystem.

## Core Components (Decoupled Design)
1. **State Enum**: Reuse/extend EnemyState as a general `EntityState` enum (e.g., add boss-specific like Aiming, Throwing). Place in GameComponents.h for sharing.

2. **StateMachine Component**: New ECS component:
   - Holds currentState (EntityState), stateTimer, previousState.
   - Map of allowed transitions (e.g., std::map<EntityState, std::vector<EntityState>>).
   - Virtual methods for customization: OnEnterState, OnUpdateState, OnExitState.

3. **StateMachineSystem**: New system (StateMachineSystem.h/cpp):
   - Updates entities with StateMachine component.
   - Calls ProcessState(deltaTime, entity) similar to EnemySystem's ProcessEnemyState.
   - Handles common logic: timers, transitions, animation switching (integrate with StateAnimation).
   - Event handling: Method to SendEvent (e.g., DamageTaken triggers Hurt).

4. **Extraction from EnemySystem**:
   - Copy/generalize: Take ChangeEnemyState, state switching, and animation logic as templates.
   - Don't modify EnemySystem: It keeps its internal handling; new FSM is opt-in for other systems.

## Rat King Implementation
- Add StateMachine component to bossEntity in BossSystem::InitializeForLevel.
- Define boss-specific states in EntityState (e.g., BossIdle, BossAiming).
- In BossSystem::Update, call StateMachineSystem::Update(entity) or handle via delegates.
- Customize: Override OnUpdateState for boss behaviors (e.g., in Aiming: update arm rotations, check timer for Throwing transition).
- Hurt Interrupt: On Damage event, ChangeState(Hurt), store previousState, return after timer.

## Steps to Implement
1. Create EntityState enum in GameComponents.h (extend from EnemyState patterns).
2. Add StateMachine component to GameComponents.h.
3. Implement StateMachineSystem.cpp/h, porting logic from EnemySystem's ProcessEnemyState/Animation.
4. Integrate into BossSystem: Attach component, route updates/events.
5. Test: Verify RatKing behaviors match current, add robustness (e.g., invalid transition logs).
6. Document: How to use for future systems (e.g., add to player or new bosses).

This keeps EnemySystem intact for minions while giving us a powerful, decoupled FSM for the Rat King.
