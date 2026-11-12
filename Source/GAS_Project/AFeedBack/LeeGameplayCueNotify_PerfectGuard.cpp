// 26.11.12 신규 - 퍼펙트 가드 피드백 GameplayCue (GameplayCue.Souls.Guard.Perfect)

#include "LeeGameplayCueNotify_PerfectGuard.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "GAS_Project/ACamera/LeeCameraShake_Bow.h"
#include "GAS_Project/MyTags.h"

// 생성자 — Cue 태그와 기본 카메라 흔들림 지정
ULeeGameplayCueNotify_PerfectGuard::ULeeGameplayCueNotify_PerfectGuard()
{
	GameplayCueTag = MyTags::Souls::GameplayCue_Guard_Perfect;
	CameraShake = ULeeCameraShake_PerfectGuard::StaticClass();
}

// 퍼펙트 가드 — 막은 지점에 이펙트·사운드, 막은 사람이 로컬 플레이어면 카메라 흔들림
bool ULeeGameplayCueNotify_PerfectGuard::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	if (!MyTarget)
	{
		return false;
	}

	const FVector GuardLocation = Parameters.Location.IsZero() ? MyTarget->GetActorLocation() : FVector(Parameters.Location);
	const FVector HitDirection = FVector(Parameters.Normal).GetSafeNormal(UE_SMALL_NUMBER, MyTarget->GetActorForwardVector());

	if (GuardSystem)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(MyTarget, GuardSystem, GuardLocation, (-HitDirection).Rotation(), GuardSystemScale);
	}

	if (GuardSound)
	{
		UGameplayStatics::PlaySoundAtLocation(MyTarget, GuardSound, GuardLocation, 1.0f, GuardSoundPitch);
	}

	const APawn* Pawn = Cast<APawn>(MyTarget);
	APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	if (CameraShake && PC && PC->IsLocalController())
	{
		PC->ClientStartCameraShake(CameraShake, CameraShakeScale);
	}
	return true;
}
