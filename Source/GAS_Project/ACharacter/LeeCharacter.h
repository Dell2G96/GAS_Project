// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameplayCueInterface.h"
#include "GameplayTagAssetInterface.h"
#include "GenericTeamAgentInterface.h"
#include "ModularCharacter.h"
#include "GameFramework/Character.h"
#include "GAS_Project/ATeam/LeeTeamAgentInterface.h"
#include "GAS_Project/AInteraction/IInteractableTarget.h"
#include "LeeCharacter.generated.h"


USTRUCT()
struct FLeeReplicatedAcceleration
{
	GENERATED_BODY()
	
	UPROPERTY()
	uint8 AccelXYRadians = 0;	// Direction of XY accel component, quantized to represent [0, 2*pi]

	UPROPERTY()
	uint8 AccelXYMagnitude = 0;	//Accel rate of XY component, quantized to represent [0, MaxAcceleration]

	UPROPERTY()
	int8 AccelZ = 0;	// Raw Z accel rate component, quantized to represent [-MaxAcceleration, MaxAcceleration]
};


USTRUCT()
struct FSharedRepMovement
{
	GENERATED_BODY()

	FSharedRepMovement();

	bool FillForCharacter(ACharacter* Character);
	bool Equals(const FSharedRepMovement& Other, ACharacter* Character) const;

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);

	UPROPERTY(Transient)
	FRepMovement RepMovement;

	UPROPERTY(Transient)
	float RepTimeStamp = 0.0f;

	UPROPERTY(Transient)
	uint8 RepMovementMode = 0;

	UPROPERTY(Transient)
	bool bProxyIsJumpForceApplied = false;

	UPROPERTY(Transient)
	bool bIsCrouched = false;
};


template<>
struct TStructOpsTypeTraits<FSharedRepMovement> : public TStructOpsTypeTraitsBase2<FSharedRepMovement>
{
	enum
	{
		WithNetSerializer = true,
		WithNetSharedSerialization = true,
	};
};

