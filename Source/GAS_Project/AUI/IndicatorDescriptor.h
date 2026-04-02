// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "IndicatorDescriptor.generated.h"


class SWidget;
class UIndicatorDescriptor;
class ULeeIndicatorManagerComponent;
class UUserWidget;
struct FFrame;
struct FSceneViewProjectionData;

struct FIndicatorProjection
{
	bool Project(const UIndicatorDescriptor& IndicatorDescriptor, const FSceneViewProjectionData& InProjectionData, const FVector2f& ScreenSize, FVector& OutScreenPositionWithDepth);	
};


UENUM(BlueprintType)
enum class EActorCanvasProjectionMode : uint8
{
	ComponentPoint,
	ComponentBoundingBox,
	ComponentScreenBoundingBox,
	ActorBoundingBox,
	ActorScreenBoundingBox
};

/**
 * 활성 인디케이터를 설명하고 제어합니다. 
 * 연관된 데이터에 '바인딩'할 수 있도록 위젯이 IActorIndicatorWidget 인터페이스를 구현하는 것이 강력히 권장됩니다.
 */
UCLASS(BlueprintType)
class GAS_PROJECT_API UIndicatorDescriptor : public UObject
{
	GENERATED_BODY()

public:
	UIndicatorDescriptor() {}

public:
	UFUNCTION(BlueprintCallable)
	UObject* GetDataObject() const { return DataObject; }

	UFUNCTION(BlueprintCallable)
	void SetDataObject(UObject* InDataObject) { DataObject = InDataObject; }
    
    UFUNCTION(BlueprintCallable)
    USceneComponent* GetSceneComponent() const { return Component; }
    /** 씬 컴포넌트를 반환합니다. */
    UFUNCTION(BlueprintCallable)
    void SetSceneComponent(USceneComponent* InComponent) { Component = InComponent; }
    /** 씬 컴포넌트를 설정합니다. */

    /** 컴포넌트 소켓 이름을 반환합니다. */
    UFUNCTION(BlueprintCallable)
    FName GetComponentSocketName() const { return ComponentSocketName; }
    /** 컴포넌트 소켓 이름을 설정합니다. */
    UFUNCTION(BlueprintCallable)
    void SetComponentSocketName(FName SocketName) { ComponentSocketName = SocketName; }

    /** 인디케이터 위젯 클래스를 반환합니다. */
    UFUNCTION(BlueprintCallable)
    TSoftClassPtr<UUserWidget> GetIndicatorClass() const { return IndicatorWidgetClass; }
    /** 인디케이터 위젯 클래스를 설정합니다. */
    UFUNCTION(BlueprintCallable)
    void SetIndicatorClass(TSoftClassPtr<UUserWidget> InIndicatorWidgetClass)
    {
       IndicatorWidgetClass = InIndicatorWidgetClass;
    }

public:
    // TODO: 이를 더 잘 정리해야 함.
    TWeakObjectPtr<UUserWidget> IndicatorWidget;

public:
    /** 인디케이터 컴포넌트가 nullptr일 때 자동으로 제거할지 설정합니다. */
    UFUNCTION(BlueprintCallable)
    void SetAutoRemoveWhenIndicatorComponentIsNull(bool CanAutomaticallyRemove)
    {
       bAutoRemoveWhenIndicatorComponentIsNull = CanAutomaticallyRemove;
    }
    /** 인디케이터 컴포넌트가 nullptr일 때 자동으로 제거할지 반환합니다. */
    UFUNCTION(BlueprintCallable)
    bool GetAutoRemoveWhenIndicatorComponentIsNull() const { return bAutoRemoveWhenIndicatorComponentIsNull; }

    /** 인디케이터 컴포넌트가 nullptr이고 자동 제거가 활성화된 경우 true를 반환합니다. */
    bool CanAutomaticallyRemove() const
    {
       return bAutoRemoveWhenIndicatorComponentIsNull && !IsValid(GetSceneComponent());
    }

public:
    // 레이아웃 속성
    //=======================

    /** 씬 컴포넌트가 유효하고 가시성이 true인 경우 true를 반환합니다. */
    UFUNCTION(BlueprintCallable)
    bool GetIsVisible() const { return IsValid(GetSceneComponent()) && bVisible; }
    
    /** 원하는 가시성을 설정합니다. */
    UFUNCTION(BlueprintCallable)
    void SetDesiredVisibility(bool InVisible)
    {
       bVisible = InVisible;
    }

    /** 프로젝션 모드를 반환합니다. */
    UFUNCTION(BlueprintCallable)
    EActorCanvasProjectionMode GetProjectionMode() const { return ProjectionMode; }
    /** 프로젝션 모드를 설정합니다. */
    UFUNCTION(BlueprintCallable)
    void SetProjectionMode(EActorCanvasProjectionMode InProjectionMode)
    {
       ProjectionMode = InProjectionMode;
    }

    // 인디케이터를 배치할 공간상의 점에 대한 수평 정렬 방식입니다.
    UFUNCTION(BlueprintCallable)
    EHorizontalAlignment GetHAlign() const { return HAlignment; }
    /** 수평 정렬 방식을 설정합니다. */
    UFUNCTION(BlueprintCallable)
    void SetHAlign(EHorizontalAlignment InHAlignment)
    {
       HAlignment = InHAlignment;
    }

