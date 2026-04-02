// Fill out your copyright notice in the Description page of Project Settings.


#include "IndicatorDescriptor.h"

#include "GAS_Project/System/LeeIndicatorManagerComponent.h"

bool FIndicatorProjection::Project(const UIndicatorDescriptor& IndicatorDescriptor,
                                   const FSceneViewProjectionData& InProjectionData, const FVector2f& ScreenSize, FVector& OutScreenPositionWithDepth)
{
    // 인디케이터 디스크립터에서 씬 컴포넌트 가져오기
    if (USceneComponent* Component = IndicatorDescriptor.GetSceneComponent())
    {
        // 소켓 이름이 있으면 소켓 위치, 없으면 컴포넌트 위치 반환
        TOptional<FVector> WorldLocation;
        if (IndicatorDescriptor.GetComponentSocketName() != NAME_None)
        {
            // 지정된 소켓의 월드 변환에서 위치 추출
            WorldLocation = Component->GetSocketTransform(IndicatorDescriptor.GetComponentSocketName()).GetLocation();
        }
        else
        {
            // 기본 컴포넌트 위치 사용
            WorldLocation = Component->GetComponentLocation();
        }

        // 월드 오프셋 적용하여 최종 프로젝션 위치 계산
        const FVector ProjectWorldLocation = WorldLocation.GetValue() + IndicatorDescriptor.GetWorldPositionOffset();
        // 프로젝션 모드 가져오기
        const EActorCanvasProjectionMode ProjectionMode = IndicatorDescriptor.GetProjectionMode();
        
        // 프로젝션 모드별로 분기 처리
        switch (ProjectionMode)
        {
            // 컴포넌트 한 점을 화면에 투영
            case EActorCanvasProjectionMode::ComponentPoint:
            {
                // 월드 위치가 유효한지 확인
                if (WorldLocation.IsSet())
                {
                    // 월드 위치를 화면 좌표로 변환하고 카메라 앞뒤 여부 확인
                    FVector2D OutScreenSpacePosition;
                    const bool bInFrontOfCamera = ULocalPlayer::GetPixelPoint(InProjectionData, ProjectWorldLocation, OutScreenSpacePosition, &ScreenSize);

                    // 수평 오프셋 적용 (카메라 뒤면 방향 반전)
                    OutScreenSpacePosition.X += IndicatorDescriptor.GetScreenSpaceOffset().X * (bInFrontOfCamera ? 1 : -1);
                    // 수직 오프셋 적용
                    OutScreenSpacePosition.Y += IndicatorDescriptor.GetScreenSpaceOffset().Y;

                    // 카메라 뒤에 있으면서 화면 내부에 위치한 경우 화면 반대편 가장자리로 이동
                    if (!bInFrontOfCamera && FBox2f(FVector2f::Zero(), ScreenSize).IsInside((FVector2f)OutScreenSpacePosition))
                    {
                        // 화면 중앙에서 위치까지의 방향 벡터 정규화
                        const FVector2f CenterToPosition = (FVector2f(OutScreenSpacePosition) - (ScreenSize / 2)).GetSafeNormal();
                        // 반대편 가장자리 위치 계산
                        OutScreenSpacePosition = FVector2D((ScreenSize / 2) + CenterToPosition * ScreenSize);
                    }

                    // 출력: (화면X, 화면Y, 카메라-월드 거리)
                    OutScreenPositionWithDepth = FVector(OutScreenSpacePosition.X, OutScreenSpacePosition.Y, FVector::Dist(InProjectionData.ViewOrigin, ProjectWorldLocation));

                    return true;
                }

                return false;
            }
            // 컴포넌트 또는 액터의 바운딩 박스를 화면에 투영하여 좌표 범위 구함
            case EActorCanvasProjectionMode::ComponentScreenBoundingBox:
            case EActorCanvasProjectionMode::ActorScreenBoundingBox:
            {
                // 바운딩 박스 가져오기
                FBox IndicatorBox;
                if (ProjectionMode == EActorCanvasProjectionMode::ActorScreenBoundingBox)
                {
                    // 액터 전체 컴포넌트 바운딩 박스
                    IndicatorBox = Component->GetOwner()->GetComponentsBoundingBox();
                }
                else
                {
                    // 개별 컴포넌트 바운딩 박스
                    IndicatorBox = Component->Bounds.GetBox();
                }

                // 바운딩 박스의 화면 좌표 범위 계산 (좌하단 LL, 우상단 UR)
                FVector2D LL, UR;
                const bool bInFrontOfCamera = ULocalPlayer::GetPixelBoundingBox(InProjectionData, IndicatorBox, LL, UR, &ScreenSize);
            
                // 바운딩 박스 앵커 포인트와 화면 오프셋 가져오기
                const FVector& BoundingBoxAnchor = IndicatorDescriptor.GetBoundingBoxAnchor();
                const FVector2D& ScreenSpaceOffset = IndicatorDescriptor.GetScreenSpaceOffset();

                // 앵커 위치 계산 (0~1 범위에서 LL-UR 보간)
                FVector ScreenPositionWithDepth;
                ScreenPositionWithDepth.X = FMath::Lerp(LL.X, UR.X, BoundingBoxAnchor.X) + ScreenSpaceOffset.X * (bInFrontOfCamera ? 1 : -1);
                ScreenPositionWithDepth.Y = FMath::Lerp(LL.Y, UR.Y, BoundingBoxAnchor.Y) + ScreenSpaceOffset.Y;
                // 깊이는 월드 위치 기준으로 계산
                ScreenPositionWithDepth.Z = FVector::Dist(InProjectionData.ViewOrigin, ProjectWorldLocation);

                // 카메라 뒤에 있으면서 화면 내부에 위치한 경우 화면 반대편 가장자리로 이동
                const FVector2f ScreenSpacePosition = FVector2f(FVector2D(ScreenPositionWithDepth));
                if (!bInFrontOfCamera && FBox2f(FVector2f::Zero(), ScreenSize).IsInside(ScreenSpacePosition))
                {
                    // 화면 중앙에서 위치까지의 방향 벡터 정규화
                    const FVector2f CenterToPosition = (ScreenSpacePosition - (ScreenSize / 2)).GetSafeNormal();
                    // 반대편 가장자리 위치 계산
                    const FVector2f ScreenPositionFromBehind = (ScreenSize / 2) + CenterToPosition * ScreenSize;
                    ScreenPositionWithDepth.X = ScreenPositionFromBehind.X;
                    ScreenPositionWithDepth.Y = ScreenPositionFromBehind.Y;
                }
             
                // 최종 출력 위치 설정
                OutScreenPositionWithDepth = ScreenPositionWithDepth;
                return true;
            }
            // 바운딩 박스 특정 지점을 월드 좌표로 계산 후 ComponentPoint처럼 처리
            case EActorCanvasProjectionMode::ActorBoundingBox:
            case EActorCanvasProjectionMode::ComponentBoundingBox:
            {
                // 바운딩 박스 가져오기
                FBox IndicatorBox;
                if (ProjectionMode == EActorCanvasProjectionMode::ActorBoundingBox)
                {
                    // 액터 전체 바운딩 박스
                    IndicatorBox = Component->GetOwner()->GetComponentsBoundingBox();
                }
                else
                {
                    // 컴포넌트 바운딩 박스
                    IndicatorBox = Component->Bounds.GetBox();
                }

                // 박스 중앙 + 크기*앵커오프셋(중앙이 0.5이므로 -0.5로 보정)
                const FVector ProjectBoxPoint = IndicatorBox.GetCenter() + (IndicatorBox.GetSize() * (IndicatorDescriptor.GetBoundingBoxAnchor() - FVector(0.5)));

                // 월드 점을 화면 좌표로 변환
                FVector2D OutScreenSpacePosition;
                const bool bInFrontOfCamera = ULocalPlayer::GetPixelPoint(InProjectionData, ProjectBoxPoint, OutScreenSpacePosition, &ScreenSize);
                // 수평 오프셋 적용 (카메라 뒤면 반전)
                OutScreenSpacePosition.X += IndicatorDescriptor.GetScreenSpaceOffset().X * (bInFrontOfCamera ? 1 : -1);
                // 수직 오프셋 적용
                OutScreenSpacePosition.Y += IndicatorDescriptor.GetScreenSpaceOffset().Y;

                // 카메라 뒤에 있으면서 화면 내부에 위치한 경우 화면 반대편 가장자리로 이동
                if (!bInFrontOfCamera && FBox2f(FVector2f::Zero(), ScreenSize).IsInside((FVector2f)OutScreenSpacePosition))
                {
                    // 방향 벡터 정규화 후 반대편 가장자리 계산
                    const FVector2f CenterToPosition = (FVector2f(OutScreenSpacePosition) - (ScreenSize / 2)).GetSafeNormal();
                    OutScreenSpacePosition = FVector2D((ScreenSize / 2) + CenterToPosition * ScreenSize);
                }

                // 출력: (화면X, 화면Y, 카메라-박스점 거리)
                OutScreenPositionWithDepth = FVector(OutScreenSpacePosition.X, OutScreenSpacePosition.Y, FVector::Dist(InProjectionData.ViewOrigin, ProjectBoxPoint));
                
                return true;
            }
        }
    }

    // 컴포넌트 없음 → 실패
    return false;
}

void UIndicatorDescriptor::SetIndicatorManagerComponent(ULeeIndicatorManagerComponent* InManager)
{
	if (ensure(ManagerPtr.IsExplicitlyNull()))
	{
		ManagerPtr = InManager;
	}
}

void UIndicatorDescriptor::UnregisterIndicator()
{
	if (ULeeIndicatorManagerComponent* Manager = ManagerPtr.Get())
	{
		ManagerPtr->RemoveIndicator(this);
	}
}
