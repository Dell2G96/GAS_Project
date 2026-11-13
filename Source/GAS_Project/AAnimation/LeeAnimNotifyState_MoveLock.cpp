// 26.11.13 신규 - 몽타주 이동 잠금 구간 AnimNotifyState

#include "LeeAnimNotifyState_MoveLock.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GAS_Project/MyTags.h"

// 생성자 — 에디터 타임라인 색 지정
ULeeAnimNotifyState_MoveLock::ULeeAnimNotifyState_MoveLock()
{
#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(200, 60, 60, 255);
#endif
}

// 구간 시작 — 이동 잠금 태그 +1
// 슈퍼아머와 달리 권한 검사를 하지 않는다: 서버에만 붙이면 클라이언트 예측 이동과 어긋나 위치 보정이 생긴다
void ULeeAnimNotifyState_MoveLock::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner))
	{
		ASC->AddLooseGameplayTag(MyTags::Souls::Status_MoveLock);
	}
}

// 구간 끝(몽타주가 끊겨도 호출됨) — 이동 잠금 태그 -1
void ULeeAnimNotifyState_MoveLock::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner);
	if (ASC && ASC->GetTagCount(MyTags::Souls::Status_MoveLock) > 0)
	{
		ASC->RemoveLooseGameplayTag(MyTags::Souls::Status_MoveLock);
	}
}

// 몽타주 타임라인에 보일 이름
FString ULeeAnimNotifyState_MoveLock::GetNotifyName_Implementation() const
{
	return TEXT("Move Lock");
}
