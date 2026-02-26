// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraComponent.h"
#include "LeeCameraComponent.generated.h"


// 템플릿 전방 선언
template<class TClass> class TSubclassOf;

// (리턴타입, 델리게이트 이름)
DECLARE_DELEGATE_RetVal(TSubclassOf<class ULeeCameraMode>, FLeeCameraModeDelegate);
	

UCLASS()
class GAS_PROJECT_API ULeeCameraComponent : public UCameraComponent
{
	GENERATED_BODY()
public:
	ULeeCameraComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	static ULeeCameraComponent* FindCameraComponent(const AActor* Actor) { return (Actor ? Actor->FindComponentByClass<ULeeCameraComponent>() : nullptr); }

	AActor* GetTargetActor() const { return GetOwner(); }
	void UpdateCameraModes();

	virtual void OnRegister() override;
	virtual void GetCameraView(float DeltaTime, FMinimalViewInfo& DesiredView) final;


	// 카메라 Blending 기능을 지원 하는 스택
	UPROPERTY()
	TObjectPtr<class ULeeCameraModeStack> CameraModeStack;


	// 현재 카메라모드를 가져오는 델리게이트
	FLeeCameraModeDelegate DetermineCameraModeDelegate;
	
};
