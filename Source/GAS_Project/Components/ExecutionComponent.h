// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ExecutionComponent.generated.h"

UENUM()
enum class EExecutionRole : uint8
{
	None,
	Attacker,   
	Victim     
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class GAS_PROJECT_API UExecutionComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	   UExecutionComponent();

    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    /********************************************************/
    /* Public Interface - Character에서 호출 */
    /********************************************************/
    
    // ExecuteInterface 함수들을 위임받아 처리
    UFUNCTION(BlueprintCallable, Category = "Execution")
    void SetVictim(AActor* InVictim);
    
    UFUNCTION(BlueprintCallable, Category = "Execution")
    void PlayVictimMontage(int32 AttackIndex, AActor* Attacker);
    
    UFUNCTION(BlueprintCallable, Category = "Execution")
    void ActivateBloodTrail();
    
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Execution")
    bool CanBeExecuted() const;
    
    UFUNCTION(BlueprintCallable, Category = "Execution")
    void StartExecution(AActor* Target);
    
    // 플레이어용: 처형 입력 처리
    UFUNCTION(BlueprintCallable, Category = "Execution")
    void TryExecuteNearestTarget();
    
    // 현재 처형 중인지 확인
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Execution")
    bool IsExecuting() const { return CurrentRole != EExecutionRole::None; }
    
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Execution")
    EExecutionRole GetCurrentRole() const { return CurrentRole; }

protected:
    /********************************************************/
    /* Configuration */
    /********************************************************/
    
    // 공격자 몽타주 (플레이어용)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Execution|Attacker")
    TArray<class UAnimMontage*> AttackerMontages;
    
    // 피해자 몽타주
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Execution|Victim")
    TArray<class UAnimMontage*> VictimMontages;
    
    // 피 이펙트
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Execution|Effects")
    class UNiagaraSystem* BloodEffect;
    
    // 피 이펙트 소켓 이름
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Execution|Effects")
    FName BloodEffectSocketName = FName("spine_03");
    
    // 모션 워핑 타겟 이름
    UPROPERTY(EditDefaultsOnly, Category = "Execution|Warping")
    FName ExecutionWarpTargetName = FName(TEXT("ExecutionTarget"));
    
    // 처형 가능 거리
    UPROPERTY(EditDefaultsOnly, Category = "Execution|Range")
    float ExecutionRange = 200.f;
    
    // 처형 시작 거리 (공격자가 피해자에게서 떨어질 거리)
    UPROPERTY(EditDefaultsOnly, Category = "Execution|Warping")
    float ExecutionStartDistance = 100.f;
    
    // 처형 가능 체력 퍼센트 (0.0 ~ 1.0)
    UPROPERTY(EditDefaultsOnly, Category = "Execution|Requirements")
    float ExecutableHealthPercent = 0.3f;
    
    // Groggy 상태일 때 무조건 처형 가능
    UPROPERTY(EditDefaultsOnly, Category = "Execution|Requirements")
    bool bExecutableWhenGroggy = true;
    
    /********************************************************/
    /* Internal State */
    /********************************************************/
    
    UPROPERTY(ReplicatedUsing = OnRep_CurrentRole)
    EExecutionRole CurrentRole = EExecutionRole::None;
    
    UPROPERTY(Replicated)
    TObjectPtr<AActor> CurrentTarget = nullptr;
    
    UFUNCTION()
    void OnRep_CurrentRole();

private:
    /********************************************************/
    /* Internal Functions */
    /********************************************************/
    
    // 가장 가까운 처형 가능한 타겟 찾기
    AActor* FindNearestExecutableTarget() const;
    
    // 모션 워핑 설정
    void SetupMotionWarping(AActor* Target);
    
    // 캐릭터 움직임 제어
    void DisableCharacterMovement();
    void EnableCharacterMovement();
    
    // 입력 제어 (플레이어만)
    void DisableCharacterInput();
    void EnableCharacterInput();
    
    // AI 제어 (AI만)
    void DisableAI();
    void EnableAI();
    
    /********************************************************/
    /* Network RPCs */
    /********************************************************/
    
    UFUNCTION(Server, Reliable)
    void Server_RequestExecution(AActor* Target, int32 MontageIndex);
    
    UFUNCTION(NetMulticast, Reliable)
    void Multicast_PlayExecution(AActor* Attacker, AActor* Victim, 
        int32 AttackerMontageIndex, int32 VictimMontageIndex);
    
    /********************************************************/
    /* Montage Callbacks */
    /********************************************************/
    
    UFUNCTION()
    void OnAttackerMontageEnded(UAnimMontage* Montage, bool bInterrupted);
    
    UFUNCTION()
    void OnVictimMontageEnded(UAnimMontage* Montage, bool bInterrupted);
    
    /********************************************************/
    /* Cached References */
    /********************************************************/
    
    UPROPERTY()
    TObjectPtr<ACharacter> OwnerCharacter = nullptr;
    
    UPROPERTY()
    TObjectPtr<class UMotionWarpingComponent> MotionWarpingComp = nullptr;
		
};
