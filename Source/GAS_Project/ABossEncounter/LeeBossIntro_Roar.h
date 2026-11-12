// 26.11.12 신규 - 보스 포효 인트로

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LeeBossIntro.h"
#include "LeeBossIntro_Roar.generated.h"

class UAbilitySystemComponent;
class UGameplayAbility;

// 보스의 포효 어빌리티를 태그로 실행하고, 끝나면 완료를 알리는 인트로
// 포효를 실행하지 못하면 바로 완료하고, 끝 신호가 오지 않아도 MaxDuration 뒤에는 완료한다
UCLASS(meta = (DisplayName = "Boss Intro - Roar"))
class GAS_PROJECT_API ULeeBossIntro_Roar : public ULeeBossIntro
{
	GENERATED_BODY()

public:
	virtual void StartIntro(AActor* Boss, FSimpleDelegate OnFinished) override;
	virtual void CancelIntro() override;

protected:
	// 실행할 포효 어빌리티 태그 (보스에게 부여돼 있어야 함). 비우면 Souls.Abilities.Boss.Stone.Roar
	UPROPERTY(EditAnywhere, Category = "Lee|BossIntro")
	FGameplayTag RoarAbilityTag;

	// 포효가 끝났다는 신호가 없을 때 강제로 전투를 시작할 시간 (초)
	UPROPERTY(EditAnywhere, Category = "Lee|BossIntro", meta = (ClampMin = "0.5"))
	float MaxDuration = 6.0f;

private:
	// 실제로 쓸 포효 태그
	FGameplayTag GetRoarTag() const;

	// 포효 어빌리티가 끝났는지 확인
	void HandleAbilityEnded(UGameplayAbility* EndedAbility);

	// 구독·타이머를 정리하고 완료를 알린다 (한 번만)
	void Finish();

	// 구독·타이머 정리
	void Cleanup();

	TWeakObjectPtr<UAbilitySystemComponent> BossASC;
	FDelegateHandle AbilityEndedHandle;
	FTimerHandle TimeoutHandle;
	FSimpleDelegate PendingOnFinished;
};