UCLASS(Config = Game, Meta = (ShortTooltip = "The base character pawn class used by this project."))
class GAS_PROJECT_API ALeeCharacter : public AModularCharacter, public  IAbilitySystemInterface , public IGameplayCueInterface, public IGameplayTagAssetInterface, public ILeeTeamAgentInterface, public IInteractableTarget
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ALeeCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// 26.10.29 수정 - protected → public. 세이브 포인트 휴식/사망 리셋 시 ULeeSavePointSubsystem이 Pawn을 정리하기 위해 호출
	void UninitAndDestroy();


	// 26.10.29 추가 - DeathMontages 중 하나를 랜덤으로 반환 (비어 있으면 nullptr)
	class UAnimMontage* GetRandomDeathMontage() const;

	// 26.11.03 추가 - 서버의 Death 어빌리티가 호출. 모든 머신에서 사망 페이드(디졸브) BP 이벤트를 실행한다
	UFUNCTION(NetMulticast, Reliable)
	void MulticastStartDeathFade(float FadeDuration);

	// 26.11.03 수정 - protected → public. 세이브 포인트 리셋이 Controller를 떼기 전에 BP OnDeathFinished(인벤토리 정리)까지 즉시 실행하기 위해 호출
	void DestroyDueToDeath();

	// 26.11.07 추가 - 화면에 보여줄 캐릭터 이름 (보스 HUD의 보스 이름 등). 캐릭터 BP Class Defaults에서 지정
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Display")
	FText CharacterDisplayName;

	// 26.11.02 추가 - 보스전 넉다운 진입 시 GA_Downed가 재생할 몽타주 (캐릭터 BP Class Defaults에서 지정, 비우면 생략)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Downed")
	TObjectPtr<class UAnimMontage> DownedMontage;

	// 26.11.02 추가 - 넉다운 중 동료에게 보여줄 부활 상호작용 (BP에서 InteractionAbilityToGrant=GA_Revive, Text, 위젯 지정)
	// 26.11.02 수정 - protected → public. 디테일 패널에서 Lee|Downed가 둘로 갈라지지 않도록 DownedMontage 바로 아래로 이동
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Downed")
	FInteractionOption ReviveInteractionOption;

	// 26.11.04 추가 - 부활 게이지를 붙일 메시 소켓(또는 본) 이름. 기본은 가슴 본
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Downed")
	FName ReviveGaugeSocket = TEXT("spine_03");

	// 26.11.04 추가 - 부활 진행도 원형 게이지 위젯 (W_ReviveGauge, 비우면 표시 안 함)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Downed")
	TSubclassOf<class ULeeReviveGaugeWidget> ReviveGaugeWidgetClass;

	// 26.11.04 추가 - 부활시키는 사람의 화면에서만 호출. 가슴 소켓에 게이지를 띄우고 Duration초 동안 채운다
	void ShowReviveGauge(float Duration);

	// 26.11.04 추가 - 부활 게이지 숨김 (부활 완료/취소 시)
	void HideReviveGauge();

	// 26.11.06 추가 - 부활 프롬프트 문구 형식. {Name}=대상 이름, {Action}=ReviveInteractionOption.Text, {Seconds}=사망까지 남은 초
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Downed")
	FText ReviveInteractionFormat = NSLOCTEXT("Lee", "ReviveInteractionFormat", "{Name} {Action} (사망까지 {Seconds}초)");

	// 26.11.06 추가 - [서버] 넉다운 사망 카운트다운 시작 (GA_Downed가 호출). 시간이 다 되면 OnDownedCountdownExpired 브로드캐스트
	void StartDownedCountdown(float Duration);

	// 26.11.06 추가 - [서버] 카운트다운 정지·초기화 (넉다운 종료 시)
	void StopDownedCountdown();

	// 26.11.06 추가 - [서버] 부활 채널링 시작/종료. 부활시키는 사람이 1명이라도 있으면 사망 카운트다운이 멈춘다
	void AddReviver();
	void RemoveReviver();

	// 26.11.06 추가 - 사망까지 남은 시간(초). 서버·클라이언트 모두 사용 (복제된 값 + 서버 시간으로 계산)
	float GetDownedRemainingTime() const;

	// 26.11.06 추가 - 부활 중이라 카운트다운이 멈춰 있는지
	bool IsDownedCountdownPaused() const { return DownedPausedRemaining >= 0.0f; }

	// 26.11.06 추가 - [서버] 사망 카운트다운이 끝났을 때 (GA_Downed가 구독해 사망 처리)
	FSimpleMulticastDelegate OnDownedCountdownExpired;

	//~IInteractableTarget interface
	// 26.11.02 추가 - 넉다운 상태일 때 살아 있는 아군에게 "부활" 상호작용 옵션을 제공
	virtual void GatherInteractionOptions(const FInteractionQuery& InteractQuery, FInteractionOptionBuilder& OptionBuilder) override;
	//~End of IInteractableTarget interface

	UFUNCTION(BlueprintCallable, Category = "Lee|Character")
	class ALeePlayerController* GetLeePlayerController() const;

	UFUNCTION(BlueprintCallable, Category = "Lee|Character")
	class ALeePlayerState* GetLeePlayerState() const;

	UFUNCTION(BlueprintCallable, Category="Lee|Character")
	class ULeeAbilitySystemComponent* GetLeeAbilitySystemComponent() const;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override;
	virtual bool HasMatchingGameplayTag(FGameplayTag TagToCheck) const override;
	virtual bool HasAllMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const override;
	virtual bool HasAnyMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const override;

	void ToggleCrouch();
	
	//~AActor interface
	virtual void PreInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Reset() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker) override;
	//~End of AActor interface

	//~APawn interface
	virtual void NotifyControllerChanged() override;
	//~End of APawn interface

	//~ILeeTeamAgentInterface interface
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual FOnLeeTeamIndexChangedDelegate* GetOnTeamIndexChangedDelegate() override;
	// 팀 태도 판정: Controller에게 위임, 없으면 팀 ID 직접 비교
	virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;
	//~End of ILeeTeamAgentInterface interface

	UFUNCTION(NetMulticast, unreliable)
	void FastSharedReplication(const FSharedRepMovement& SharedRepMovement);

	FSharedRepMovement LastSharedReplication;

	virtual bool UpdateSharedReplication();

