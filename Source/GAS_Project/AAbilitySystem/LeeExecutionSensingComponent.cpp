// Fill out your copyright notice in the Description page of Project Settings.
//
// [DEPRECATED] 이 컴포넌트는 ULeeFinishInteractionComponent + ULeeFinishTargetComponent
// 쌍으로 대체되었습니다. 블루프린트 참조가 남아있을 수 있어 클래스 자체는 유지하되,
// 내부 로직은 비활성화합니다. 새 기능은 추가하지 마세요.

#include "LeeExecutionSensingComponent.h"

ULeeExecutionSensingComponent::ULeeExecutionSensingComponent()
{
	// 비활성화: 더 이상 틱하지 않음
	PrimaryComponentTick.bCanEverTick = false;
}

void ULeeExecutionSensingComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	// No-op: 신 시스템이 대체함
}
