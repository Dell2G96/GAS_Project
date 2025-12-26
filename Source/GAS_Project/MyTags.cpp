// Vince Petrelli All Rights Reserved


#include "MyTags.h"

namespace MyTags
{
	UE_DEFINE_GAMEPLAY_TAG(None,"MyTags.None")
	
	/** Input Tags **/
	UE_DEFINE_GAMEPLAY_TAG(InputTag_Move,"InputTag.Move")
	UE_DEFINE_GAMEPLAY_TAG(InputTag_Look,"InputTag.Look")
	UE_DEFINE_GAMEPLAY_TAG(InputTag_EquipKnife,"InputTag.EquipKnife")
	UE_DEFINE_GAMEPLAY_TAG(InputTag_UnEquipKnife,"InputTag.UnEquipKnife")

	namespace SetByCaller
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Projectile, "MyTags.SetByCaller.Projectile", "Tag for Set by Caller Magnitude for Projectiles")
	
	}	
	namespace Abilities
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(ActivateOnGiven, "MyTags.Abilities.ActivateOnGiven", "Tag for Abilities that should activate immediately once given.")

		UE_DEFINE_GAMEPLAY_TAG(ComboChange, "MyTags.Abilities.Combo.Change")
		UE_DEFINE_GAMEPLAY_TAG(Combo1, "MyTags.Abilities.Combo.Change.Combo01")
		UE_DEFINE_GAMEPLAY_TAG(Combo2, "MyTags.Abilities.Combo.Change.Combo02")
		UE_DEFINE_GAMEPLAY_TAG(Combo3, "MyTags.Abilities.Combo.Change.Combo03")
		UE_DEFINE_GAMEPLAY_TAG(Combo4, "MyTags.Abilities.Combo.Change.Combo04")
		UE_DEFINE_GAMEPLAY_TAG(ComboChangeEnd, "MyTags.Abilities.Combo.Change.End")
		UE_DEFINE_GAMEPLAY_TAG(ComboDamage, "MyTags.Abilities.Combo.Damage")
		UE_DEFINE_GAMEPLAY_TAG(HitStop, "MyTags.Abilities.HitStop")

		namespace Equip
		{
			UE_DEFINE_GAMEPLAY_TAG(EquipKnife, "MyTags.Abilities.Equip.Knife")
		}
		namespace UnEquip
		{
			UE_DEFINE_GAMEPLAY_TAG(UnEquipKnife, "MyTags.Abilities.UnEquip.Knife")	
		}
		
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(BasicAttack, "MyTags.Abilities.BasicAttack", "Tag for the Primary Ability")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(BasicAttackPressed, "MyTags.Abilities.BasicAttack.Pressed", "Tag for the Primary Ability")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(BasicAttackReleased, "MyTags.Abilities.BasicAttack.Released", "Tag for the Primary Ability")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(HeavyAttack, "MyTags.Abilities.HeavyAttack", "Tag for the Secondary Ability")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Tertiary, "MyTags.Abilities.Tertiary", "Tag for the Tertiary Ability")

		namespace Enemy
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attack, "MyTags.Abilities.Enemy.Attack", "Enemy Attack Tag")
		}
	}

	namespace Events
	{
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
		
		namespace Player
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(HitReact, "MyTags.Events.Player.HitReact", "Tag for the Player HitReact Event")
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Knockdown, "MyTags.Events.Player.Knockdown", "Tag for the Player Death Event")
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Death, "MyTags.Events.Player.Death", "Tag for the Player Death Event")
		}
	
		namespace Enemy
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(HitReact, "MyTags.Events.Enemy.HitReact", "Tag for the Enemy HitReact Event")
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(EndAttack, "MyTags.Events.Enemy.EndAttack", "Tag for the Enemy Ending an Attack")
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(MeleeTraceHit, "MyTags.Events.Enemy.MeleeTraceHit", "Tag for the Enemy Melee Trace Hit")
		}
	}
	namespace Status
	{
		UE_DEFINE_GAMEPLAY_TAG(Strafing, "MyTags.Status.Strafing")
		UE_DEFINE_GAMEPLAY_TAG(Equip, "MyTags.Status.Equip")
		UE_DEFINE_GAMEPLAY_TAG(UnEquip, "MyTags.Status.UnEquip")
		UE_DEFINE_GAMEPLAY_TAG(BattleMode, "MyTags.Status.Battle")
		UE_DEFINE_GAMEPLAY_TAG(IdleMode, "MyTags.Status.IdleMode")
		UE_DEFINE_GAMEPLAY_TAG(Knockdown, "MyTags.Status.Knockdown")
		UE_DEFINE_GAMEPLAY_TAG(Dead, "MyTags.Status.Dead")
		UE_DEFINE_GAMEPLAY_TAG(Stun, "MyTags.Status.Stun")
		UE_DEFINE_GAMEPLAY_TAG(HealthFull, "MyTags.Status.Health.Full")
		UE_DEFINE_GAMEPLAY_TAG(HealthEmpty, "MyTags.Status.Health.Empty")
		UE_DEFINE_GAMEPLAY_TAG(StaminaFull, "MyTags.Status.Stamina.Full")
		UE_DEFINE_GAMEPLAY_TAG(StaminaEmpty, "MyTags.Status.Stamina.Empty")
	}
}
