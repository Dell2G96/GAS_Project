#include "LeeAnimNotifyState_WeaponTelegraph.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/MeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/AFeedBack/LeeGameplayCue_WeaponTelegraph.h"
#include "GAS_Project/AEquipment/LeeEquipmentMeshSelection.h"

ULeeAnimNotifyState_WeaponTelegraph::ULeeAnimNotifyState_WeaponTelegraph()
{
#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(50, 150, 255, 255);
#endif
}

void ULeeAnimNotifyState_WeaponTelegraph::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (!MeshComp)
	{
		return;
	}

	FTelegraphRuntimeData& Data = RuntimeMap.FindOrAdd(MeshComp);
	AActor* Owner = MeshComp->GetOwner();

	// 1. 서버에서는 ASC에 GameplayCue 활성 (네트워크 복제 지원)
	const FGameplayTag CueTag = ResolveCueTag();
	const UWorld* World = MeshComp->GetWorld();
	UAbilitySystemComponent* OwnerASC = Owner ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner) : nullptr;
	const bool bCueOwnsVisuals = World && World->IsGameWorld() && OwnerASC && CueTag.IsValid();
	if (bCueOwnsVisuals && Owner->HasAuthority())
	{
		if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner))
		{
			FGameplayCueParameters Parameters;
			Parameters.SourceObject = this;
			ASC->AddGameplayCue(CueTag, Parameters);
			Data.ActiveCueTag = CueTag;
		}
	}

	// 1-1. [타격별 공격 속성] 서버에서 공격자 ASC에 DamageType Loose Tag 부여 → GA_AttackMelee가 히트 순간 읽어 판정에 반영
	const FGameplayTag DamageTypeTag = ResolveDamageTypeTag();
	if (Owner && Owner->HasAuthority() && DamageTypeTag.IsValid())
	{
		if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner))
		{
			ASC->AddLooseGameplayTag(DamageTypeTag);
			Data.ActiveDamageTypeTag = DamageTypeTag;
		}
	}

	// 발광 시작 — GlowDuration 경과 시 NotifyTick이 발광만 끈다
	Data.ElapsedTime = 0.0f;
	Data.bGlowActive = true;

	// 게임에서는 Cue가 시각효과를 소유하여 노티파이와 이중으로 적용하지 않는다.
	if (!World || World->GetNetMode() == NM_DedicatedServer || bCueOwnsVisuals) return;
	TArray<FLeeResolvedTelegraphTarget> Targets;
	if (!EquipmentTargets.IsEmpty())
	{
		LeeWeaponTelegraphVisuals::Resolve(Owner, EquipmentTargets, Targets);
	}
	else
	{
		UMeshComponent* TargetMesh = Owner ? ALeeGameplayCue_WeaponTelegraph::FindWeaponMeshOnActor(Owner, AttachSocketName, TEXT("WeaponTip")) : nullptr;
		if (!TargetMesh)
		{
			const TArray<UMeshComponent*> PreviewMeshes = LeeEquipmentMeshSelection::FindPreviewMeshes(Owner);
			auto* MatchingMesh = PreviewMeshes.FindByPredicate([this](const UMeshComponent* Mesh)
			{
				return Mesh->DoesSocketExist(AttachSocketName);
			});
			TargetMesh = MatchingMesh ? *MatchingMesh : (PreviewMeshes.IsEmpty() ? nullptr : PreviewMeshes[0]);
		}
		if (!TargetMesh) TargetMesh = MeshComp;
		FName Socket = AttachSocketName;
		if (!TargetMesh->DoesSocketExist(Socket)) Socket = TargetMesh->DoesSocketExist(TEXT("hand_r")) ? FName(TEXT("hand_r")) : NAME_None;
		Targets.Add({TargetMesh, Socket});
	}
	LeeWeaponTelegraphVisuals::Apply(Targets, ResolveOverlayMaterial(), ResolveNiagaraSystem(), Data.Visuals, MeshComp);
}
void ULeeAnimNotifyState_WeaponTelegraph::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (!MeshComp)
	{
		return;
	}

	FTelegraphRuntimeData Data;
	if (!RuntimeMap.RemoveAndCopyValue(MeshComp, Data))
	{
		return;
	}

	// 1. 아직 켜져 있는 발광 정리 (GlowDuration 안에 구간이 끝난 경우)
	if (Data.bGlowActive)
	{
		StopGlow(MeshComp, Data);
	}

	// 2. [타격별 공격 속성] 서버 Loose Tag 제거 — GA EndAbility가 먼저 정리했으면 카운트가 0이므로 건너뜀
	AActor* Owner = MeshComp->GetOwner();
	if (Owner && Owner->HasAuthority() && Data.ActiveDamageTypeTag.IsValid())
	{
		UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner);
		if (ASC && ASC->GetTagCount(Data.ActiveDamageTypeTag) > 0)
		{
			ASC->RemoveLooseGameplayTag(Data.ActiveDamageTypeTag);
		}
	}
}

