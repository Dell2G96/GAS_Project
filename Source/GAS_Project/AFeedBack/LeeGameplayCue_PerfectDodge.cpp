// Fill out your copyright notice in the Description page of Project Settings.

#include "LeeGameplayCue_PerfectDodge.h"

#include "Components/PoseableMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/LeeLogChannels.h"

// 생성자 — Cue 태그 설정 (Execute형 버스트 연출)
ALeeGameplayCue_PerfectDodge::ALeeGameplayCue_PerfectDodge()
{
	GameplayCueTag = MyTags::Souls::GameplayCue_Dodge_Perfect;
	bAutoDestroyOnRemove = false;
}

// Cue 실행 — 클라이언트에서만 잔상 샘플링 시작 (데디 서버는 연출 스킵)
bool ALeeGameplayCue_PerfectDodge::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& /*Parameters*/)
{
	// [임시 디버그] Cue 실행 진입 확인 — Target/NetMode
	UE_LOG(LogLee, Warning, TEXT("[Afterimage] OnExecute 진입: Target=%s NetMode=%d"),
		*GetNameSafe(MyTarget), (int32)GetNetMode());

	// 데디 서버에서는 로직 없음 — 연출은 클라이언트 전용
	if (GetNetMode() == NM_DedicatedServer)
	{
		UE_LOG(LogLee, Warning, TEXT("[Afterimage] 데디서버 → 연출 스킵"));
		return false;
	}

	const ACharacter* TargetCharacter = Cast<ACharacter>(MyTarget);
	USkeletalMeshComponent* TargetMesh = TargetCharacter ? TargetCharacter->GetMesh() : nullptr;
	if (!TargetMesh)
	{
		UE_LOG(LogLee, Warning, TEXT("[Afterimage] TargetMesh 없음(Target이 ACharacter가 아니거나 Mesh 없음) → 스킵"));
		return false;
	}

	// [임시 디버그] 슬롯/폴백/맵 구성 확인
	UE_LOG(LogLee, Warning, TEXT("[Afterimage] TargetMesh=%s NumMaterials=%d 폴백AfterimageMaterial=%s MapNum=%d"),
		*GetNameSafe(TargetMesh), TargetMesh->GetNumMaterials(), *GetNameSafe(AfterimageMaterial), AfterimageMaterialMap.Num());

	// 연속 실행 대비: 이전 잔상/타이머 정리 후 새로 시작
	ClearAfterimages();
	SourceMesh = TargetMesh;

	// 첫 샘플은 즉시, 이후 SampleInterval 간격으로 최대 MaxCount개
	SpawnAfterimageSample();
	GetWorld()->GetTimerManager().SetTimer(SampleTimerHandle, this, &ThisClass::SpawnAfterimageSample,
		AfterimageSampleInterval, /*bLoop*/true);

	// 페이드 갱신 타이머 (프레임 단위 부드러움이 필요하면 간격을 줄인다)
	GetWorld()->GetTimerManager().SetTimer(FadeTimerHandle, this, &ThisClass::UpdateAfterimageFade,
		0.02f, /*bLoop*/true);

	return true;
}

// 파괴 시 잔상/타이머 정리
void ALeeGameplayCue_PerfectDodge::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearAfterimages();
	Super::EndPlay(EndPlayReason);
}

