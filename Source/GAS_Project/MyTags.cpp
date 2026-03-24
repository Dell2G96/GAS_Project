// Vince Petrelli All Rights Reserved


#include "MyTags.h"


namespace MyTags
{
	
	UE_DEFINE_GAMEPLAY_TAG(Movement_Mode_Walking, "Movement.Mode.Walking");
	UE_DEFINE_GAMEPLAY_TAG(Movement_Mode_NavWalking, "Movement.Mode.NavWalking");
	UE_DEFINE_GAMEPLAY_TAG(Movement_Mode_Falling, "Movement.Mode.Falling");
	UE_DEFINE_GAMEPLAY_TAG(Movement_Mode_Swimming, "Movement.Mode.Swimming");
	UE_DEFINE_GAMEPLAY_TAG(Movement_Mode_Flying, "Movement.Mode.Flying");
	UE_DEFINE_GAMEPLAY_TAG(Movement_Mode_Custom, "Movement.Mode.Custom");

	const TMap<uint8, FGameplayTag> MovementModeTagMap =
	{
		{ MOVE_Walking, Movement_Mode_Walking },
		{ MOVE_NavWalking, Movement_Mode_NavWalking },
		{ MOVE_Falling, Movement_Mode_Falling },
		{ MOVE_Swimming, Movement_Mode_Swimming },
		{ MOVE_Flying, Movement_Mode_Flying },
		{ MOVE_Custom, Movement_Mode_Custom }
	};

	const TMap<uint8, FGameplayTag> CustomMovementModeTagMap =
	{
		// Fill these in with your custom modes
	};

	
	namespace InitState
	{
		UE_DEFINE_GAMEPLAY_TAG(Spawned,"InitState.Spawned")
		UE_DEFINE_GAMEPLAY_TAG(DataAvailable , "InitState.DataAvailable")
		UE_DEFINE_GAMEPLAY_TAG(DataInitialized, "InitState.DataInitialized")
		UE_DEFINE_GAMEPLAY_TAG(GameplayReady, "InitState.GameplayReady")
	}
	
	UE_DEFINE_GAMEPLAY_TAG(None,"MyTags.None")

	namespace Lyra
	{
		UE_DEFINE_GAMEPLAY_TAG(InputTag_Move,"InputTag.Move")
		UE_DEFINE_GAMEPLAY_TAG(InputTag_Look_Mouse,"InputTag.Look.Mouse")
		
		UE_DEFINE_GAMEPLAY_TAG(InputTag_Weapon_Fire,"InputTag.Weapon.Fire")			
		UE_DEFINE_GAMEPLAY_TAG(InputTag_BowAim,"InputTag.BowAim")			
		//UE_DEFINE_GAMEPLAY_TAG(InputTag_Look_Mouse,"InputTag.Look.Mouse")


		UE_DEFINE_GAMEPLAY_TAG(UI_Layer_Game,"UI.Layer.Game")
		UE_DEFINE_GAMEPLAY_TAG(UI_Layer_GameMenu,"UI.Layer.GameMenu")
		UE_DEFINE_GAMEPLAY_TAG(UI_Layer_Menu,"UI.Layer.Menu")
		UE_DEFINE_GAMEPLAY_TAG(UI_Layer_Modal,"UI.Layer.Modal")

		UE_DEFINE_GAMEPLAY_TAG(Lyra_QickBar_Message_SlotsChanged, "Lyra.QuickBar.Message.SlotsChanged")
		UE_DEFINE_GAMEPLAY_TAG(Lyra_QickBar_Message_ActiveIndexChanged, "Lyra.QuickBar.Message.ActiveIndexChanged")


		UE_DEFINE_GAMEPLAY_TAG(Lyra_Elimination_Message, "Lyra.Elimination.Message")

		
		UE_DEFINE_GAMEPLAY_TAG(Lyra_Enemy_Found, "Lyra.Enemy.Found")
		UE_DEFINE_GAMEPLAY_TAG(Lyra_Enemy_Lost, "Lyra.Enemy.Lost")

