#include "LeeGameplayCue_SandBlind.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GAS_Project/MyTags.h"

// 생성자 — Cue 태그 지정, 페이드용 Tick은 필요할 때만 켠다
ALeeGameplayCue_SandBlind::ALeeGameplayCue_SandBlind()
{
	GameplayCueTag = MyTags::Souls::GameplayCue_SandBlind;

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	// OnRemove 이후 페이드 아웃이 끝날 때까지 살아 있도록 지연 파괴 (실제 지연값은 OnRemove에서 FadeOutTime으로 갱신)
	bAutoDestroyOnRemove = true;
	AutoDestroyDelay = 1.1f;
}

// GE 활성(또는 늦게 relevant 됨) — 로컬 플레이어 화면에만 모래 머티리얼을 붙이고 페이드 인
bool ALeeGameplayCue_SandBlind::WhileActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	// 데디 서버에는 화면이 없다
	if (GetNetMode() == NM_DedicatedServer)
	{
		return false;
	}

	// Cue는 피격자가 보이는 모든 클라이언트에서 실행되므로, 피격자 본인 화면에서만 처리한다
	const APawn* TargetPawn = Cast<APawn>(MyTarget);
	if (!TargetPawn || !TargetPawn->IsLocallyControlled() || !SandMaterial)
	{
		return false;
	}

	UCameraComponent* Camera = TargetPawn->FindComponentByClass<UCameraComponent>();
	if (!Camera)
	{
		return false;
	}

	// 재피격 등으로 다시 불려도 머티리얼은 한 번만 붙인다
	if (!SandMID)
	{
		SandMID = UMaterialInstanceDynamic::Create(SandMaterial, this);
		SandMID->SetScalarParameterValue(AmountParamName, 0.f);
		Camera->AddOrUpdateBlendable(SandMID, 1.0f);
		TargetCamera = Camera;
		CurrentAmount = 0.f;
	}

	StartFade(1.f, FadeInTime);
	return true;
}

// GE 만료 — 페이드 아웃 시작. 끝나면 Tick에서 머티리얼을 제거한다
bool ALeeGameplayCue_SandBlind::OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	// 부모의 HandleGameplayCue가 OnRemove 직후 이 값으로 파괴 타이머를 건다 → 페이드가 끝난 뒤 정리되게
	AutoDestroyDelay = FadeOutTime + 0.1f;

	if (SandMID)
	{
		StartFade(0.f, FadeOutTime);
	}
	return true;
}

// 매 틱 — 현재 강도를 목표값으로 일정 속도로 이동, 0까지 걷히면 정리
void ALeeGameplayCue_SandBlind::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!SandMID)
	{
		SetActorTickEnabled(false);
		return;
	}

	CurrentAmount = FMath::FInterpConstantTo(CurrentAmount, TargetAmount, DeltaSeconds, FadeSpeed);
	SandMID->SetScalarParameterValue(AmountParamName, CurrentAmount);

	if (FMath::IsNearlyEqual(CurrentAmount, TargetAmount))
	{
		SetActorTickEnabled(false);

		if (TargetAmount <= 0.f)
		{
			ClearEffect();
		}
	}
}

// 파괴 시 머티리얼이 카메라에 남지 않도록 정리
void ALeeGameplayCue_SandBlind::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearEffect();
	Super::EndPlay(EndPlayReason);
}

// Cue 액터 풀링 재사용 전 정리 (풀링되면 EndPlay가 불리지 않는다)
bool ALeeGameplayCue_SandBlind::Recycle()
{
	ClearEffect();
	return Super::Recycle();
}

// 페이드 목표값을 정하고 Tick을 켠다
void ALeeGameplayCue_SandBlind::StartFade(float InTargetAmount, float InFadeTime)
{
	TargetAmount = InTargetAmount;
	FadeSpeed = 1.f / FMath::Max(InFadeTime, 0.01f);
	SetActorTickEnabled(true);
}

// 카메라에서 머티리얼을 떼고 상태 초기화
void ALeeGameplayCue_SandBlind::ClearEffect()
{
	if (UCameraComponent* Camera = TargetCamera.Get())
	{
		if (SandMID)
		{
			Camera->RemoveBlendable(SandMID);
		}
	}

	SandMID = nullptr;
	TargetCamera.Reset();
	CurrentAmount = 0.f;
	TargetAmount = 0.f;
	SetActorTickEnabled(false);
}