protected:
	void OnAbilitySystemInitialized();
	void OnAbilitySystemUnInitialized();

	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;

	virtual void OnRep_Controller() override;
	virtual void OnRep_PlayerState() override;

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) final;

	void InitializeGameplayTags();
	virtual void FellOutOfWorld(const class UDamageType& dmgType) override;
	
	UFUNCTION()
	virtual void OnDeathStarted(AActor* OwningActor);

	UFUNCTION()
	virtual void OnDeathFinished(AActor* OwningActor);

	void DisableMovementAndCollision();

	UFUNCTION(BlueprintImplementableEvent, meta=(DisplayName="OnDeathFinished"))
	void K2_OnDeathFinished();

	// 26.11.03 추가 - 사망 몽타주 후 페이드 시작 시 호출. BP에서 FadeDuration 동안 디졸브 파라미터를 0→1로 올린다
	UFUNCTION(BlueprintImplementableEvent, meta=(DisplayName="OnDeathFadeStarted"))
	void K2_OnDeathFadeStarted(float FadeDuration);

	// 26.10.29 추가 - 사망 시작 시 Death 어빌리티가 재생할 몽타주 후보 (캐릭터 BP Class Defaults에서 지정)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Death")
	TArray<TObjectPtr<class UAnimMontage>> DeathMontages;

	// 26.11.04 추가 - 부활 게이지를 그리는 WidgetComponent (처음 ShowReviveGauge 때 로컬에서만 생성)
	UPROPERTY(Transient)
	TObjectPtr<class UWidgetComponent> ReviveGaugeComponent;

	// 26.11.06 추가 - [복제] 카운트다운 진행 중일 때 사망 예정 서버 시간 (-1 = 넉다운 아님)
	UPROPERTY(Replicated)
	float DownedDeathServerTime = -1.0f;

	// 26.11.06 추가 - [복제] 부활 중이라 멈춰 있을 때 남은 시간 (-1 = 진행 중)
	UPROPERTY(Replicated)
	float DownedPausedRemaining = -1.0f;

	// 26.11.06 추가 - [서버] 사망 카운트다운 타이머와 부활시키는 사람 수
	FTimerHandle DownedCountdownHandle;
	int32 ActiveReviverCount = 0;

	// 26.11.06 추가 - [서버] 카운트다운 만료 콜백
	void HandleDownedCountdownExpired();

	// 26.11.06 추가 - 부활 프롬프트 문구 캐시 (남은 초나 이름이 바뀔 때만 새로 만들어 프롬프트가 매 스캔 다시 그려지지 않게 한다)
	FText CachedReviveInteractionText;
	int32 CachedReviveSeconds = -1;
	FString CachedReviveName;

	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode) override;
	void SetMovementModeTag(EMovementMode MovementMode, uint8 CustomMovementMode, bool bTagEnabled);

	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual bool CanJumpInternal_Implementation() const;

private:
	// UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Lee|Character")
	// TSubclassOf<class ULeeInputComponent> InputComponentClass;

	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Lee|Character",meta=(AllowPrivateAccess = "true"))
	TObjectPtr<class ULeePawnExtensionComponent> PawnExtComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Lee|Character" ,Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class ULeeHealthComponent> HealthComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Lee|Character", Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class ULeeCameraComponent> CameraComponent;
	
	UPROPERTY(Transient, ReplicatedUsing = OnRep_ReplicatedAcceleration)
	FLeeReplicatedAcceleration ReplicatedAcceleration;

	UPROPERTY(ReplicatedUsing = OnRep_MyTeamID)
	FGenericTeamId MyTeamID;

	UPROPERTY()
	FOnLeeTeamIndexChangedDelegate OnTeamChangedDelegate;

	// 26.11.03 추가 - DestroyDueToDeath가 이미 실행됐는지 (리셋 직접 호출 + 다음 틱 타이머의 중복 실행 방지)
	bool bDeathCleanupDone = false;


protected:
	// Called to determine what happens to the team ID when possession ends
	virtual FGenericTeamId DetermineNewTeamAfterPossessionEnds(FGenericTeamId OldTeamID) const
	{
		// This could be changed to return, e.g., OldTeamID if you want to keep it assigned afterwards, or return an ID for some neutral faction, or etc...
		return FGenericTeamId::NoTeam;
	}

	
private:
	UFUNCTION()
	void OnControllerChangedTeam(UObject* TeamAgent, int32 OldTeam, int32 NewTeam);

	// Controller가 팀 정보를 제공하지 않을 때(일반 AI 컨트롤러 등), GameState의 LeeTeamCreationComponent에서 Enemy 클래스 기준 팀 ID를 가져온다
	int32 ResolveEnemyTeamIdFromGameState() const;

	// Controller → PlayerState → GameState의 EnemyTeamId 매핑 순서로 유효한 팀 ID를 결정한다
	FGenericTeamId ResolveTeamID() const;

	UFUNCTION()
	void OnRep_ReplicatedAcceleration();

	UFUNCTION()
	void OnRep_MyTeamID(FGenericTeamId OldTeamID);

};
