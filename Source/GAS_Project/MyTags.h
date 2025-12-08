// Vince Petrelli All Rights Reserved

#pragma once

#include "NativeGameplayTags.h"

namespace MyTags
{
	/** Input Tags **/
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Look)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Move)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_EquipKnife)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_UnEquipKnife)

	namespace SetByCaller
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Projectile);
	}	// SetByCaller

	
	namespace Abilities
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(ActivateOnGiven);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(ComboChange);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combo1);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combo2);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combo3);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combo4);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(ComboChangeEnd);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(ComboDamage);

		namespace Equip
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(EquipKnife);

		}
		namespace UnEquip
		{
			
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(UnEquipKnife);
		}

		UE_DECLARE_GAMEPLAY_TAG_EXTERN(BasicAttack);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(BasicAttackPressed);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(BasicAttackReleased);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(HeavyAttack);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tertiary);

		namespace Enemy
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attack);
		}
	}	// Abilities

	
	namespace Events
	{
		namespace Combo
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(ComboStart);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(ComboEnd);
		}
		namespace Trace
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(TraceStart);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(TraceEnd);
		}
		namespace Hit
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Hit);			
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(LightHit);			
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(HeavyHit);			
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
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Death);
		}
	
		namespace Enemy
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(HitReact);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(EndAttack);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(MeleeTraceHit);
		}
	}

	namespace Status
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Equip);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(UnEquip);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(BattleMode);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(IdleMode);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Dead);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Stun);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(HealthFull);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(HealthEmpty);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(StaminaFull);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(StaminaEmpty);
	}
}


/*
 *#pragma endregion

#pragma region SetByCaller
#pragma endregion

#pragma region SetByCaller
#pragma endregion

*/