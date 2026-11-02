// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonSessionSubsystem.h"
#include "GameFramework/Actor.h"
#include "GAS_Project/AInteraction/IInteractableTarget.h"
#include "GAS_Project/AInteraction/InteractionOption.h"
#include "LeeBossRequestActor.generated.h"

class UCommonActivatableWidget;
class ULeeUserFacingExperience;
class USphereComponent;
class UStaticMeshComponent;

// 26.11.02 추가 - 필드의 보스방 앞에 두는 "도움 요청" Actor. F 상호작용으로 보스 원정 로비 UI(방 만들기/방 목록)를 연다
// 방 정보(보스 이름, 로비 맵·Experience, 접속 방식)를 가지고 있고, 로비 위젯이 이 값을 읽어 세션을 만들거나 찾는다
UCLASS(Blueprintable)
class GAS_PROJECT_API ALeeBossRequestActor : public AActor, public IInteractableTarget
{
	GENERATED_BODY()

public:
	ALeeBossRequestActor();

	//~IInteractableTarget interface
	virtual void GatherInteractionOptions(const FInteractionQuery& InteractQuery, FInteractionOptionBuilder& InteractionBuilder) override;
	//~End of IInteractableTarget interface

	const FText& GetBossName() const { return BossName; }
	const ULeeUserFacingExperience* GetBossLobbyExperience() const { return BossLobbyExperience; }
	ECommonSessionOnlineMode GetSessionOnlineMode() const { return SessionOnlineMode; }
	TSoftClassPtr<UCommonActivatableWidget> GetLobbyWidgetClass() const { return LobbyWidgetClass; }

protected:
	// 외형 (BP에서 메시 지정)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lee|BossRequest")
	TObjectPtr<UStaticMeshComponent> RequestMesh;

	// 상호작용 감지용 구체 (Interaction 채널에만 Overlap)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lee|BossRequest")
	TObjectPtr<USphereComponent> InteractionSphere;

	// 상호작용 옵션 (InteractionAbilityToGrant = LeeGameplayAbility_OpenBossLobby, Text = "도움 요청")
	UPROPERTY(EditAnywhere, Category = "Lee|BossRequest")
	FInteractionOption Option;

	// 방 목록에 표시할 보스 이름
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lee|BossRequest")
	FText BossName;

	// 방을 만들 때 이동할 로비 맵 + Experience + 최대 인원 (MapID = L_BossLobby, Experience = MultiplayerBoss)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lee|BossRequest")
	TObjectPtr<ULeeUserFacingExperience> BossLobbyExperience;

	// 세션 방식 (NULL 서브시스템/PIE 테스트는 LAN, Steam·EOS 연동 후 Online)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lee|BossRequest")
	ECommonSessionOnlineMode SessionOnlineMode = ECommonSessionOnlineMode::LAN;

	// 상호작용 시 띄울 로비 위젯 (ULeeBossLobbyWidget 기반 BP)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lee|BossRequest")
	TSoftClassPtr<UCommonActivatableWidget> LobbyWidgetClass;
};
