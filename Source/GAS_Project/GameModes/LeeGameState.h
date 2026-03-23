// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "ModularGameState.h"
#include "GameFramework/GameStateBase.h"
#include "LeeGameState.generated.h"

struct FLeeVerbMessage;
class APlayerState;
class UAbilitySystemComponent;
class ULeeAbilitySystemComponent;
class ULeeExperienceManagerComponent;
class UObject;

UCLASS(Config = Game)
class GAS_PROJECT_API ALeeGameState : public AModularGameStateBase, public  IAbilitySystemInterface
{
    GENERATED_BODY()

public:
    ALeeGameState(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    //~AActor 인터페이스
    virtual void PreInitializeComponents() override;
    virtual void PostInitializeComponents() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaSeconds) override;
    //~AActor 인터페이스 끝


    //~AGameStateBase 인터페이스
    virtual void AddPlayerState(APlayerState* PlayerState) override;
    virtual void RemovePlayerState(APlayerState* PlayerState) override;
    virtual void SeamlessTravelTransitionCheckpoint(bool bToTransitionMap) override;
    //~AGameStateBase 인터페이스 끝

    //~IAbilitySystemInterface
     virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
    //~IAbilitySystemInterface 끝

    // 게임 전반에 걸쳐 사용되는 어빌리티 시스템 컴포넌트를 반환합니다
    UFUNCTION(BlueprintCallable, Category = "Lee|GameState")
    ULeeAbilitySystemComponent* GetLeeAbilitySystemComponent() const { return AbilitySystemComponent; }

    // 모든 클라이언트에게 (아마도) 전달될 메시지를 전송합니다
    // (처치 알림, 서버 접속 메시지 등 유실되어도 괜찮은 클라이언트 알림에만 사용하세요)
    UFUNCTION(NetMulticast, Unreliable, BlueprintCallable, Category = "Lee|GameState")
     void MulticastMessageToClients(const FLeeVerbMessage Message);

    // 모든 클라이언트에게 반드시 전달될 메시지를 전송합니다
    // (유실되면 안 되는 클라이언트 알림에만 사용하세요)
    UFUNCTION(NetMulticast, Reliable, BlueprintCallable, Category = "Lee|GameState")
     void MulticastReliableMessageToClients(const FLeeVerbMessage Message);

    // 서버의 FPS를 반환합니다 (클라이언트에 복제됨)
     float GetServerFPS() const;

    // 로컬 플레이어 스테이트가 리플레이를 녹화 중임을 표시합니다
     void SetRecorderPlayerState(APlayerState* NewPlayerState);

    // 유효한 경우 리플레이를 녹화한 플레이어 스테이트를 반환합니다
     APlayerState* GetRecorderPlayerState() const;

    // 리플레이 녹화 플레이어 스테이트가 변경될 때 호출되는 델리게이트
    DECLARE_MULTICAST_DELEGATE_OneParam(FOnRecorderPlayerStateChanged, APlayerState*);
    FOnRecorderPlayerStateChanged OnRecorderPlayerStateChangedEvent;

private:
    // 현재 게임플레이 경험(Experience)의 로딩 및 관리를 담당하는 컴포넌트
    UPROPERTY()
    TObjectPtr<ULeeExperienceManagerComponent> ExperienceManagerComponent;

    // 게임 전반에 걸쳐 사용되는 어빌리티 시스템 컴포넌트 서브오브젝트 (주로 게임플레이 큐 처리)
    UPROPERTY(VisibleAnywhere, Category = "Lee|GameState")
    TObjectPtr<ULeeAbilitySystemComponent> AbilitySystemComponent;

protected:
    UPROPERTY(Replicated)
    float ServerFPS;

    // 리플레이를 녹화한 플레이어 스테이트입니다. 따라갈 올바른 폰을 선택하는 데 사용됩니다
    // 리플레이 스트림에서만 설정되며, 일반적으로는 복제되지 않습니다
    UPROPERTY(Transient, ReplicatedUsing = OnRep_RecorderPlayerState)
    TObjectPtr<APlayerState> RecorderPlayerState;

    UFUNCTION()
     void OnRep_RecorderPlayerState();
    


};
