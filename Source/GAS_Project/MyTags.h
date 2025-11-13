// Vince Petrelli All Rights Reserved

#pragma once

#include "NativeGameplayTags.h"

namespace MyTags
{
	/** Input Tags **/
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Look)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Move)
	
	namespace SetByCaller
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Projectile);
	}	
	namespace Abilities
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(ActivateOnGiven);
	
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Primary);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Secondary);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tertiary);

		namespace Enemy
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attack);
		}
	}

	namespace Events
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(KillScored);

		namespace Player
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(HitReact);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Death);
		}
	
		namespace Enemy
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(HitReact);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(EndAttack);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(MeleeTraceHit);
		}
	}
	
	
}