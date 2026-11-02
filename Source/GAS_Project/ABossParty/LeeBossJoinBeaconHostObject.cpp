// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeBossJoinBeaconHostObject.h"

#include "LeeBossJoinBeaconClient.h"
#include "Engine/NetConnection.h"

// 이 HostObject가 담당할 클라이언트 비콘 클래스와 타입 이름 지정 (클라이언트 비콘의 클래스 이름과 같아야 연결된다)
ALeeBossJoinBeaconHostObject::ALeeBossJoinBeaconHostObject(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ClientBeaconActorClass = ALeeBossJoinBeaconClient::StaticClass();
	BeaconTypeName = ClientBeaconActorClass->GetName();
}

// 대기 목록에 추가 (중복 요청은 무시)
void ALeeBossJoinBeaconHostObject::AddRequest(ALeeBossJoinBeaconClient* Request)
{
	if (Request && !PendingRequests.Contains(Request))
	{
		PendingRequests.Add(Request);
		OnRequestsChanged.Broadcast();
	}
}

// 수락 결과 전송 후 목록에서 제거
void ALeeBossJoinBeaconHostObject::ApproveRequest(ALeeBossJoinBeaconClient* Request)
{
	if (Request && PendingRequests.Remove(Request) > 0)
	{
		// 비콘 연결의 PlayerId는 접속할 때 보낸 플레이어 고유 ID다 (게임 접속 후 PlayerState의 UniqueId와 같은 값)
		const UNetConnection* Connection = Request->GetNetConnection();
		const FUniqueNetIdRepl PlayerId = Connection ? Connection->PlayerId : FUniqueNetIdRepl();

		Request->ClientJoinApproved();
		OnRequestApproved.Broadcast(PlayerId);
		OnRequestsChanged.Broadcast();
	}
}

// 거절 결과 전송 후 목록에서 제거
void ALeeBossJoinBeaconHostObject::RejectRequest(ALeeBossJoinBeaconClient* Request)
{
	if (Request && PendingRequests.Remove(Request) > 0)
	{
		Request->ClientJoinRejected();
		OnRequestsChanged.Broadcast();
	}
}

// 대기 중인 요청 전부 거절
void ALeeBossJoinBeaconHostObject::RejectAllRequests()
{
	for (ALeeBossJoinBeaconClient* Request : PendingRequests)
	{
		if (Request)
		{
			Request->ClientJoinRejected();
		}
	}

	PendingRequests.Empty();
	OnRequestsChanged.Broadcast();
}

// 요청자가 대기를 취소했거나 연결이 끊김 → 목록에서 제거
void ALeeBossJoinBeaconHostObject::NotifyClientDisconnected(AOnlineBeaconClient* LeavingClientActor)
{
	if (PendingRequests.Remove(Cast<ALeeBossJoinBeaconClient>(LeavingClientActor)) > 0)
	{
		OnRequestsChanged.Broadcast();
	}

	Super::NotifyClientDisconnected(LeavingClientActor);
}
