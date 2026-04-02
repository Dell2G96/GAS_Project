// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AsyncMixin.h"
#include "Blueprint/UserWidgetPool.h"
#include "Widgets/SPanel.h"

class FActiveTimerHandle;
class FArrangedChildren;
class FChildren;
class FPaintArgs;
class FReferenceCollector;
class FSlateRect;
class FSlateWindowElementList;
class FWidgetStyle;
class UIndicatorDescriptor;
class ULeeIndicatorManagerComponent;
struct FSlateBrush;

/**
 * 액터 캔버스 Slate 위젯 클래스
 * FAsyncMixin, FGCObject 상속으로 비동기 처리와 GC 안전성 보장
 */
class SActorCanvas : public SPanel, public FAsyncMixin, public FGCObject
{
public:
    /** ActorCanvas 전용 슬롯 클래스 */
    class FSlot : public TSlotBase<FSlot>
    {
    public: 

       /** 인디케이터 디스크립터로 슬롯 생성 */
       FSlot(UIndicatorDescriptor* InIndicator)
          : TSlotBase<FSlot>()
          , Indicator(InIndicator)
          , ScreenPosition(FVector2D::ZeroVector)
          , Depth(0)
          , Priority(0.f)
          , bIsIndicatorVisible(true)
          , bInFrontOfCamera(true)
          , bHasValidScreenPosition(false)
          , bDirty(true)
          , bWasIndicatorClamped(false)
          , bWasIndicatorClampedStatusChanged(false)
       {
       }

       /** Slate 슬롯 인자 정의 시작 */
       SLATE_SLOT_BEGIN_ARGS(FSlot, TSlotBase<FSlot>)
       SLATE_SLOT_END_ARGS()
       using TSlotBase<FSlot>::Construct;

       /** 인디케이터 가시성 반환 */
       bool GetIsIndicatorVisible() const { return bIsIndicatorVisible; }
       /** 인디케이터 가시성 설정 (변경 시 Dirty 플래그 설정) */
       void SetIsIndicatorVisible(bool bVisible)
       {
          if (bIsIndicatorVisible != bVisible)
          {
             bIsIndicatorVisible = bVisible;
             bDirty = true;
          }

          RefreshVisibility();
       }

       /** 화면 위치 반환 */
       FVector2D GetScreenPosition() const { return ScreenPosition; }
       /** 화면 위치 설정 (변경 시 Dirty 플래그 설정) */
       void SetScreenPosition(FVector2D InScreenPosition)
       {
          if (ScreenPosition != InScreenPosition)
          {
             ScreenPosition = InScreenPosition;
             bDirty = true;
          }
       }

       /** 깊이(거리) 반환 */
       double GetDepth() const { return Depth; }
       /** 깊이 설정 (변경 시 Dirty 플래그 설정) */
       void SetDepth(double InDepth)
       {
          if (Depth != InDepth)
          {
             Depth = InDepth;
             bDirty = true;
          }
       }

       /** 우선순위 반환 */
       int32 GetPriority() const { return Priority; }
       /** 우선순위 설정 (변경 시 Dirty 플래그 설정) */
       void SetPriority(int32 InPriority)
       {
          if (Priority != InPriority)
          {
             Priority = InPriority;
             bDirty = true;
          }
       }

       /** 카메라 앞 여부 반환 */
       bool GetInFrontOfCamera() const { return bInFrontOfCamera; }
       /** 카메라 앞 여부 설정 (변경 시 Dirty 플래그 및 가시성 새로고침) */
       void SetInFrontOfCamera(bool bInFront)
       {
          if (bInFrontOfCamera != bInFront)
          {
             bInFrontOfCamera = bInFront;
             bDirty = true;
          }

          RefreshVisibility();
       }

       /** 유효한 화면 위치 여부 반환 */
       bool HasValidScreenPosition() const { return bHasValidScreenPosition; }
       /** 유효한 화면 위치 여부 설정 (변경 시 Dirty 플래그 및 가시성 새로고침) */
       void SetHasValidScreenPosition(bool bValidScreenPosition)
       {
          if (bHasValidScreenPosition != bValidScreenPosition)
          {
             bHasValidScreenPosition = bValidScreenPosition;
             bDirty = true;
          }

          RefreshVisibility();
       }

