// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Dataflow/DataflowContextObject.h"
#include "LeeCameraMode.generated.h"

struct FLeeCameraModeView
{
	FLeeCameraModeView();

	void Blend(const FLeeCameraModeView& Other, float OtherWeight);

	FVector Location;
	FRotator Rotation;
	FRotator ControlRotation;
	float FieldOfView;
};

UENUM()
enum class ELeeCameraModeBlendFunction : uint8
{
	Linear,
	EaseIn,
	EaseOut,
	EaseInOut,
	COUNT
};

UCLASS(Abstract, NotBlueprintable)
class GAS_PROJECT_API ULeeCameraMode : public UObject
{
	GENERATED_BODY()
public:
	ULeeCameraMode(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	void UpdateCameraMode(float DeltaTime);
	virtual void UpdateView(float DeltaTime);
	void UpdateBlending(float DeltaTime);

	class ULeeCameraComponent* GetLeeCameraComponent() const;
	AActor* GetTargetActor() const;
	FVector GetPivotLocation() const;
	FRotator GetPivotRotation() const;

	FLeeCameraModeView View;

	/** Camera Mode의 FOV */
	UPROPERTY(EditDefaultsOnly, Category = "View", Meta = (UIMin = "5.0", UIMax = "170", ClampMin = "5.0", Clampmax = "170.0"))
	float FieldOfView;

	/** View에 대한 Pitch [Min, Max] */
	UPROPERTY(EditDefaultsOnly, Category = "View", Meta = (UIMin = "-89.9", UIMax = "89.9", ClampMin = "-89.9", Clampmax = "89.9"))
	float ViewPitchMin;

	UPROPERTY(EditDefaultsOnly, Category = "View", Meta = (UIMin = "-89.9", UIMax = "89.9", ClampMin = "-89.9", Clampmax = "89.9"))
	float ViewPitchMax;

	UPROPERTY(EditDefaultsOnly, Category="Lee|Blending")
	float BlendTime;

	float BlendAlpha;

	float BlendWeight;

	UPROPERTY(EditDefaultsOnly, Category="Lee|Blending")
	float BlendExponent;

	ELeeCameraModeBlendFunction BlendFunction;
};

UCLASS()
class ULeeCameraModeStack : public UObject
{
	GENERATED_BODY()
public:
	ULeeCameraModeStack(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	ULeeCameraMode* GetCameraModeInstance(TSubclassOf<ULeeCameraMode>& CameraModeClass);

	void PushCameraMode(TSubclassOf<ULeeCameraMode>& CameraModeClass);
	void EvaluateStack(float DeltaTime, FLeeCameraModeView& OutCameraModeView);
	void UpdateStack(float DeltaTime);
	void BlendStack(FLeeCameraModeView& OutCameraModeView) const;

	
	UPROPERTY()
	TArray<TObjectPtr<ULeeCameraMode>> CameraModeInstances;

	UPROPERTY()
	TArray<TObjectPtr<ULeeCameraMode>> CameraModeStack;
};
