		//Status
		UE_DEFINE_GAMEPLAY_TAG(Status_Death_Dying, "Lyra.Status.Death.Dying")
		UE_DEFINE_GAMEPLAY_TAG(Status_Death_Dead, "Lyra.Status.Death.Dead")
		UE_DEFINE_GAMEPLAY_TAG(Status_Crouching, "Lyra.Status.Crouching")
		

	}

	namespace Souls
	{
		//Gameplay
		UE_DEFINE_GAMEPLAY_TAG(Gameplay_Damage, "Souls.Gameplay.Damage")             
		UE_DEFINE_GAMEPLAY_TAG(Gameplay_DamageImmunity, "Souls.Gameplay.DamageImmunity")     
		UE_DEFINE_GAMEPLAY_TAG(Gameplay_DamageSelfDestruct, "Souls.Gameplay.SelfDestruct") 
		UE_DEFINE_GAMEPLAY_TAG(Gameplay_FellOutOfWorld, "Souls.Gameplay.FellOutOfWorld")    
		UE_DEFINE_GAMEPLAY_TAG(Gameplay_Damage_Message, "Souls.Gameplay.Message")
		UE_DEFINE_GAMEPLAY_TAG(Gameplay_MovementStopped, "Souls.Gameplay.MovementStopped")




		//Ability
		UE_DEFINE_GAMEPLAY_TAG(Ability_Type_Action_BowAim, "Souls.Ability.Type.Action.BowAim")
		
		
		//GameplayEvent
		UE_DEFINE_GAMEPLAY_TAG(GameplayEvent_Death, "Souls.GameplayEvent.Death")
		UE_DEFINE_GAMEPLAY_TAG(GameplayEvent_Bow_SpawnArrow, "Souls.GameplayEvent.Bow.SpawnArrow")


		//SetByCaller
		UE_DEFINE_GAMEPLAY_TAG(SetByCaller_Damage, "Souls.SetByCaller.Damage")


		


		UE_DEFINE_GAMEPLAY_TAG(Ability_Behavior_SurvivesDeath, "Souls.Ability.Behavior.SurvivesDeath")

		//Event
		UE_DEFINE_GAMEPLAY_TAG(Event_Movement_BowAnim, "Souls.Event.Movement.BowAnim")

		
		
		
	}

	/** Input Tags **/
	// UE_DEFINE_GAMEPLAY_TAG(InputTag_EquipKnife,"InputTag.EquipKnife")
	// UE_DEFINE_GAMEPLAY_TAG(InputTag_UnEquipKnife,"InputTag.UnEquipKnife")
	UE_DEFINE_GAMEPLAY_TAG(InputTag_Toggleable,"InputTag.Toggleable")
	UE_DEFINE_GAMEPLAY_TAG(InputTag_Toggleable_TargetLock,"InputTag.Toggleable.TargetLock")
	

