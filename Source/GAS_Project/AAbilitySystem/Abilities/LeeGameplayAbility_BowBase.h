// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LeeGameplayAbility.h"
#include "LeeGameplayAbility_BowBase.generated.h"

class ALeeArrowProjectile;
class UGameplayEffect;
class UAnimMontage;

/** 화살 한 발의 세기 — 데미지, 날아가는 속도(cm/s), 사거리(cm) */
USTRUCT(BlueprintType)
struct FLeeBowShotParams
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Bow", meta = (ClampMin = "0.0"))
	float Damage = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Bow", meta = (ClampMin = "100.0", ForceUnits = "cm/s"))
	float Speed = 2500.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Bow", meta = (ClampMin = "100.0", ForceUnits = "cm"))
	float Range = 2000.0f;
};

/**
 * 활 어빌리티 공용 베이스 (조준 BowAim, 발사 BowFire).
 * 손 소켓에 화살 스폰, 카메라 조준 방향 계산, 데미지 Spec을 실은 화살 발사, 활 시위 당김 표시를 맡는다.
 * 화살 스폰·발사는 서버에서만 한다 (화살은 리플리케이트 액터).
 */
UCLASS(Abstract)
class GAS_PROJECT_API ULeeGameplayAbility_BowBase : public ULeeGameplayAbility
{
	GENERATED_BODY()

public:
	ULeeGameplayAbility_BowBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	/** 쏘는 화살 클래스 */
	UPROPERTY(EditDefaultsOnly, Category = "Lee|Bow")
	TSubclassOf<ALeeArrowProjectile> ArrowClass;

	/** 화살을 붙이고 쏘는 캐릭터 메시 소켓 */
	UPROPERTY(EditDefaultsOnly, Category = "Lee|Bow")
	FName ArrowSocketName = TEXT("ArrowSpawn");

	/** 명중 시 적용할 데미지 GE (SetByCaller Damage + ExecCalc_Damage 경로, 예: GE_Melee_Damage) */
	UPROPERTY(EditDefaultsOnly, Category = "Lee|Bow")
	TSubclassOf<UGameplayEffect> DamageEffect;

	/** 쏘는 순간 재생할 몽타주 (반동) */
	UPROPERTY(EditDefaultsOnly, Category = "Lee|Bow")
	TObjectPtr<UAnimMontage> FireMontage;

	/** 활 액터(B_Bow)의 시위 당김 bool 변수 이름 — 같은 이름의 bool이 있는 부착 액터에만 적용 */
	UPROPERTY(EditDefaultsOnly, Category = "Lee|Bow")
	FName BowStretchPropertyName = TEXT("IsStretched");

	/** [서버] 손 소켓에 화살을 붙여 스폰한다 (아직 날지 않음) */
	ALeeArrowProjectile* SpawnArrowInHand() const;

	/** [서버] 데미지 Spec을 실어 화살을 카메라 조준 방향으로 날린다 */
	void LaunchArrow(ALeeArrowProjectile* Arrow, const FLeeBowShotParams& Shot) const;

	/** 카메라가 보는 지점을 향하는 발사 방향 (From = 화살 위치) */
	FVector GetAimDirection(const FVector& From, float Range) const;

	/** 활 시위 당김 애니 표시 켜기/끄기 (B_Bow의 IsStretched) */
	void SetBowStretched(bool bStretched) const;

	/** 서버(권위)에서 실행 중인지 */
	bool IsServerSide() const;

	/** 이 어빌리티를 조작하는 플레이어 화면(로컬)에서 실행 중인지 */
	bool IsLocalSide() const;

	/** 활을 쏘는 동안 캐릭터가 이동 방향 대신 카메라 방향을 보게 하고(true), 끝나면 되돌린다(false).
	 *  락온 중이면 락온이 회전을 맡으므로 건드리지 않는다. 조준·속사가 함께 쓴다 */
	void SetFaceCameraRotation(bool bFaceCamera);

private:
	/** 이 어빌리티가 bOrientRotationToMovement를 꺼 두었는지 (되돌릴 책임이 있는지) */
	bool bChangedOrientRotation = false;

	/** 바꾸기 전의 bOrientRotationToMovement 값 (되돌릴 값) */
	bool bOriginalOrientRotation = true;
};
