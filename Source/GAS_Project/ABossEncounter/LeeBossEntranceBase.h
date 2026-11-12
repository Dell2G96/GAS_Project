// 26.11.12 신규 - 보스 아레나 입구 공통 베이스

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GAS_Project/AInteraction/IInteractableTarget.h"
#include "GAS_Project/AInteraction/InteractionOption.h"
#include "LeeBossEntranceBase.generated.h"

class ALeeBossEncounterManager;
class UArrowComponent;
class UBoxComponent;

// 입구 상태
UENUM(BlueprintType)
enum class ELeeBossEntranceState : uint8
{
	Disabled,	// 이번 판에 쓰지 않음 (충돌·상호작용 없음)
	Open,		// 그냥 지나갈 수 있음 (재도전 문의 대기 상태)
	Blocked		// 막혀 있고, 바깥에서 F로만 안쪽으로 들어갈 수 있음 (안개벽, 전투 중 봉쇄)
};

// 보스 아레나 입구의 공통 베이스 (Strategy: 안개벽·문이 이 타입으로 관리자에 꽂힌다)
// 막힘 충돌, 바깥쪽 F 상호작용, 안쪽 지점으로 이동, 관리자에 입장 알림을 공통으로 처리한다
// 액터의 앞(X+) 방향이 아레나 안쪽이다. InsidePoint는 통과 후 도착할 위치
UCLASS(Abstract, Blueprintable)
class GAS_PROJECT_API ALeeBossEntranceBase : public AActor, public IInteractableTarget
{
	GENERATED_BODY()

public:
	ALeeBossEntranceBase();

	//~IInteractableTarget interface
	virtual void GatherInteractionOptions(const FInteractionQuery& InteractQuery, FInteractionOptionBuilder& InteractionBuilder) override;
	//~End of IInteractableTarget interface

	// [서버] 이번 판의 입구로 켠다 (첫 상태는 하위 클래스가 정한다)
	void ActivateEntrance(ALeeBossEncounterManager* InManager);

	// [서버] 전투 시작 시 봉쇄 (밖으로 못 나가고, 바깥 동료는 F로 들어올 수 있음)
	void SealEntrance();

	// [서버] 이번 판에 쓰지 않는다
	void DisableEntrance();

	// [서버] F 상호작용 처리: 안쪽 지점으로 이동시키고 관리자에 입장을 알린다
	void PassThrough(APawn* Pawn);

	UFUNCTION(BlueprintPure, Category = "Lee|BossEntrance")
	ELeeBossEntranceState GetEntranceState() const { return EntranceState; }

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 이번 판 입구로 켰을 때의 첫 상태
	virtual ELeeBossEntranceState GetActivatedState() const { return ELeeBossEntranceState::Blocked; }

	// 관리자에게 "플레이어가 아레나에 들어왔다"고 알린다 (서버)
	void NotifyPlayerEntered(APawn* Pawn);

	// 상태가 정해지거나 바뀔 때 (서버·클라이언트). BP에서 안개 이펙트·문 열림/닫힘 연출을 처리한다
	UFUNCTION(BlueprintImplementableEvent, Category = "Lee|BossEntrance", meta = (DisplayName = "On Entrance State Changed"))
	void K2_OnEntranceStateChanged(ELeeBossEntranceState NewState);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lee|BossEntrance")
	TObjectPtr<USceneComponent> SceneRoot;

	// Blocked일 때 Pawn을 막는 벽
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lee|BossEntrance")
	TObjectPtr<UBoxComponent> BlockerBox;

	// 상호작용 감지용 (Interaction 채널에만 Overlap)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lee|BossEntrance")
	TObjectPtr<UBoxComponent> InteractionBox;

	// 통과 후 도착 위치·방향 (아레나 안쪽에 둔다)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lee|BossEntrance")
	TObjectPtr<UArrowComponent> InsidePoint;

	// 상호작용 옵션 (기본: LeeGameplayAbility_PassBossEntrance, 문구는 BP에서 지정)
	UPROPERTY(EditAnywhere, Category = "Lee|BossEntrance")
	FInteractionOption Option;

	UPROPERTY(ReplicatedUsing = OnRep_EntranceState, VisibleInstanceOnly, Category = "Lee|BossEntrance")
	ELeeBossEntranceState EntranceState = ELeeBossEntranceState::Disabled;

	UPROPERTY(Transient)
	TWeakObjectPtr<ALeeBossEncounterManager> Manager;

private:
	// 상태 변경 (서버)
	void SetEntranceState(ELeeBossEntranceState NewState);

	UFUNCTION()
	void OnRep_EntranceState();

	// 상태에 맞게 충돌을 켜고 끈 뒤 BP에 알린다 (서버·클라이언트)
	void ApplyEntranceState();

	// 26.11.12 추가 - 컴포넌트 태그(ShowWhenBlocked / ShowWhenOpen)로 상태별 외형 표시
	void ApplyStateVisuals();

	// 요청한 Pawn이 입구 바깥쪽에 있는지
	bool IsOutside(const AActor* Avatar) const;
};
