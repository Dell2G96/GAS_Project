// Vince Petrelli All Rights Reserved

#pragma once

#include "NativeGameplayTags.h"

// 프로젝트 전역 GameplayTag 선언부.
// 네임스페이스 = 태그 계열, 각 네임스페이스 내부는 입력 → 어빌리티 → 상태 → 이벤트 → 메시지/데미지/큐 순으로 정렬
namespace MyTags
{
	// CharacterMovementComponent의 MovementMode ↔ GameplayTag 매핑 테이블 (MyTags.cpp에서 정의)
	extern const TMap<uint8, FGameplayTag> MovementModeTagMap;
	extern const TMap<uint8, FGameplayTag> CustomMovementModeTagMap;

	// ===== 공용 =====
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(None);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Toggleable)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Toggleable_TargetLock)

	// ----- 기타 -----
	// UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Look)
	// UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Move)
	// UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_EquipKnife)
	// UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_UnEquipKnife)

	// ===== 초기화 단계 (Lyra InitState) =====
	namespace InitState
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Spawned)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(DataAvailable)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(DataInitialized)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayReady)
	}

	// ===== Lyra 프레임워크 이식분 =====
	namespace Lyra
	{
		// ----- 입력 -----
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Move)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Look_Mouse)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Weapon_Fire)

		// ----- 어빌리티 -----
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Interaction_Activate);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Interaction_Duraction_Message);

		// ----- 상태 -----
		// 이건 공용으로 사용하니까 살려야할지,,,말아야할지 고민 중
		// UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Death_Dying)
		// UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Death_Dead)
		// UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Death_Dead)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Crouching)

		// ----- 메시지 채널 -----
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Lyra_QickBar_Message_SlotsChanged)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Lyra_QickBar_Message_ActiveIndexChanged)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Lyra_Elimination_Message)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Lyra_Enemy_Found)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Lyra_Enemy_Lost)

		// ----- UI / 플랫폼 -----
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_Game)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_GameMenu)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_Menu)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_Modal)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Platform_Trait_Input_PrimarlyController)
	}

	// ===== 소울라이크 신규 계열 =====
	namespace Souls
	{
		// ----- 입력 (InputTag) -----
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_BowAim)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_BowFire)
		// Player 약공격/강공격 입력 태그 — LeeInputConfig/AbilitySet에서 사용
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Attack_Light);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Attack_Heavy);
		// 가드(Hold) / 회피 입력 — LeeInputConfig의 AbilityInputActions에 연결
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Guard)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Dodge)
		// 처형/암살 공용 입력 태그 (F키) — InputConfig에서 GA_Finisher와 연결
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Finisher)
		// 타겟 락온 입력 태그 — 토글은 AbilityInputActions(GA_TargetLock), 전환은 NativeInputActions
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_TargetLock)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_TargetLock_SwitchLeft)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_TargetLock_SwitchRight)

		// ----- 어빌리티 식별 태그 (AbilityTags / CancelAbilities 매칭용) -----
		// 약공격 어빌리티 식별 태그 — 약공격 BP의 AbilityTags에서 사용
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Attack_Melee_Light);
		// 강공격 어빌리티 식별 태그 — 강공격 BP의 AbilityTags에서 사용
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Attack_Melee_Heavy);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Guard);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Dodge);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_HitReaction);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Execution);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Assassination);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_AssassinationVictim);
		// 타겟 락온 — 어빌리티 식별 태그, 상태 태그(레거시 MyTags::Status::TargetLock과 별개 네임스페이스), UI 메시지 채널
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_TargetLock);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Type_Action_BowAim)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Type_Action_BowFire)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Behavior_SurvivesDeath)

		// ----- 쿨다운 -----
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_BowFire)

		// ----- 공통 게임플레이 -----
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gameplay_Damage)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gameplay_DamageImmunity)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gameplay_DamageSelfDestruct)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gameplay_FellOutOfWorld)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gameplay_Damage_Message)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gameplay_MovementStopped)

		// ----- 상태 - 전투 (공격/방어) -----
		// 근접 공격 어빌리티 식별 태그 — TryActivateAbilityByTag에서 사용
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Attack_Melee);
		// 공격 중 상태 태그 — 어빌리티 활성 동안 ActivationOwnedTags로 자동 부여/제거
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Attack_Attacking);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Guard_Active);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Guard_Perfect);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Dodge_Active);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Dodge_Perfect);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_CounterWindow);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Stamina_RegenBlocked);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Invincible);

		// ----- 상태 - 처형/암살 (피니셔) -----
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Groggy);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Vulnerable_Execution);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Executing);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Executied);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Assassinating);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Assassinated);
		// 피해자: 처형/암살 당하는 중 (GA_FinisherVictim의 ActivationOwnedTags, 중복 피니셔 잠금 겸용)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Finisher_Victim);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Unaware);

		// ----- 상태 - 사망 / 락온 -----
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Death_Dying);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Death_Dead);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_TargetLock);

		// ----- 이벤트 - 사망 / 이동 -----
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEvent_Death)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEvent_Bow_SpawnArrow)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Movement_BowAnim)

		// ----- 이벤트 - 공격 (몽타주 AnimNotify 발신) -----
		//  공격 단계별 스태미나 비용 이벤트 — 몽타주 AnimNotify가 발사, AttackMelee가 반복 수신
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Attack_CommitStep);
		// 콤보 입력 허용 구간 시작/끝 — 몽타주의 Lee Gameplay Event Window(Server) 노티파이가 발사
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Attack_ComboWindowOpen);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Attack_ComboWindowClose);
		// 콤보 전환 실행 시점 — 몽타주의 Lee Gameplay Event(Server) 단발 노티파이가 발사, 큐된 입력을 이 시점에 소비
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Attack_ComboTransition);
		// 다단 공격 몽타주에서 Warp 구간마다 워프 타깃을 재계산 — Lee Gameplay Event(Server) 노티파이가 2번째+ Warp 구간 직전에 발사
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Attack_RefreshWarpTarget);

		// ----- 이벤트 - 방어 판정 (ULeeDefenseComponent 발신) -----
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Defense_GuardHit);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Defense_PerfectGuard);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Defense_PerfectDodge);

		// ----- 이벤트 - 피격 리액션 -----
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Combat_HitReact);
		// 강공격 피격 이벤트 — HitReact의 형제 태그 (추후 GA_HitReaction 확장에서 사용)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Combat_HitReactHeavy);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Combat_Parried);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Combat_GuardBreak);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Combat_PostureBreak);

		// ----- 이벤트 - 처형/암살 (피니셔) -----
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Execution_Start);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Execution_End);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Assassination_Start);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Assassination_End);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_BeFinished);
		// [신규] 피니셔 데미지 타이밍 — 공격자 몽타주의 AnimNotify가 발사, GA_Finisher가 수신
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Finisher_Damage);

		// ----- 이벤트 - i-frame 윈도우 -----
		// i-frame 윈도우 이벤트 — ULeeAnimNotifyState_GameplayEvent가 발사 (보조 신호용)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Window_IFrame_Begin);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Window_IFrame_End);

		// ----- UI 메시지 채널 (GameplayMessageSubsystem) -----
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Finisher_Prompt);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_TargetLock);

		// ----- 데미지 판정 결과 (ExecCalc → Spec DynamicAssetTags) -----
		// 데미지 판정 결과 — ULeeExecCalc_Damage가 Spec DynamicAssetTags에 기록,
		// LeeSoulsStatSet::PostGameplayEffectExecute → ULeeDefenseComponent가 읽어 분기
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(DamageResult_HitReact);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(DamageResult_GuardHit);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(DamageResult_PerfectGuard);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(DamageResult_PerfectDodge);

		// ----- 데미지 속성 / 원인 -----
		// [신규] 강공격 속성 태그 — 공격 GE Spec의 DynamicAssetTag로 전달 (추후 강공격 피격 리액션에서 사용)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(DamageType_Attack_Heavy);
		// [신규] 가드 불가 속성 태그 — 정의만, 실제 판정 분기는 추후 구현
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(DamageType_Attack_Unblockable);
		// 스태미나 감소 원인 태그 — 스태미나 0 도달 시 GuardBreak/PostureBreak 분기 근거 (리뷰 P0-3)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(DamageType_ParryCounter);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(DamageType_DodgeCost);

		// ----- SetByCaller -----
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Damage)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_StaminaDamage);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Duration);

		// ----- GameplayCue -----
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_FinishIndicator);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Dodge_Perfect);

		// ----- AI - 어택 토큰 Quota (ULeeAttackTokenComponent) -----
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attacker_Melee);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attacker_Melee_Light);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attacker_Melee_Heavy);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attacker_Ranged);

		// ----- AI - StateTree 이벤트 (SendStateTreeEvent) -----
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(AIEvent_Died);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(AIEvent_TargetChanged);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(AIEvent_TargetInvalidated);
		// [신규] 그로기 진입/해제 AI 이벤트 — ULeeTargetSelectionComponent가 Status_Groggy 태그 변화를 감지해 발신
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(AIEvent_Groggy_Begin);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(AIEvent_Groggy_End);
	}

	// ===== 레거시 계열 (C 접두사 클래스에서 사용) =====
	namespace SetByCaller
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Projectile);
	}

	namespace Abilities
	{
		// ----- 공용 -----
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(ActivateOnGiven);

		// ----- 기본 공격 / 콤보 -----
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(BasicAttack);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(BasicAttackPressed);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(BasicAttackReleased);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(HeavyAttack);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(ComboChange);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combo1);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combo2);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combo3);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combo4);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(ComboChangeEnd);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(ComboDamage);

		// ----- 가드 / 회피 -----
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Guard);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(GuardPressed);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(GuardReleased);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Roll);

		// ----- 처형 -----
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Execution);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Victim);

		// ----- 기타 -----
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TargetLock);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(HitStop);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Launch);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tertiary);

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
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Melee);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Melee_Attack);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Range);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Range_Attack);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Trace);
		}
	}	// Abilities

	namespace Events
	{
		// ----- 가드 -----
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Block_Hit);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Block_Perfect);

		// ----- 타겟 전환 -----
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(SwitchTarget_Left);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(SwitchTarget_Right);

		// ----- 투사체 -----
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
	}	// Events

	namespace Status
	{
		// ----- 가드 -----
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Guarding);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(PerfectGuard);

		// ----- 락온 / 이동 -----
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TargetLock);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Strafing);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Rolling);

		// ----- 장비 / 전투 모드 -----
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Equip);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(UnEquip);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(BattleMode);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(IdleMode);

		// ----- 처형 -----
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Groggy);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Executing);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Executied);

		// ----- 피격 / 사망 -----
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(UnderAttack);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Knockdown);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Stun);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Dead);

		// ----- 어트리뷰트 임계값 -----
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(HealthFull);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(HealthEmpty);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(StaminaFull);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(StaminaEmpty);
	}
}
