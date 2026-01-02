// Fill out your copyright notice in the Description page of Project Settings.


#include "C_BaseAniminstance.h"

#include "GAS_Project/MyTags.h"
#include "GAS_Project/Characters/Enemy/CEnemyBase.h"
#include "GAS_Project/GAS/CAbilitySystemStatics.h"

bool UC_BaseAniminstance::DoesOwnerHaveTag(FGameplayTag TagToCheck) const
{
	AActor* OwingPawn = TryGetPawnOwner();
	const ACEnemyBase* Enemy = Cast<ACEnemyBase>(OwingPawn);
	if (!Enemy)
	{
		// 1) 정상 루트: ASC 태그가 클라에도 존재하면 그대로 True
		return UCAbilitySystemStatics::NativeDoseActorHaveTag(OwingPawn, TagToCheck);
	}
	
	// // 2) 보정 루트: AI는 Minimal Replication에서 GE로 부여된 태그가 클라에 안 보일 수 있음
	// // ABP_Enemy가 체크하는 Strafing 태그에 한해서, Enemy가 복제해준 bool을 사용
	else if (Enemy)
	{
		static const FGameplayTag StrafingTag = MyTags::Status::Strafing;
		if (TagToCheck.MatchesTagExact(StrafingTag))
		{
			return Enemy->IsStrafing();

		}
	}
	
	
	return false;
}