       /** 더티 상태 반환 */
       bool bIsDirty() const { return bDirty; }

       /** 더티 플래그 클리어 */
       void ClearDirtyFlag()
       {
          bDirty = false;
       }

       /** 이전 프레임에 인디케이터가 화면 클램핑되었는지 반환 */
       bool WasIndicatorClamped() const { return bWasIndicatorClamped; }
       /** 이전 프레임 클램핑 상태 설정 (const paint 중 캐시 업데이트용 mutable) */
       void SetWasIndicatorClamped(bool bWasClamped) const
       {
          if (bWasClamped != bWasIndicatorClamped)
          {
             bWasIndicatorClamped = bWasClamped;
             bWasIndicatorClampedStatusChanged = true;
          }
       }

       /** 클램핑 상태 변경 플래그 반환 */
       bool WasIndicatorClampedStatusChanged() const { return bWasIndicatorClampedStatusChanged; }
       /** 클램핑 상태 변경 플래그 클리어 */
       void ClearIndicatorClampedStatusChangedFlag()
       {
          bWasIndicatorClampedStatusChanged = false;
       }

    private:
       /** 가시성 새로고침 (유효성 + 화면 위치 확인 후 위젯 가시성 설정) */
       void RefreshVisibility()
       {
          const bool bIsVisible = bIsIndicatorVisible && bHasValidScreenPosition;
          GetWidget()->SetVisibility(bIsVisible ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed);
       }

       // SActorCanvas::AddReferencedObjects에서 GC 생존 보장
       UIndicatorDescriptor* Indicator;
       FVector2D ScreenPosition;
       double Depth;
       int32 Priority;

       uint8 bIsIndicatorVisible : 1;
       uint8 bInFrontOfCamera : 1;
       uint8 bHasValidScreenPosition : 1;
       uint8 bDirty : 1;
       
       /** 
        * 이전 프레임에 인디케이터가 시각적으로 화면 클램핑되었는지의 캐시 및 프레임 지연 값
        * const paint 작업 중 캐시되므로 semi-hacky한 mutable 구현
        */
       mutable uint8 bWasIndicatorClamped : 1;
       mutable uint8 bWasIndicatorClampedStatusChanged : 1;

       friend class SActorCanvas;
    };

    /** ActorCanvas 전용 화살표 슬롯 클래스 */
    class FArrowSlot : public TSlotBase<FArrowSlot>
    {
    };

    /** 이 Slate 위젯의 인자 시작 */
    SLATE_BEGIN_ARGS(SActorCanvas) {
       _Visibility = EVisibility::HitTestInvisible;
    }

       /** 이 위젯이 지원하는 슬롯이 있음을 나타냄 */
       SLATE_SLOT_ARGUMENT(SActorCanvas::FSlot, Slots)
    
    /** 항상 마지막에 위치 */
    SLATE_END_ARGS()

    /** 기본 생성자 - 자식 패널 초기화 */
    SActorCanvas()
       : CanvasChildren(this)
       , ArrowChildren(this)
       , AllChildren(this)
    {
       AllChildren.AddChildren(CanvasChildren);
       AllChildren.AddChildren(ArrowChildren);
    }

    /** 인자 및 로컬 플레이어 컨텍스트, 화살표 브러시로 생성 */
    void Construct(const FArguments& InArgs, const FLocalPlayerContext& InCtx, const FSlateBrush* ActorCanvasArrowBrush);

    // SWidget 인터페이스
    /** 자식 위젯들 배치 (레이아웃) */
    virtual void OnArrangeChildren( const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren ) const override;
    /** 원하는 크기 반환 (항상 0 → 부모가 크기 결정) */
    virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D::ZeroVector; }
    /** 자식 위젯들 반환 */
    virtual FChildren* GetChildren() override { return &AllChildren; }
    /** 그리기 (핵심 렌더링 로직) */
    virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const;
    // SWidget 끝

    /** 추가된 순서대로 그리기 여부 설정 (활성화 시 배치 최적화 비활성) */
    void SetDrawElementsInOrder(bool bInDrawElementsInOrder) { bDrawElementsInOrder = bInDrawElementsInOrder; }

    /** GC 시 참조자 이름 반환 */
    virtual FString GetReferencerName() const override;
    /** GC 참조 객체 추가 */
    virtual void AddReferencedObjects( FReferenceCollector& Collector ) override;
    
