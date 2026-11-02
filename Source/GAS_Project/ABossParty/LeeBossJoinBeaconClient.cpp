// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeBossJoinBeaconClient.h"

#include "LeeBossJoinBeaconHostObject.h"
#include "TimerManager.h"
#include "Engine/World.h"

// 호스트 비콘 주소(IP:BeaconPort)로 접속 시작 — 결과는 OnConnected / OnFailure로 온다
bool ALeeBossJoinBeaconClient::RequestJoin(const FString& ConnectInfo, const FString& InPlayerName)
{
	PlayerName = InPlayerName;

	FURL URL(nullptr, *ConnectInfo, TRAVEL_Absolute);
	return InitClient(URL);
}

// 접속 완료 → 호스트에 입장 요청 전송
void ALeeBossJoinBeaconClient::OnConnected()
{
	ServerRequestJoin(PlayerName);
}

// 접속 실패 / 호스트가 사라짐 → 아직 결과를 받지 않았다면 실패로 알린다
void ALeeBossJoinBeaconClient::OnFailure()
{
	Super::OnFailure();

	if (!bResultReceived)
	{
		FinishWithResult(ELeeBossJoinResult::Failed);
	}
}

// [호스트] 요청자 이름 저장 후 HostObject의 대기 목록에 추가
void ALeeBossJoinBeaconClient::ServerRequestJoin_Implementation(const FString& InPlayerName)
{
	PlayerName = InPlayerName;

	if (ALeeBossJoinBeaconHostObject* HostObject = Cast<ALeeBossJoinBeaconHostObject>(GetBeaconOwner()))
	{
		HostObject->AddRequest(this);
	}
}

// [클라이언트] 수락 받음
void ALeeBossJoinBeaconClient::ClientJoinApproved_Implementation()
{
	FinishWithResult(ELeeBossJoinResult::Approved);
}

// [클라이언트] 거절 받음
void ALeeBossJoinBeaconClient::ClientJoinRejected_Implementation()
{
	FinishWithResult(ELeeBossJoinResult::Rejected);
}

// 결과 1회 알림 + 다음 틱에 비콘 파괴 (RPC/네트워크 처리 중에 NetDriver를 지우면 위험하므로)
void ALeeBossJoinBeaconClient::FinishWithResult(ELeeBossJoinResult Result)
{
	bResultReceived = true;
	OnJoinResult.ExecuteIfBound(Result);
	OnJoinResult.Unbind();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			DestroyBeacon();
		}));
	}
}