	namespace SetByCaller
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Projectile, "MyTags.SetByCaller.Projectile", "Tag for Set by Caller Magnitude for Projectiles")
	
	}	
	namespace Abilities
	{
		UE_DEFINE_GAMEPLAY_TAG(ActivateOnGiven, "MyTags.Abilities.ActivateOnGiven")
		UE_DEFINE_GAMEPLAY_TAG(Roll, "MyTags.Abilities.Roll")
		UE_DEFINE_GAMEPLAY_TAG(Guard, "MyTags.Abilities.Guard")
		
		UE_DEFINE_GAMEPLAY_TAG(TargetLock, "MyTags.Abilities.TargetLock")
		
		UE_DEFINE_GAMEPLAY_TAG(ComboChange, "MyTags.Abilities.Combo.Change")
		UE_DEFINE_GAMEPLAY_TAG(Combo1, "MyTags.Abilities.Combo.Change.Combo01")
		UE_DEFINE_GAMEPLAY_TAG(Combo2, "MyTags.Abilities.Combo.Change.Combo02")
		UE_DEFINE_GAMEPLAY_TAG(Combo3, "MyTags.Abilities.Combo.Change.Combo03")
		UE_DEFINE_GAMEPLAY_TAG(Combo4, "MyTags.Abilities.Combo.Change.Combo04")
		UE_DEFINE_GAMEPLAY_TAG(ComboChangeEnd, "MyTags.Abilities.Combo.Change.End")
		UE_DEFINE_GAMEPLAY_TAG(ComboDamage, "MyTags.Abilities.Combo.Damage")
		
		UE_DEFINE_GAMEPLAY_TAG(HitStop, "MyTags.Abilities.HitStop")

		UE_DEFINE_GAMEPLAY_TAG(GuardPressed, "MyTags.Abilities.Guard.Pressed")
		UE_DEFINE_GAMEPLAY_TAG(GuardReleased, "MyTags.Abilities.Guard.Released")

		UE_DEFINE_GAMEPLAY_TAG(BasicAttack, "MyTags.Abilities.BasicAttack")
		UE_DEFINE_GAMEPLAY_TAG(BasicAttackPressed, "MyTags.Abilities.BasicAttack.Pressed")
		UE_DEFINE_GAMEPLAY_TAG(BasicAttackReleased, "MyTags.Abilities.BasicAttack.Released")
		UE_DEFINE_GAMEPLAY_TAG(HeavyAttack, "MyTags.Abilities.HeavyAttack")
		UE_DEFINE_GAMEPLAY_TAG(Tertiary, "MyTags.Abilities.Tertiary")

		
		UE_DEFINE_GAMEPLAY_TAG(Launch, "MyTags.Abilities.Launch")
		
		UE_DEFINE_GAMEPLAY_TAG(Execution, "MyTags.Abilities.Execution")
		UE_DEFINE_GAMEPLAY_TAG(Victim, "MyTags.Abilities.Victim")
		

		
		namespace Equip
		{
			UE_DEFINE_GAMEPLAY_TAG(EquipKnife, "MyTags.Abilities.Equip.Knife")
		}
		namespace UnEquip
		{
			UE_DEFINE_GAMEPLAY_TAG(UnEquipKnife, "MyTags.Abilities.UnEquip.Knife")	
		}
		

		namespace Enemy
		{
			UE_DEFINE_GAMEPLAY_TAG(Range, "MyTags.Abilities.Enemy.Range")
			UE_DEFINE_GAMEPLAY_TAG(Melee, "MyTags.Abilities.Enemy.Melee")
			UE_DEFINE_GAMEPLAY_TAG(Melee_Attack, "MyTags.Abilities.Enemy.Melee.Attack")
			UE_DEFINE_GAMEPLAY_TAG(Range_Attack, "MyTags.Abilities.Enemy.Range.Attack")
			UE_DEFINE_GAMEPLAY_TAG(Trace, "MyTags.Abilities.Enemy.Trace")
		}
	}

	namespace Events
	{
		UE_DEFINE_GAMEPLAY_TAG(Block_Hit, "MyTags.Events.Block.Hit")
		UE_DEFINE_GAMEPLAY_TAG(Block_Perfect, "MyTags.Events.Block.Perfect")
		
		UE_DEFINE_GAMEPLAY_TAG(SwitchTarget_Left, "MyTags.Events.SwitchTarget.Left")
		UE_DEFINE_GAMEPLAY_TAG(SwitchTarget_Right, "MyTags.Events.SwitchTarget.Right")

		
		UE_DEFINE_GAMEPLAY_TAG(SpawnProjectile, "MyTags.Events.SpawnProjectile")
		
		
		namespace Combo
		{
			UE_DEFINE_GAMEPLAY_TAG(Combo_Start, "MyTags.Events.Combo.Start")
			UE_DEFINE_GAMEPLAY_TAG(Combo_End, "MyTags.Events.Combo.End")
		}
		namespace Trace
		{
			UE_DEFINE_GAMEPLAY_TAG(Trace_Start, "MyTags.Events.Trace.Start")
			UE_DEFINE_GAMEPLAY_TAG(Trace_End, "MyTags.Events.Trace.End")
		}
		
		namespace Hit
		{
			UE_DEFINE_GAMEPLAY_TAG(Hit, "MyTags.Events.Hit")
			UE_DEFINE_GAMEPLAY_TAG(HitStop, "MyTags.Events.HitStop")
			UE_DEFINE_GAMEPLAY_TAG(LightHit, "MyTags.Events.Hit.LightHit")
			UE_DEFINE_GAMEPLAY_TAG(HeavyHit, "MyTags.Events.Hit.HeavyHit")
		}
		namespace Equip
		{
			UE_DEFINE_GAMEPLAY_TAG(Knife,"MyTags.Events.Equip.Knife")
			
		}
	
		namespace UnEquip
		{
			UE_DEFINE_GAMEPLAY_TAG(Knife,"MyTags.Events.UnEquip.Knife")
		}

		namespace Execution
		{
			UE_DEFINE_GAMEPLAY_TAG(Start,"MyTags.Events.Execution.Start")
			UE_DEFINE_GAMEPLAY_TAG(End,"MyTags.Events.Execution.End")
		}
		
		namespace Player
		{
			UE_DEFINE_GAMEPLAY_TAG(HitReact, "MyTags.Events.Player.HitReact")
			UE_DEFINE_GAMEPLAY_TAG(Knockdown, "MyTags.Events.Player.Knockdown")
			UE_DEFINE_GAMEPLAY_TAG(Death, "MyTags.Events.Player.Death")
		}
	
		namespace Enemy
		{
			UE_DEFINE_GAMEPLAY_TAG(HitReact, "MyTags.Events.Enemy.HitReact")
			UE_DEFINE_GAMEPLAY_TAG(EndAttack, "MyTags.Events.Enemy.EndAttack")
			UE_DEFINE_GAMEPLAY_TAG(MeleeTraceHit, "MyTags.Events.Enemy.MeleeTraceHit")
		}
	}
	namespace Status
	{
		UE_DEFINE_GAMEPLAY_TAG(Guarding, "MyTags.Status.Guarding")
		UE_DEFINE_GAMEPLAY_TAG(PerfectGuard, "MyTags.Status.PerfectGuard")
		
		UE_DEFINE_GAMEPLAY_TAG(TargetLock, "MyTags.Status.TargetLock")
		
		UE_DEFINE_GAMEPLAY_TAG(Strafing, "MyTags.Status.Strafing")
		UE_DEFINE_GAMEPLAY_TAG(UnderAttack, "MyTags.Status.UnderAttack")
		
		UE_DEFINE_GAMEPLAY_TAG(Equip, "MyTags.Status.Equip")
		UE_DEFINE_GAMEPLAY_TAG(UnEquip, "MyTags.Status.UnEquip")

		
		UE_DEFINE_GAMEPLAY_TAG(Groggy, "MyTags.Status.Groggy")
		UE_DEFINE_GAMEPLAY_TAG(Executing, "MyTags.Status.Executing")
		UE_DEFINE_GAMEPLAY_TAG(Executed, "MyTags.Status.Executed")
		
		
		UE_DEFINE_GAMEPLAY_TAG(BattleMode, "MyTags.Status.Battle")
		UE_DEFINE_GAMEPLAY_TAG(IdleMode, "MyTags.Status.IdleMode")
		UE_DEFINE_GAMEPLAY_TAG(Rolling, "MyTags.Status.Rolling")
		
		UE_DEFINE_GAMEPLAY_TAG(Knockdown, "MyTags.Status.Knockdown")
		UE_DEFINE_GAMEPLAY_TAG(Dead, "MyTags.Status.Dead")
		UE_DEFINE_GAMEPLAY_TAG(Stun, "MyTags.Status.Stun")
		
		UE_DEFINE_GAMEPLAY_TAG(HealthFull, "MyTags.Status.Health.Full")
		UE_DEFINE_GAMEPLAY_TAG(HealthEmpty, "MyTags.Status.Health.Empty")
		UE_DEFINE_GAMEPLAY_TAG(StaminaFull, "MyTags.Status.Stamina.Full")
		UE_DEFINE_GAMEPLAY_TAG(StaminaEmpty, "MyTags.Status.Stamina.Empty")
	}
}
