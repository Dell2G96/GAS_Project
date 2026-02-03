// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS_Project/Characters/CCharacter.h"
#include "CEnemyBase.generated.h"

UCLASS()
class GAS_PROJECT_API ACEnemyBase : public ACCharacter
{
	GENERATED_BODY()

public:
	ACEnemyBase();
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual UAttributeSet* GetAttributeSet() const override;

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	FORCEINLINE class UBoxComponent* GetLeftHandCollision() const { return LeftHandCollision; }
	FORCEINLINE class UBoxComponent* GetRightHandCollision() const { return RightHandCollision;}
protected:
	virtual void BeginPlay() override;
	virtual void HandleDeath() override;

	/*********************************************************************/
	/*								Collision                            */
	/*********************************************************************/
#if WITH_EDITOR
	//~ Begin UObject Interface.
	virtual void PostEditChangeProperty( struct FPropertyChangedEvent& PropertyChangedEvent) override;
	//~ End UObject Interface
#endif
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GAS|Collision")
	FName LeftHandSocket;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GAS|Collision")
	FName RightHandSocket;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GAS|Collision")
	class UBoxComponent* LeftHandCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GAS|Collision")
	class UBoxComponent* RightHandCollision;



	

	/*********************************************************************/
	/*								Execution                            */
	/*********************************************************************/
public:
	UFUNCTION(BlueprintPure, Category="GAS|Execution")
	FVector GetExecutionUIWorldLocation() const;
	
protected:
	
	UFUNCTION()
    void OnExecutionOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
    
    UFUNCTION()
    void OnExecutionOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);


	
	void OnGroggyStateChanged(bool bIsGroggy);
	void HandleGroggyTagChanged(const struct FGameplayTag Tag, int32 NewCount);

	// 조건 판정 Only 서버
	bool CanShowExecutionUIFor(class ACPlayerCharacter* Player) const;
	
	void NotifyExecutionUI(ACPlayerCharacter* Player, bool bShow);

	// 오버랩 범위 내 Timer
	void StartExecutionEvaluationTimer();
	void StopExecutionEvaluationTimer();
	void EvaluateExecutionForPlayers();
	void SetExecutionUIForPlayer(ACPlayerCharacter* Player, bool bShow);

	FTimerHandle ExecutionUITimer;

	// [ADDED] UI 기준 소켓/오프셋 (프로젝트 스켈레톤에 맞게 바꿔도 됨)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GAS|Execution")
	FName ExecutionUISocketName = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GAS|Execution")
	FVector ExecutionUIOffset = FVector(0.f, 0.f, 0.f);

	// [ADDED] Enemy 뒤 60도 조건 (뒤 기준 60도면 Dot <= cos(120) = -0.5)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GAS|Execution")
	float ExecutionBehindAngleDeg = 60.f;

	// [ADDED] 블랙보드 TargetActor 키
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GAS|Execution")
	FName BlackboardTargetKeyName = TEXT("TargetActor");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GAS|Execution")
	class USphereComponent* ExecutionTrigger;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GAS|Execution")
	class UWidgetComponent* ExecutionWidgetComponent;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GAS|Execution")
	TSubclassOf<UUserWidget> ExecutionUIClass;

	UPROPERTY()
	TSet<TWeakObjectPtr<class ACPlayerCharacter>> PlayersInExecutionRange;
	
	// [ADDED] 서버에서만 사용: 마지막으로 이 플레이어에게 보여줬는지 캐시
	TMap<TWeakObjectPtr<ACPlayerCharacter>, bool> LastExecutionUIState;

	// [ADDED] 서버에서만 사용: 주기적 판정 타이머
	FTimerHandle ExecutionEvalTimerHandle;
	
	// 실제 인스턴스를 저장할 포인터
	UPROPERTY()
	UUserWidget* ExecutionUI;

	
	/*********************************************************************/
	/*								Team ID                              */
	/*********************************************************************/
public:
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	
	

	/*********************************************************************/
	/*						Death And Respawn                            */
	/*********************************************************************/
private:
	virtual void OnDead() override;
	virtual void OnRespawn() override;

public:
	// UFUNCTION(BlueprintCallable)
	// FORCEINLINE UCAbilitySystemComponent* GetMyAbilitySystemComponent() const { return MyAbilitySystemComponent; }
	//
	// UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GAS|Ability")
	// class UCAbilitySystemComponent* MyAbilitySystemComponent;

	/*********************************************************************/
	/*								AI									 */
	/*********************************************************************/
public:
	// ✅ AnimBP에서 Strafing 태그가 클라에서 안 보일 때를 대비한 복제 상태
	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool IsStrafing() const { return bIsStrafing; }

	// ✅ 서버에서 Strafing 태그 변화를 감지해 bIsStrafing을 갱신
	void SetupStrafingReplicationBridge();
	void HandleStrafingTagChanged(const FGameplayTag Tag, int32 NewCount);

	

	// ✅ Strafing 상태(서버 → 모든 클라)
	UPROPERTY(ReplicatedUsing = OnRep_IsStrafing, VisibleAnywhere, BlueprintReadOnly, Category="GAS|Locomotion")
	bool bIsStrafing = false;

	UFUNCTION()
	void OnRep_IsStrafing();
	

};


