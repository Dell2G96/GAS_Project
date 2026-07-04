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
		// [신규] 처형/암살 공용 입력 태그 (F키) — InputConfig에서 GA_Finisher와 연결
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Finisher)

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
		
		
		// QuickBar
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

		//AilityType
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Type_Action_BowAim)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Type_Action_BowFire)
		
		//Ability
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Execution);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Assassination);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_AssassinationVictim);

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
		
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Assassination_Start);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Assassination_End);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Execution_Start);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Execution_End);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_BeFinished);
		// [신규] 피니셔 데미지 타이밍 — 공격자 몽타주의 AnimNotify가 발사, GA_Finisher가 수신
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Finisher_Damage);

		// [신규] 피니셔 프롬프트 UI 메시지 채널 (GameplayMessageSubsystem)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Finisher_Prompt);

		// GameplayCue
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_FinishIndicator);
		
		
		//Status
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Groggy);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Executing);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Executied);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Assassinating);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Assassinated);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Unaware);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Vulnerable_Execution);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Invincible);
		// [신규] 피해자: 처형/암살 당하는 중 (GA_FinisherVictim의 ActivationOwnedTags, 중복 피니셔 잠금 겸용)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Finisher_Victim);
		// [신규] 근접 공격 어빌리티 식별 태그 — TryActivateAbilityByTag에서 사용
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Attack_Melee);
		// [신규] 공격 중 상태 태그 — 어빌리티 활성 동안 ActivationOwnedTags로 자동 부여/제거
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Attack_Attacking);
		
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
