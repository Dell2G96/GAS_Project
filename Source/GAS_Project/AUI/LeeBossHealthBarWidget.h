#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LeeBossHealthBarWidget.generated.h"

class ULeeAttributeBarWidget;
class UTextBlock;

// 26.11.07 추가 - 화면 상단 중앙의 보스 HUD (W_BossHealthBar)
// "Boss" Actor Tag가 붙은 살아 있는 보스를 찾으면 이름·HP·스태미나 바를 보여주고, 보스 HP가 0이 되면 숨긴다
UCLASS(Abstract, Blueprintable)
class GAS_PROJECT_API ULeeBossHealthBarWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// 보스를 찾거나, 바인딩한 보스가 사라졌는지 확인
	void RefreshBoss();

	// 월드에서 "Boss" 태그가 붙은 살아 있는 보스를 찾는다 (없으면 nullptr)
	AActor* FindAliveBoss() const;

	// 보스에 바들을 바인딩하고 표시 (nullptr이면 해제하고 숨김)
	void SetBoss(AActor* NewBoss);

	// 보스 HP가 남아 있는지 (클라이언트에도 복제되는 HP로 판단)
	static bool IsBossAlive(const AActor* Boss);

	// 보스 바 전체를 담는 패널. 이 패널만 숨긴다 (위젯 자신을 Collapsed로 두면 Tick이 멈춤)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> Panel_BossBar;

	// 보스 이름 (바 아래)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_BossName;

	// 보스 HP 바 (ULeeAttributeBarWidget 자식 BP, AttributeType=Health, bBindToOwningPawn=false)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<ULeeAttributeBarWidget> Bar_BossHealth;

	// 보스 스태미나 바 (ULeeAttributeBarWidget 자식 BP, AttributeType=Stamina, bBindToOwningPawn=false)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<ULeeAttributeBarWidget> Bar_BossStamina;

	// 보스를 찾을 Actor Tag (ULeeBossPartySubsystem의 보스 처치 판정과 같은 태그)
	UPROPERTY(EditDefaultsOnly, Category = "Lee|BossHUD")
	FName BossActorTag = TEXT("Boss");

	// 보스 검색·상태 확인 주기(초)
	UPROPERTY(EditDefaultsOnly, Category = "Lee|BossHUD", meta = (ClampMin = "0.05"))
	float RefreshInterval = 0.25f;

private:
	TWeakObjectPtr<AActor> BoundBoss;

	float TimeSinceRefresh = 0.0f;
};
