// 26.11.12 신규 - 보스전 진행 단계 관리자

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LeeBossEncounterManager.generated.h"

class ALeeBossEntranceBase;
class ULeeBossIntro;

// 보스전 진행 단계 (대기 → 인트로 → 전투 → 승리/전멸 순서로만 흐른다)
UENUM(BlueprintType)
enum class ELeeBossEncounterPhase : uint8
{
	Waiting,	// 맵 로딩 후 입장 대기. 보스 AI 정지
	Intro,		// 첫 도전 인트로 연출 (포효)
	Combat,		// 전투 중. 보스 AI 가동, 입구 봉쇄
	Victory,	// 보스 처치
	Wipe		// 파티 전멸
};

DECLARE_MULTICAST_DELEGATE_OneParam(FLeeBossEncounterPhaseChangedDelegate, ELeeBossEncounterPhase /*NewPhase*/);

// 보스맵에 1개 배치하는 보스전 진행 관리자 (열거형 상태 머신)
// 서버가 단계를 바꾸고 클라이언트에 복제한다. 보스 AI·HUD는 OnPhaseChanged를 구독해 반응한다 (Observer)
// 입장 방식(안개벽/문)과 인트로 연출은 교체 가능한 객체로 받는다 (Strategy)
UCLASS(Blueprintable)
class GAS_PROJECT_API ALeeBossEncounterManager : public AActor
{
	GENERATED_BODY()

public:
	ALeeBossEncounterManager();

	// 현재 월드의 보스전 관리자 (없으면 nullptr — 관리자가 없는 맵은 기존처럼 바로 전투)
	static ALeeBossEncounterManager* Find(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "Lee|BossEncounter")
	ELeeBossEncounterPhase GetPhase() const { return Phase; }

	// 보스 HP 바를 보여도 되는 단계인지 (대기 중에는 숨긴다)
	bool IsBossRevealed() const { return Phase != ELeeBossEncounterPhase::Waiting; }

	// [서버] 입구가 "플레이어가 아레나에 들어왔다"고 알린다. 대기 단계의 첫 입장만 전투를 시작시킨다
	void HandlePlayerEntered(ALeeBossEntranceBase* Entrance, APawn* EnteredPawn);

	// 단계가 바뀔 때 서버·클라이언트 모두에서 호출된다
	FLeeBossEncounterPhaseChangedDelegate OnPhaseChanged;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// BP에서 BGM·연출을 붙일 수 있게 단계 변경을 알린다
	UFUNCTION(BlueprintImplementableEvent, Category = "Lee|BossEncounter", meta = (DisplayName = "On Phase Changed"))
	void K2_OnPhaseChanged(ELeeBossEncounterPhase NewPhase);

	// 세이브의 도전 기록 키 (보스마다 고유하게)
	UPROPERTY(EditInstanceOnly, Category = "Lee|BossEncounter")
	FName BossId;

	// 이 전투의 보스 (인트로 포효 대상). 비우면 BossActorTag로 찾는다
	UPROPERTY(EditInstanceOnly, Category = "Lee|BossEncounter")
	TObjectPtr<AActor> BossActor;

	// BossActor가 비었을 때 보스를 찾을 액터 태그 (보스 HP 바·승리 판정과 같은 "Boss")
	UPROPERTY(EditAnywhere, Category = "Lee|BossEncounter")
	FName BossActorTag = TEXT("Boss");

	// 첫 도전 때 쓰는 입구 (안개벽)
	UPROPERTY(EditInstanceOnly, Category = "Lee|BossEncounter")
	TObjectPtr<ALeeBossEntranceBase> FirstAttemptEntrance;

	// 재도전 때 쓰는 입구 (문)
	UPROPERTY(EditInstanceOnly, Category = "Lee|BossEncounter")
	TObjectPtr<ALeeBossEntranceBase> RetryEntrance;

	// 첫 도전 인트로 연출 (기본: 포효). 재도전은 인트로를 생략한다
	UPROPERTY(EditAnywhere, Instanced, Category = "Lee|BossEncounter")
	TObjectPtr<ULeeBossIntro> Intro;

	UPROPERTY(ReplicatedUsing = OnRep_Phase, VisibleInstanceOnly, Category = "Lee|BossEncounter")
	ELeeBossEncounterPhase Phase = ELeeBossEncounterPhase::Waiting;

private:
	// 단계 전환은 이 함수 한 곳에서만 한다
	void SetPhase(ELeeBossEncounterPhase NewPhase);

	// 허용된 전환인지 (역방향·건너뛰기 방지)
	static bool CanTransition(ELeeBossEncounterPhase From, ELeeBossEncounterPhase To);

	// 새 단계에 들어갈 때 할 일 (서버)
	void EnterPhase(ELeeBossEncounterPhase NewPhase);

	// 호스트 도전 기록으로 안개벽/문 중 하나를 켠다 (서버)
	void SetupEntrance();

	// 파티 전원의 세이브에 도전(bDefeated=false) 또는 처치(true) 기록을 남긴다 (서버)
	void RecordForAllPlayers(bool bDefeated);

	// 인트로 대상 보스를 찾는다
	AActor* FindBossActor() const;

	void HandleIntroFinished();
	void HandleBossDefeated();
	void HandlePartyWiped();

	UFUNCTION()
	void OnRep_Phase();

	// 단계 변경을 C++ 구독자와 BP에 알린다
	void BroadcastPhaseChanged();

	// 이번 판에 쓰는 입구
	UPROPERTY(Transient)
	TObjectPtr<ALeeBossEntranceBase> ActiveEntrance;

	// 호스트가 이 보스에 처음 도전하는지 (서버)
	bool bFirstAttempt = true;

	FDelegateHandle BossDefeatedHandle;
	FDelegateHandle PartyWipedHandle;
};
