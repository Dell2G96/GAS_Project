// Vince Petrelli All Rights Reserved


#include "MyTags.h"

namespace MyTags
{
	/** Input Tags **/
	UE_DEFINE_GAMEPLAY_TAG(InputTag_Move,"InputTag.Move")
	UE_DEFINE_GAMEPLAY_TAG(InputTag_Look,"InputTag.Look")

	namespace SetByCaller
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Projectile, "MyTags.SetByCaller.Projectile", "Tag for Set by Caller Magnitude for Projectiles")
	
	}	
	namespace Abilities
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(ActivateOnGiven, "MyTags.Abilities.ActivateOnGiven", "Tag for Abilities that should activate immediately once given.")
	
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(BasicAttack, "MyTags.Abilities.BasicAttack", "Tag for the Primary Ability")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(HeavyAttack, "MyTags.Abilities.HeavyAttack", "Tag for the Secondary Ability")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Tertiary, "MyTags.Abilities.Tertiary", "Tag for the Tertiary Ability")

		namespace Enemy
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attack, "MyTags.Abilities.Enemy.Attack", "Enemy Attack Tag")
		}
	}

	namespace Events
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(KillScored, "MyTags.Events.KillScored", "Tag for the KillScored Event")

		namespace Player
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(HitReact, "MyTags.Events.Player.HitReact", "Tag for the Player HitReact Event")
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Death, "MyTags.Events.Player.Death", "Tag for the Player Death Event")
		}
	
		namespace Enemy
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(HitReact, "MyTags.Events.Enemy.HitReact", "Tag for the Enemy HitReact Event")
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(EndAttack, "MyTags.Events.Enemy.EndAttack", "Tag for the Enemy Ending an Attack")
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(MeleeTraceHit, "MyTags.Events.Enemy.MeleeTraceHit", "Tag for the Enemy Melee Trace Hit")
		}
	}
}