// 매 틱 — 구간 시작 후 GlowDuration초가 지나면 발광만 끈다 (속성 태그는 NotifyEnd까지 유지)
void ULeeAnimNotifyState_WeaponTelegraph::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	if (!MeshComp || GlowDuration <= 0.0f)
	{
		return;
	}

	FTelegraphRuntimeData* Data = RuntimeMap.Find(MeshComp);
	if (!Data || !Data->bGlowActive)
	{
		return;
	}

	Data->ElapsedTime += FrameDeltaTime;
	if (Data->ElapsedTime >= GlowDuration)
	{
		StopGlow(MeshComp, *Data);
	}
}

// 발광만 끈다 — 서버 Cue 제거, 오버레이 복원, 나이아가라 제거 후 bGlowActive=false
void ULeeAnimNotifyState_WeaponTelegraph::StopGlow(USkeletalMeshComponent* MeshComp, FTelegraphRuntimeData& Data) const
{
	// 1. 서버 GameplayCue 제거
	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (Owner && Owner->HasAuthority() && Data.ActiveCueTag.IsValid())
	{
		if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner))
		{
			ASC->RemoveGameplayCue(Data.ActiveCueTag);
		}
	}

	LeeWeaponTelegraphVisuals::Clear(Data.Visuals);
	Data.ActiveCueTag = FGameplayTag();


	Data.bGlowActive = false;
}

// TelegraphType → 데미지 판정용 DamageType 태그 (파랑=퍼펙트 가드 전용, 빨강=퍼펙트 회피 전용/가드 불가)
FGameplayTag ULeeAnimNotifyState_WeaponTelegraph::ResolveDamageTypeTag() const
{
	switch (TelegraphType)
	{
	case ELeeWeaponTelegraphType::PerfectGuardOnly:
		return MyTags::Souls::DamageType_Attack_PerfectGuardOnly;
	case ELeeWeaponTelegraphType::Unblockable:
		return MyTags::Souls::DamageType_Attack_Unblockable;
	default:
		return FGameplayTag();
	}
}

FString ULeeAnimNotifyState_WeaponTelegraph::GetNotifyName_Implementation() const
{
	switch (TelegraphType)
	{
	case ELeeWeaponTelegraphType::PerfectGuardOnly:
		return TEXT("Weapon Telegraph [Blue: Perfect Guard]");
	case ELeeWeaponTelegraphType::Unblockable:
		return TEXT("Weapon Telegraph [Red: Perfect Dodge]");
	default:
		return CustomCueTag.IsValid() ? CustomCueTag.ToString() : TEXT("Weapon Telegraph [Custom]");
	}
}

FGameplayTag ULeeAnimNotifyState_WeaponTelegraph::ResolveCueTag() const
{
	if (CustomCueTag.IsValid())
	{
		return CustomCueTag;
	}

	switch (TelegraphType)
	{
	case ELeeWeaponTelegraphType::PerfectGuardOnly:
		return MyTags::Souls::GameplayCue_Attack_PerfectGuardOnly;
	case ELeeWeaponTelegraphType::Unblockable:
		return MyTags::Souls::GameplayCue_Attack_Unblockable;
	default:
		return FGameplayTag();
	}
}

UMaterialInterface* ULeeAnimNotifyState_WeaponTelegraph::ResolveOverlayMaterial() const
{
	if (OverrideOverlayMaterial)
	{
		return OverrideOverlayMaterial;
	}

	const TCHAR* DefaultPath = (TelegraphType == ELeeWeaponTelegraphType::Unblockable)
		? TEXT("/Game/VFX/WeaponTelegraph/MI_WeaponTelegraph_Red.MI_WeaponTelegraph_Red")
		: TEXT("/Game/VFX/WeaponTelegraph/MI_WeaponTelegraph_Blue.MI_WeaponTelegraph_Blue");

	return Cast<UMaterialInterface>(StaticLoadObject(UMaterialInterface::StaticClass(), nullptr, DefaultPath));
}

UNiagaraSystem* ULeeAnimNotifyState_WeaponTelegraph::ResolveNiagaraSystem() const
{
	if (OverrideNiagaraSystem)
	{
		return OverrideNiagaraSystem;
	}

	const TCHAR* DefaultPath = (TelegraphType == ELeeWeaponTelegraphType::Unblockable)
		? TEXT("/Game/VFX/WeaponTelegraph/NS_WeaponTelegraph_Red.NS_WeaponTelegraph_Red")
		: TEXT("/Game/VFX/WeaponTelegraph/NS_WeaponTelegraph_Blue.NS_WeaponTelegraph_Blue");

	return Cast<UNiagaraSystem>(StaticLoadObject(UNiagaraSystem::StaticClass(), nullptr, DefaultPath));
}