// 타겟 메시의 현재 포즈를 PoseableMesh로 복사해 월드에 남긴다
void ALeeGameplayCue_PerfectDodge::SpawnAfterimageSample()
{
	USkeletalMeshComponent* Mesh = SourceMesh.Get();
	if (!Mesh || SpawnedSampleCount >= AfterimageMaxCount)
	{
		// 샘플링 종료 (페이드 타이머는 남은 잔상이 다 사라질 때까지 유지)
		GetWorld()->GetTimerManager().ClearTimer(SampleTimerHandle);
		return;
	}

	UPoseableMeshComponent* Sample = NewObject<UPoseableMeshComponent>(this);
	if (!Sample)
	{
		return;
	}

	Sample->RegisterComponent();
	Sample->SetSkinnedAssetAndUpdate(Mesh->GetSkeletalMeshAsset(), /*bReinitPose*/true);
	Sample->CopyPoseFromSkeletalComponent(Mesh);
	Sample->SetWorldTransform(Mesh->GetComponentTransform());
	Sample->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Sample->SetCastShadow(false);

	// 슬롯별 잔상 머티리얼로 교체 — 원본 슬롯 머티리얼을 키로 AfterimageMaterialMap에서 반투명 버전을 찾고,
	// 매핑에 없으면 AfterimageMaterial(폴백)을 사용. 슬롯마다 별도의 MID를 만들어 원본 텍스처를 유지한다.
	// 생성된 MID는 Sample(UMeshComponent)의 OverrideMaterials에 보관되어 GC로부터 안전하다 —
	// 별도 배열에 담지 않고 UpdateAfterimageFade에서 Sample->GetMaterial()로 다시 조회한다.
	for (int32 MaterialIndex = 0; MaterialIndex < Sample->GetNumMaterials(); ++MaterialIndex)
	{
		UMaterialInterface* OriginalSlotMaterial = Mesh->GetMaterial(MaterialIndex);

		UMaterialInterface* AfterimageSlotMaterial = AfterimageMaterial;
		if (OriginalSlotMaterial)
		{
			if (const TObjectPtr<UMaterialInterface>* MappedMaterial = AfterimageMaterialMap.Find(OriginalSlotMaterial))
			{
				AfterimageSlotMaterial = *MappedMaterial;
			}
		}

		// [임시 디버그] 각 슬롯이 어떤 머티리얼을 고르는지
		UE_LOG(LogLee, Warning, TEXT("[Afterimage] Slot %d: Orig=%s → Chosen=%s"),
			MaterialIndex, *GetNameSafe(OriginalSlotMaterial), *GetNameSafe(AfterimageSlotMaterial));

		if (AfterimageSlotMaterial)
		{
			UMaterialInstanceDynamic* SlotMID = UMaterialInstanceDynamic::Create(AfterimageSlotMaterial, this);
			SlotMID->SetVectorParameterValue(TintParamName, AfterimageTint);
			SlotMID->SetScalarParameterValue(OpacityParamName, 1.0f);
			Sample->SetMaterial(MaterialIndex, SlotMID);
		}
	}

	AfterimageSamples.Add(Sample);
	AfterimageSpawnTimes.Add(GetWorld()->GetTimeSeconds());
	++SpawnedSampleCount;
}

// 잔상 페이드 갱신 — 수명 비율로 불투명도 감소, 만료 시 파괴
void ALeeGameplayCue_PerfectDodge::UpdateAfterimageFade()
{
	const double Now = GetWorld()->GetTimeSeconds();

	for (int32 Index = AfterimageSamples.Num() - 1; Index >= 0; --Index)
	{
		const float Alpha = 1.0f - static_cast<float>((Now - AfterimageSpawnTimes[Index]) / AfterimageLifetime);

		UPoseableMeshComponent* Sample = AfterimageSamples[Index];

		if (Alpha <= 0.0f)
		{
			if (Sample)
			{
				Sample->DestroyComponent();
			}
			AfterimageSamples.RemoveAt(Index);
			AfterimageSpawnTimes.RemoveAt(Index);
			continue;
		}

		// 슬롯별로 배정된 MID를 다시 조회해 불투명도만 갱신 (미지정 슬롯은 원본 머티리얼이라 캐스트 실패 → 스킵)
		if (Sample)
		{
			for (int32 MaterialIndex = 0; MaterialIndex < Sample->GetNumMaterials(); ++MaterialIndex)
			{
				if (UMaterialInstanceDynamic* SlotMID = Cast<UMaterialInstanceDynamic>(Sample->GetMaterial(MaterialIndex)))
				{
					SlotMID->SetScalarParameterValue(OpacityParamName, Alpha);
				}
			}
		}
	}

	// 샘플링이 끝났고 잔상도 모두 사라졌으면 페이드 타이머 정지
	if (AfterimageSamples.IsEmpty() && SpawnedSampleCount >= AfterimageMaxCount)
	{
		GetWorld()->GetTimerManager().ClearTimer(FadeTimerHandle);
		SpawnedSampleCount = 0;
	}
}

// 잔상 컴포넌트/타이머 전부 정리
void ALeeGameplayCue_PerfectDodge::ClearAfterimages()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SampleTimerHandle);
		World->GetTimerManager().ClearTimer(FadeTimerHandle);
	}

	for (UPoseableMeshComponent* Sample : AfterimageSamples)
	{
		if (Sample)
		{
			Sample->DestroyComponent();
		}
	}
	AfterimageSamples.Reset();
	AfterimageSpawnTimes.Reset();
	SpawnedSampleCount = 0;
}
