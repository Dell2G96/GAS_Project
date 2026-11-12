#include "LeeBossHealthBarWidget.h"

#include "LeeAttributeBarWidget.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "GAS_Project/ACharacter/LeeCharacter.h"
#include "GAS_Project/ACharacter/LeeHealthComponent.h"
#include "GAS_Project/ABossEncounter/LeeBossEncounterManager.h"

// 생성 직후 숨긴 상태로 시작하고 바로 한 번 보스를 찾는다
void ULeeBossHealthBarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 위젯 자신은 항상 보이게(클릭 통과) 두고 패널만 숨긴다 — 자신을 숨기면 NativeTick이 멈춘다
	SetVisibility(ESlateVisibility::HitTestInvisible);
	SetBoss(nullptr);
	RefreshBoss();
}

// 위젯 제거 시 바인딩 해제
void ULeeBossHealthBarWidget::NativeDestruct()
{
	SetBoss(nullptr);
	Super::NativeDestruct();
}

// RefreshInterval마다 보스 상태 확인
void ULeeBossHealthBarWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	TimeSinceRefresh += InDeltaTime;
	if (TimeSinceRefresh >= RefreshInterval)
	{
		TimeSinceRefresh = 0.0f;
		RefreshBoss();
	}
}

// 바인딩한 보스가 죽거나 사라졌으면 숨기고, 보스가 없으면 새로 찾는다
void ULeeBossHealthBarWidget::RefreshBoss()
{
	AActor* CurrentBoss = BoundBoss.Get();
	if (CurrentBoss)
	{
		if (!IsBossAlive(CurrentBoss))
		{
			SetBoss(nullptr);
		}
		return;
	}

	if (AActor* NewBoss = FindAliveBoss())
	{
		SetBoss(NewBoss);
	}
}

// "Boss" 태그가 붙은 액터 중 HP가 남아 있는 첫 번째 보스
AActor* ULeeBossHealthBarWidget::FindAliveBoss() const
{
	// 26.11.12 추가 - 보스전 관리자가 있는 맵에서는 입장(인트로/전투) 전까지 보스 바를 숨긴다
	if (const ALeeBossEncounterManager* Encounter = ALeeBossEncounterManager::Find(this))
	{
		if (!Encounter->IsBossRevealed())
		{
			return nullptr;
		}
	}

	TArray<AActor*> BossActors;
	UGameplayStatics::GetAllActorsWithTag(this, BossActorTag, BossActors);

	for (AActor* Boss : BossActors)
	{
		if (IsBossAlive(Boss))
		{
			return Boss;
		}
	}
	return nullptr;
}

// 두 바를 보스에 바인딩하고 이름을 채운 뒤 패널 표시. nullptr이면 해제 후 숨김
void ULeeBossHealthBarWidget::SetBoss(AActor* NewBoss)
{
	BoundBoss = NewBoss;

	if (Bar_BossHealth)
	{
		Bar_BossHealth->BindToActor(NewBoss);
	}
	if (Bar_BossStamina)
	{
		Bar_BossStamina->BindToActor(NewBoss);
	}

	if (Text_BossName)
	{
		const ALeeCharacter* BossCharacter = Cast<ALeeCharacter>(NewBoss);
		const FText BossName = BossCharacter ? BossCharacter->CharacterDisplayName : FText::GetEmpty();
		Text_BossName->SetText(BossName);
		// 이름을 지정하지 않은 보스는 빈 줄이 남지 않도록 이름 칸을 접는다
		Text_BossName->SetVisibility(BossName.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}

	if (Panel_BossBar)
	{
		Panel_BossBar->SetVisibility(NewBoss ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

// HealthComponent가 있고 HP가 0보다 크면 살아 있는 보스로 본다
bool ULeeBossHealthBarWidget::IsBossAlive(const AActor* Boss)
{
	if (!IsValid(Boss))
	{
		return false;
	}

	const ULeeHealthComponent* HealthComponent = ULeeHealthComponent::FindHealthComponent(Boss);
	return HealthComponent && HealthComponent->GetHealth() > 0.0f;
}
