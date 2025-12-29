// Fill out your copyright notice in the Description page of Project Settings.



#include "GAS_Project/GAS/GameplayCue/AHitGameplayCue.h"

#include "GAS_Project/Characters/CCharacter.h"

AHitGameplayCue::AHitGameplayCue()
{
	// 스폰할 때마다 새로운 인스턴스 생성
	bUniqueInstancePerInstigator = true;
    
	// 제거 시 자동 파괴
	bAutoDestroyOnRemove = true;
	AutoDestroyDelay = 0.1f;
}

bool AHitGameplayCue::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	if (!MyTarget)
	{
		return false;
	}

	// 피격 효과 적용
	ApplyHitEffect(MyTarget);

	// N초 후 복원하는 타이머 설정
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			HitRecoveryTimerHandle,
			[this, WeakTarget = TWeakObjectPtr<AActor>(MyTarget)]()
			{
				if (AActor* Target = WeakTarget.Get())
				{
					RestoreHitEffect(Target);
				}
			},
			HitDuration,
			false // 반복 안함
		);
	}

	return true;
}


bool AHitGameplayCue::OnActive(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	if (!Super::OnActive(MyTarget, Parameters))
	{
		return false;
	}

	if (!MyTarget)
	{
		return false;
	}

	// 피격 효과 적용
	ApplyHitEffect(MyTarget);

	// N초 후 복원하는 타이머 설정
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			HitRecoveryTimerHandle,
			[this, WeakTarget = TWeakObjectPtr<AActor>(MyTarget)]()
			{
				if (AActor* Target = WeakTarget.Get())
				{
					RestoreHitEffect(Target);
				}
			},
			HitDuration,
			false // 반복 안함
		);
	}

	return true;
}

void AHitGameplayCue::ApplyHitEffect(AActor* TargetActor)
{
	ACCharacter* Character = Cast<ACCharacter>(TargetActor);
	if (!Character)
	{
		return;
	}

	// 스켈레탈 메시의 모든 머티리얼 가져오기
	USkeletalMeshComponent* SkeletalMesh = Character->GetMesh();
	if (!SkeletalMesh)
	{
		return;
	}

	// 각 머티리얼 슬롯에 대해 Dynamic Material Instance 생성
	SkeletalMesh->SetScalarParameterValueOnMaterials(HitColorParameterName, 1.0f); // 예시로 색상 파라미터 설정
}

void AHitGameplayCue::RestoreHitEffect(AActor* TargetActor)
{
	ACCharacter* Character = Cast<ACCharacter>(TargetActor);
	if (!Character)
	{
		return;
	}

	USkeletalMeshComponent* SkeletalMesh = Character->GetMesh();
	if (!SkeletalMesh)
	{
		return;
	}

	SkeletalMesh->SetScalarParameterValueOnMaterials(HitColorParameterName, 0.f); // 예시로 색상 파라미터 설정

}