    // 인디케이터를 배치할 공간상의 점에 대한 수직 정렬 방식입니다.
    /** 수직 정렬 방식을 반환합니다. */
    UFUNCTION(BlueprintCallable)
    EVerticalAlignment GetVAlign() const { return VAlignment; }
    /** 수직 정렬 방식을 설정합니다. */
    UFUNCTION(BlueprintCallable)
    void SetVAlign(EVerticalAlignment InVAlignment)
    {
       VAlignment = InVAlignment;
    }

    // 인디케이터를 화면 가장자리에 클램핑할지 여부입니다.
    /** 화면 클램핑 여부를 반환합니다. */
    UFUNCTION(BlueprintCallable)
    bool GetClampToScreen() const { return bClampToScreen; }
    /** 화면 클램핑 여부를 설정합니다. */
    UFUNCTION(BlueprintCallable)
    void SetClampToScreen(bool bValue)
    {
       bClampToScreen = bValue;
    }

    // 화면 가장자리에 클램핑할 때 화살표를 표시할지 여부입니다.
    /** 화면 클램핑 화살표 표시 여부를 반환합니다. */
    UFUNCTION(BlueprintCallable)
    bool GetShowClampToScreenArrow() const { return bShowClampToScreenArrow; }
    /** 화면 클램핑 화살표 표시 여부를 설정합니다. */
    UFUNCTION(BlueprintCallable)
    void SetShowClampToScreenArrow(bool bValue)
    {
       bShowClampToScreenArrow = bValue;
    }

    // 월드 공간에서의 인디케이터 위치 오프셋입니다.
    /** 월드 위치 오프셋을 반환합니다. */
    UFUNCTION(BlueprintCallable)
    FVector GetWorldPositionOffset() const { return WorldPositionOffset; }
    /** 월드 위치 오프셋을 설정합니다. */
    UFUNCTION(BlueprintCallable)
    void SetWorldPositionOffset(FVector Offset)
    {
       WorldPositionOffset = Offset;
    }

    // 화면 공간에서의 인디케이터 위치 오프셋입니다.
    /** 화면 공간 오프셋을 반환합니다. */
    UFUNCTION(BlueprintCallable)
    FVector2D GetScreenSpaceOffset() const { return ScreenSpaceOffset; }
    /** 화면 공간 오프셋을 설정합니다. */
    UFUNCTION(BlueprintCallable)
    void SetScreenSpaceOffset(FVector2D Offset)
    {
       ScreenSpaceOffset = Offset;
    }

    /** 바운딩 박스 앵커를 반환합니다. */
    UFUNCTION(BlueprintCallable)
    FVector GetBoundingBoxAnchor() const { return BoundingBoxAnchor; }
    /** 바운딩 박스 앵커를 설정합니다. */
    UFUNCTION(BlueprintCallable)
    void SetBoundingBoxAnchor(FVector InBoundingBoxAnchor)
    {
       BoundingBoxAnchor = InBoundingBoxAnchor;
    }

public:
    // 정렬 속성
    //=======================

    // (깊이별로 정렬된 후) 인디케이터들을 정렬할 수 있게 해주며, 일부 인디케이터 그룹이 항상 다른 그룹 앞에 오도록 합니다.
    /** 우선순위를 반환합니다. */
    UFUNCTION(BlueprintCallable)
    int32 GetPriority() const { return Priority; }
    /** 우선순위를 설정합니다. */
    UFUNCTION(BlueprintCallable)
    void SetPriority(int32 InPriority)
    {
       Priority = InPriority;
    }

public:
    ULeeIndicatorManagerComponent* GetIndicatorManagerComponent() { return ManagerPtr.Get(); }
    /** 인디케이터 매니저 컴포넌트를 설정합니다. */
	void SetIndicatorManagerComponent(ULeeIndicatorManagerComponent* InManager);
    
    /** 인디케이터를 등록 해제합니다. */
    UFUNCTION(BlueprintCallable)
	void UnregisterIndicator();


private:
	UPROPERTY()
	bool bVisible = true;
	UPROPERTY()
	bool bClampToScreen = false;
	UPROPERTY()
	bool bShowClampToScreenArrow = false;
	UPROPERTY()
	bool bOverrideScreenPosition = false;
	UPROPERTY()
	bool bAutoRemoveWhenIndicatorComponentIsNull = false;

	UPROPERTY()
	EActorCanvasProjectionMode ProjectionMode = EActorCanvasProjectionMode::ComponentPoint;

	UPROPERTY()
	TEnumAsByte<EHorizontalAlignment> HAlignment = HAlign_Center;

	UPROPERTY()
	TEnumAsByte<EVerticalAlignment> VAlignment = VAlign_Center;

	UPROPERTY()
	int32 Priority = 0;

	UPROPERTY()
	FVector BoundingBoxAnchor = FVector(0.5, 0.5, 0.5);
	UPROPERTY()
	FVector2D ScreenSpaceOffset = FVector2D(0,0);
	UPROPERTY()
	FVector WorldPositionOffset = FVector(0,0,0);

private:
	friend class SActorCanvas;

	UPROPERTY()
	TObjectPtr<UObject> DataObject;
	
	UPROPERTY()
	TObjectPtr<USceneComponent> Component;
	
	UPROPERTY()
	FName ComponentSocketName = NAME_None;
	
	UPROPERTY()
	TSoftClassPtr<UUserWidget> IndicatorWidgetClass;
	
	UPROPERTY()
	TWeakObjectPtr<class ULeeIndicatorManagerComponent> ManagerPtr;

	
	TWeakPtr<SWidget> Content;
	TWeakPtr<SWidget> CanvasHost;
	
	
};
























