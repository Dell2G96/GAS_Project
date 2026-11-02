// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeReticleWidgetBase.h"

// #include "GAS_Project/MyTags.h"
// #include "GAS_Project/ACharacter/LeeTargetLockComponent.h"
#include "GAS_Project/AInventory/LeeInventoryItemInstance.h"
#include "GAS_Project/AWeapons/LeeWeaponInstance.h"

ULeeReticleWidgetBase::ULeeReticleWidgetBase(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{

}

void ULeeReticleWidgetBase::InitializeFromWeapon(class ULeeWeaponInstance* InWeapon)
{
	WeaponInstance = InWeapon;
	InventoryInstance = nullptr;
	if (WeaponInstance)
	{
		InventoryInstance = Cast<ULeeInventoryItemInstance>(WeaponInstance->GetInstigator());
	}
	OnWeaponInitialized();
}

// 26.11.03 추가 - 빌드 검증 완료, 학습용으로 주석 처리

// // 락온 메시지 구독 + 이미 락온 중인 상태에서 무기를 바꿔 낀 경우를 위해 현재 상태 즉시 반영
// void ULeeReticleWidgetBase::NativeConstruct()
// {
// 	Super::NativeConstruct();
//
// 	DefaultVisibility = GetVisibility();
//
// 	TargetLockListenerHandle = UGameplayMessageSubsystem::Get(this).RegisterListener(
// 		MyTags::Souls::Message_TargetLock, this, &ThisClass::HandleTargetLockMessage);
//
// 	const ULeeTargetLockComponent* TargetLock = ULeeTargetLockComponent::FindTargetLockComponent(GetOwningPlayerPawn());
// 	ApplyTargetLockVisibility(TargetLock && TargetLock->IsLocked());
// }
//
// // 락온 메시지 구독 해제
// void ULeeReticleWidgetBase::NativeDestruct()
// {
// 	TargetLockListenerHandle.Unregister();
//
// 	Super::NativeDestruct();
// }
//
// // 리슨서버에서는 원격 클라의 락온 메시지도 들어오므로 내 Pawn이 보낸 메시지만 처리
// void ULeeReticleWidgetBase::HandleTargetLockMessage(FGameplayTag Channel, const FLeeTargetLockMessage& Message)
// {
// 	if (Message.Instigator != GetOwningPlayerPawn())
// 	{
// 		return;
// 	}
// 	ApplyTargetLockVisibility(Message.bLocked);
// }
//
// // 락온 중이면 Collapsed, 아니면 생성 시점의 원래 Visibility로 복귀
// void ULeeReticleWidgetBase::ApplyTargetLockVisibility(bool bLocked)
// {
// 	SetVisibility(bLocked ? ESlateVisibility::Collapsed : DefaultVisibility);
// }
