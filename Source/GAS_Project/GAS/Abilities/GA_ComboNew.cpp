// Fill out your copyright notice in the Description page of Project Settings.


#include "GA_ComboNew.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/Characters/Player/CPlayerCharacter.h"
#include "GAS_Project/Components/CWeaponComponent.h"
#include "GAS_Project/Item//Weapon/CWeapon.h"


class ACPlayerCharacter;

void UGA_ComboNew::SendHitReacEventToActors(const TArray<class AActor*>& HitActors)
{
	for (AActor* HitActor : HitActors)
	{
		FGameplayEventData Payload;
		Payload.Instigator = GetAvatarActorFromActorInfo();
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(HitActor, MyTags::Events::Enemy::HitReact,Payload);
	}
}

TArray<AActor*> UGA_ComboNew::HitBoxTrace()
{
    TArray<AActor*> OutActors;

    UWorld* World = GetWorld();
    if (!World) return OutActors;

    // 아바타 / 캐릭터 캐스팅
    AActor* AvatarActor = GetAvatarActorFromActorInfo();
    if (!AvatarActor) return OutActors;

    ACPlayerCharacter* OwnerCharacter = Cast<ACPlayerCharacter>(AvatarActor);
    if (!OwnerCharacter)  return OutActors;

    // 무기 컴포넌트
    UCWeaponComponent* WeaponComp = OwnerCharacter->FindComponentByClass<UCWeaponComponent>();
    if (!WeaponComp) return OutActors;

    // 현재 장착 무기
    ACWeapon* Weapon = WeaponComp->GetCharacterCurrentEquippedWeapon();
    if (!Weapon)  return OutActors;

    // 무기 메쉬
    USkeletalMeshComponent* WeaponMesh = Weapon->GetWeaponMesh();
    if (!WeaponMesh)  return OutActors;

    // 소켓 존재 여부 체크
    if (!WeaponMesh->DoesSocketExist(StartSocket) ||
        !WeaponMesh->DoesSocketExist(EndSocket)) return OutActors;

    const FVector Start = WeaponMesh->GetSocketLocation(StartSocket);
    const FVector End   = WeaponMesh->GetSocketLocation(EndSocket);

    // 충돌 파라미터 설정
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ComboHitBoxTrace), false, AvatarActor);
    // 자기 자신 무시
    QueryParams.AddIgnoredActor(AvatarActor);

    FCollisionResponseParams ResponseParams;
    ResponseParams.CollisionResponse.SetAllChannels(ECR_Ignore);
    ResponseParams.CollisionResponse.SetResponse(ECC_Pawn, ECR_Block);

    const FCollisionShape Sphere = FCollisionShape::MakeSphere(HitBoxRadius);

    TArray<FHitResult> HitResults;

    const bool bHit = World->SweepMultiByChannel(
        HitResults,
        Start,					// 시작 소켓 위치
        End,					// 끝 소켓 위치
        FQuat::Identity,
        ECC_Visibility,        // 전용 채널 있으면 그걸로 교체 추천
        Sphere,					
        QueryParams,			
        ResponseParams
    );
    if (bShouldDrawDebug)
    {
	    DrawDebugHitTrace(HitResults, (Start + End) * 0.5f);
    }

    if (!bHit)
    {
    	UE_LOG(LogTemp, Warning, TEXT("No Hit Detected in HitBoxTrace"));
	    return OutActors;
    }

    // 결과 정리
    OutActors.Reserve(HitResults.Num());

    for (const FHitResult& Result : HitResults)
    {
        AActor* HitActor = Result.GetActor();
        if (!IsValid(HitActor))
        {
            continue;
        }

        // 혹시라도 자기 자신이 들어오면 필터
        if (HitActor == AvatarActor)
        {
            continue;
        }

        OutActors.AddUnique(HitActor);
    }

    return OutActors;
}

void UGA_ComboNew::HitScanStart()
{
	UWorld* World = GetWorld();
	if (!World) return;

	// 이미 돌고 있으면 초기화
	if (World->GetTimerManager().IsTimerActive(HitBoxTraceTimerHandle))
	{
		World->GetTimerManager().ClearTimer(HitBoxTraceTimerHandle);
	}

	// 0.02초(50fps) 간격으로 계속 트레이스
	const float TraceInterval = 0.02f;

	World->GetTimerManager().SetTimer(
		HitBoxTraceTimerHandle,
		this,
		&UGA_ComboNew::HitScanTick,
		TraceInterval,
		true  // 반복
	);
}

void UGA_ComboNew::HitScanEnd()
{
	UWorld* World = GetWorld();
	if (!World) return;

	World->GetTimerManager().ClearTimer(HitBoxTraceTimerHandle);
}

void UGA_ComboNew::HitScanTick()
{
	// 실제 물리 트레이스
	TArray<AActor*> HitActors = HitBoxTrace();

	// 맞은 액터에게 HitReact 이벤트 전송 (원하면 여기서 데미지도)
	if (HitActors.Num() > 0)
	{
		SendHitReacEventToActors(HitActors);

		// TODO: GAS 데미지 GE 적용도 여기서 같이 수행 가능
	}
}


void UGA_ComboNew::DrawDebugHitTrace(const TArray<FHitResult>& Hits, const FVector& HitBoxLocation) const
{
	DrawDebugSphere(GetWorld(), HitBoxLocation, HitBoxRadius, 16, FColor::Red, false, 3.f);

	for (const FHitResult& Result : Hits)
	{
		if (IsValid(Result.GetActor()))
		{
			FVector DebugLocation = Result.GetActor()->GetActorLocation();
			DebugLocation.Z += 100.f;
			DrawDebugSphere(GetWorld(), DebugLocation, 30.f, 10, FColor::Green, false, 3.f);
		}
	}
}
