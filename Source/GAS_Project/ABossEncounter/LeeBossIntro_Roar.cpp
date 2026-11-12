// 26.11.12 신규 - 보스 포효 인트로

#include "LeeBossIntro_Roar.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Abilities/GameplayAbility.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "GAS_Project/LeeLogChannels.h"

// 지정한 포효 태그, 비어 있으면 Stone 보스 포효 태그 (CDO 생성 시점엔 태그가 아직 없을 수 있어 실행 시점에 찾는다)
FGameplayTag ULeeBossIntro_Roar::GetRoarTag() const
{
	return RoarAbilityTag.IsValid() ? RoarAbilityTag : FGameplayTag::RequestGameplayTag(TEXT("Souls.Abilities.Boss.Stone.Roar"), /*ErrorIfNotFound*/ false);
}

// 포효 어빌리티를 실행하고 끝나기를 기다린다. 실행 실패면 바로 완료
void ULeeBossIntro_Roar::StartIntro(AActor* Boss, FSimpleDelegate OnFinished)
{
	Cleanup();
	PendingOnFinished = OnFinished;

	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Boss);
	UWorld* World = Boss ? Boss->GetWorld() : nullptr;
	const FGameplayTag RoarTag = GetRoarTag();
	if (!ASC || !World || !RoarTag.IsValid())
	{
		UE_LOG(LogLee, Warning, TEXT("[BossIntro_Roar] 보스(%s) ASC 또는 포효 태그가 없어 인트로를 건너뜁니다."), *GetNameSafe(Boss));
		Finish();
		return;
	}

	BossASC = ASC;
	AbilityEndedHandle = ASC->AbilityEndedCallbacks.AddUObject(this, &ThisClass::HandleAbilityEnded);
	World->GetTimerManager().SetTimer(TimeoutHandle, FTimerDelegate::CreateUObject(this, &ThisClass::Finish), MaxDuration, false);

	if (!ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(RoarTag)))
	{
		UE_LOG(LogLee, Warning, TEXT("[BossIntro_Roar] 포효(%s) 실행 실패 → 바로 전투 시작"), *RoarTag.ToString());
		Finish();
	}
}

// 구독·타이머를 정리하고 완료 알림을 버린다
void ULeeBossIntro_Roar::CancelIntro()
{
	Cleanup();
	PendingOnFinished.Unbind();
}

// 끝난 어빌리티가 포효면 완료
void ULeeBossIntro_Roar::HandleAbilityEnded(UGameplayAbility* EndedAbility)
{
	if (EndedAbility && EndedAbility->GetAssetTags().HasTag(GetRoarTag()))
	{
		Finish();
	}
}

// 정리 후 완료를 한 번만 알린다 (콜백 안에서 다시 불려도 안전하게 먼저 비운다)
void ULeeBossIntro_Roar::Finish()
{
	Cleanup();

	FSimpleDelegate Callback = PendingOnFinished;
	PendingOnFinished.Unbind();
	Callback.ExecuteIfBound();
}

// 포효 종료 구독과 시간 초과 타이머를 해제한다
void ULeeBossIntro_Roar::Cleanup()
{
	if (UAbilitySystemComponent* ASC = BossASC.Get())
	{
		ASC->AbilityEndedCallbacks.Remove(AbilityEndedHandle);
		if (UWorld* World = ASC->GetWorld())
		{
			World->GetTimerManager().ClearTimer(TimeoutHandle);
		}
	}
	BossASC.Reset();
	AbilityEndedHandle.Reset();
}
