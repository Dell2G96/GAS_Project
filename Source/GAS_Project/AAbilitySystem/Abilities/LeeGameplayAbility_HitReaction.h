
#pragma once

#include "CoreMinimal.h"
#include "LeeGameplayAbility.h"
#include "LeeGameplayAbility_HitReaction.generated.h"

class UAnimMontage;

/**
 * 히트 리액션 어빌리티 (GameplayEvent 트리거형) — 연출 전용, 상태(그로기 GE)는 DefenseComponent가 관리.
 *
 * 트리거 이벤트 5종 → 몽타주 분기:
 *  - Souls.Events.Combat.Parried      → 패리당함 몽타주 + 방어자 방향 모션워핑
 *  - Souls.Events.Combat.HitReact     → 일반 경직 몽타주 (방향 섹션, 제자리)
 *  - Souls.Events.Combat.HitReactHeavy → 강공격 경직 몽타주 (방향 섹션 + 루트모션 넉백)
 *  - Souls.Events.Combat.GuardBreak   → 가드 브레이크 몽타주
 *  - Souls.Events.Combat.PostureBreak → 체간 붕괴(무릎 꿇기) 몽타주
 *
 * ActivationGroup = Exclusive_Replaceable. 공격 중(Exclusive_Blocking)인 대상은
 * DefenseComponent가 이벤트 발송 전에 Exclusive 어빌리티를 먼저 취소해 슬롯을 비워준다
 * 그로기 몽타주도 재생해야 하므로 Groggy를 ActivationBlockedTags로 막지 않는다.
 *
 * 가드브레이크 → 그로기 체인:
 *  GuardBreak 몽타주가 끝나면 DefenseComponent::EnterGroggy()를 호출한다.
 *  → GE_Groggy 적용 + PostureBreak 이벤트 발송 → 이 어빌리티가 재트리거되어 그로기 몽타주(loop) 재생.
 *  그로기 몽타주는 loop이라 스스로 끝나지 않으므로, Status.Groggy 태그가 제거될 때 어빌리티를 종료해 끊는다.
 */
UCLASS()
class GAS_PROJECT_API ULeeGameplayAbility_HitReaction : public ULeeGameplayAbility
{
	GENERATED_BODY()

public:
	ULeeGameplayAbility_HitReaction(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;
	
private:
	/** 트리거 이벤트 태그로 재생할 몽타주 선택 */
	UAnimMontage* SelectMontageForEvent(const FGameplayTag& EventTag) const;

	FName SelectStaggerSection(const AActor* Avatar, const AActor* Attacker) const;

	UFUNCTION()
	void OnMontageFinished();

	/** 그로기 GE 만료(Status.Groggy 제거) — loop 몽타주를 끊고 어빌리티 종료 */
	UFUNCTION()
	void OnGroggyTagRemoved();

	/** 이번 활성화를 트리거한 이벤트 태그. 몽타주 종료 시 후속 처리(가드브레이크→그로기 체인) 분기에 사용 */
	FGameplayTag ActiveEventTag;

protected:
	/** 패리당함 몽타주 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|HitReaction|Montage")
	TObjectPtr<UAnimMontage> ParriedReactionMontage;

	/** 일반 경직 몽타주 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|HitReaction|Montage")
	TObjectPtr<UAnimMontage> StaggerReactionMontage;

	/** 강공격 경직 몽타주 (루트모션 포함, Front/Back/Left/Right 섹션은 StaggerReactionMontage와 동일 규약) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|HitReaction|Montage")
	TObjectPtr<UAnimMontage> HeavyStaggerReactionMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|HitReaction|Montage")
	FName StaggerFrontSection = TEXT("Front");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|HitReaction|Montage")
	FName StaggerBackSection = TEXT("Back");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|HitReaction|Montage")
	FName StaggerLeftSection = TEXT("Left");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|HitReaction|Montage")
	FName StaggerRightSection = TEXT("Right");

	/** 가드 브레이크 몽타주 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|HitReaction|Montage")
	TObjectPtr<UAnimMontage> GuardBreakReactionMontage;

	/** 체간 붕괴(무릎 꿇기) 몽타주 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|HitReaction|Montage")
	TObjectPtr<UAnimMontage> GroggyReactionMontage;

	/** 패리당함 몽타주의 Motion Warping 윈도우가 참조할 워프 타겟 이름 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|HitReaction")
	FName WarpTargetName = TEXT("ParriedWarp");


};