private:
    /** 인디케이터 추가 시 호출 */
    void OnIndicatorAdded(UIndicatorDescriptor* Indicator);
    /** 인디케이터 제거 시 호출 */
    void OnIndicatorRemoved(UIndicatorDescriptor* Indicator);

    /** 인디케이터 엔트리에 대한 슬롯 추가 */
    void AddIndicatorForEntry(UIndicatorDescriptor* Indicator);
    /** 인디케이터 엔트리에 대한 슬롯 제거 */
    void RemoveIndicatorForEntry(UIndicatorDescriptor* Indicator);

    /** 슬롯 인자 스코프드 객체 반환 타입 */
    using FScopedWidgetSlotArguments = TPanelChildren<FSlot>::FScopedWidgetSlotArguments;
    /** 액터 슬롯 추가 */
    FScopedWidgetSlotArguments AddActorSlot(UIndicatorDescriptor* Indicator);
    /** 슬롯 위젯 제거 및 인덱스 반환 */
    int32 RemoveActorSlot(const TSharedRef<SWidget>& SlotWidget);

    /** 인디케이터 표시 여부 설정 */
    void SetShowAnyIndicators(bool bIndicators);
    /** 캔버스 업데이트 타이머 콜백 */
    EActiveTimerReturnType UpdateCanvas(double InCurrentTime, float InDeltaTime);

    /** 오프셋과 크기 계산 헬퍼 함수 */
    void GetOffsetAndSize(const UIndicatorDescriptor* Indicator,
       FVector2D& OutSize, 
       FVector2D& OutOffset,
       FVector2D& OutPaddingMin,
       FVector2D& OutPaddingMax) const;

    /** 활성 타이머 업데이트 */
    void UpdateActiveTimer();

private:
    /** 모든 인디케이터 배열 */
    TArray<TObjectPtr<UIndicatorDescriptor>> AllIndicators;
    /** 비활성 인디케이터 배열 */
    TArray<UIndicatorDescriptor*> InactiveIndicators;
    
    /** 로컬 플레이어 컨텍스트 */
    FLocalPlayerContext LocalPlayerContext;
    /** 인디케이터 매니저 컴포넌트 약한 포인터 */
    TWeakObjectPtr<ULeeIndicatorManagerComponent> IndicatorComponentPtr;

    /** 캔버스 슬롯들 */
    TPanelChildren<FSlot> CanvasChildren;
    /** 화살표 슬롯들 (mutable) */
    mutable TPanelChildren<FArrowSlot> ArrowChildren;
    /** 모든 자식 슬롯 (캔버스 + 화살표) */
    FCombinedChildren AllChildren;

    /** 인디케이터 위젯 풀 */
    FUserWidgetPool IndicatorPool;

    /** 액터 캔버스 화살표 브러시 */
    const FSlateBrush* ActorCanvasArrowBrush = nullptr;

    /** 다음 화살표 인덱스 (mutable) */
    mutable int32 NextArrowIndex = 0;
    /** 마지막 업데이트 화살표 인덱스 (mutable) */
    mutable int32 ArrowIndexLastUpdate = 0;

    /** 추가된 순서대로 그리기 여부 (활성화 시 배치 최적화 비활성) */
    bool bDrawElementsInOrder = false;

    /** 인디케이터 표시 여부 */
    bool bShowAnyIndicators = false;

    /** 그리기 지오메트리 캐시 (mutable) */
    mutable TOptional<FGeometry> OptionalPaintGeometry;

    /** 캔버스 업데이트용 타이머 핸들 */
    TSharedPtr<FActiveTimerHandle> TickHandle;
};