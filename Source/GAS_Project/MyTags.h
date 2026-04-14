// Vince Petrelli All Rights Reserved

#pragma once

#include "NativeGameplayTags.h"

namespace MyTags
{
	extern const TMap<uint8, FGameplayTag> MovementModeTagMap;
	extern const TMap<uint8, FGameplayTag> CustomMovementModeTagMap;
	
	namespace InitState
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Spawned)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(DataAvailable)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(DataInitialized)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayReady)
	}

	
	namespace Lyra
	{
		// InputTag
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Move)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Look_Mouse)

		UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Weapon_Fire)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_BowAim)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_BowFire)

		// UI
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_Game)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_GameMenu)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_Menu)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_Modal)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_LAYER_MENU)
		
		
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Platform_Trait_Input_PrimarlyController)
		
		//Ability
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Interaction_Activate);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Interaction_Duraction_Message);
		
		
	
		
		// QUickBar
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Lyra_QickBar_Message_SlotsChanged)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Lyra_QickBar_Message_ActiveIndexChanged)

		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Lyra_Elimination_Message)

		
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Lyra_Enemy_Found)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Lyra_Enemy_Lost)

		//Status
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Death_Dying)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Death_Dead)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Crouching)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Death_Dead)

		
	}

	namespace Souls
	{
		// Gameplay
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gameplay_Damage)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gameplay_DamageImmunity)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gameplay_DamageSelfDestruct)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gameplay_FellOutOfWorld)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gameplay_Damage_Message)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gameplay_MovementStopped)

		//Aility
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Type_Action_BowAim)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Type_Action_BowFire)

		//Cooldown
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_BowFire)

		//GameplayEvent
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEvent_Death)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEvent_Bow_SpawnArrow)
		

		//SetByCaller
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Damage)
		
		
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Behavior_SurvivesDeath)

		//Event
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Movement_BowAnim)

		
	}
	
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(None);
	
	/** Input Tags **/
	// UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Look)
	// UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Move)
	// UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_EquipKnife)
	// UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_UnEquipKnife)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Toggleable)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Toggleable_TargetLock)

	namespace SetByCaller
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Projectile);
	}	// SetByCaller

	
	namespace Abilities
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(ActivateOnGiven);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Roll);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Guard);
		
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TargetLock);
		
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(ComboChange);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combo1);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combo2);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combo3);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combo4);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(ComboChangeEnd);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(ComboDamage);
		
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(HitStop);

		UE_DECLARE_GAMEPLAY_TAG_EXTERN(BasicAttack);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(BasicAttackPressed);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(BasicAttackReleased);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(HeavyAttack);
		
		
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tertiary);

		//Guard
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(GuardPressed);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(GuardReleased);
		
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Launch);

		
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Execution);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Victim);

		


		namespace Equip
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(EquipKnife);

		}
		namespace UnEquip
		{
			
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(UnEquipKnife);
		}
		
		namespace Enemy
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Range);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Melee);
			
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Melee_Attack);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Range_Attack);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Trace);
		}
	}	// Abilities

	
	namespace Events
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Block_Hit);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Block_Perfect);

		
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(SwitchTarget_Left);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(SwitchTarget_Right);

		UE_DECLARE_GAMEPLAY_TAG_EXTERN(SpawnProjectile)
		namespace Combo
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combo_Start);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combo_End);
		}
		namespace Trace
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Trace_Start);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Trace_End);
		}
		namespace Hit
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Hit);			
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(LightHit);			
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(HeavyHit);			
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(HitStop);			
		}
		namespace Equip
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Knife);
			
		}
		
		namespace UnEquip
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Knife);
		}

		namespace Execution
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Start);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(End);
		}
		
		namespace Player
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(HitReact);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Knockdown);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Death);
		}
	
		namespace Enemy
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(HitReact);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(EndAttack);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(MeleeTraceHit);
		}
	}// Events

	namespace Status
	{
		//Guard
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Guarding);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(PerfectGuard);

		//TargetLock-On
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TargetLock);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Strafing);
		
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(UnderAttack);

		//Equip
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Equip);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(UnEquip);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(BattleMode);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(IdleMode);

		//Execution
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Groggy);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Executing);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Executied);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Vulnerable_Execution);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Invincible);

		//Rolling
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Rolling);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Knockdown);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Dead);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Stun);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(HealthFull);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(HealthEmpty);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(StaminaFull);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(StaminaEmpty);
	}// Status

	namespace Cooldown
	{
				
	}

}


/*
 *#pragma endregion

#pragma region SetByCaller
#pragma endregion

#pragma region SetByCaller
#pragma endregion

*/