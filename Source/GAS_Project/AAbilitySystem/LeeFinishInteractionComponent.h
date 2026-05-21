// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LeeFinishInteractionComponent.generated.h"

class UBoxComponent;
class UAbilitySystemComponent;

UENUM(BlueprintType)
enum class ELeeFinishType : uint8
{
	None            UMETA(DisplayName = "None"),
	Execution       UMETA(DisplayName = "Execution"),
	Assassination   UMETA(DisplayName = "Assassination"),
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FOnLeeFinishCandidateEvent,
	AActor*, EnemyActor,
	AActor*, PlayerActor,
	ELeeFinishType, FinishType);


/** Enemy에 부착되어 앞/뒤 처형/암살 감지 박스를 제공하는 컴포넌트. */
UCLASS(ClassGroup=(Custom),Blueprintable, meta=(BlueprintSpawnableComponent))
class GAS_PROJECT_API ULeeFinishInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULeeFinishInteractionComponent();

	virtual void InitializeComponent() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Deprecated: 이제 Player의 ULeeFinishTargetComponent에 직접 overlap을 등록한다.
	UPROPERTY(BlueprintAssignable, Category = "Lee|Finish")
	FOnLeeFinishCandidateEvent OnFinishCandidateEntered;

	UPROPERTY(BlueprintAssignable, Category = "Lee|Finish")
	FOnLeeFinishCandidateEvent OnFinishCandidateLeft;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lee|Finish")
	UBoxComponent* GetExecutionBox() const { return Box_Execution; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lee|Finish")
	UBoxComponent* GetAssassinationBox() const { return Box_Assassination; }

	// Indicator 부착용(가슴) 소켓 이름
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lee|Finish")
	FName GetIndicatorSocketName() const { return IndicatorSocketName; }

	// 현재 이 Enemy 기준으로 해당 타입이 유효한지 (오버랩 중 + 조건 만족)
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lee|Finish")
	bool IsCandidateValidFor(AActor* Player, ELeeFinishType Type) const;

protected:
	// 콜리전 박스는 C++ 기본값을 제공하고, 블루프린트에서 시각적으로 조절할 수 있다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lee|Finish|Collision")
	TObjectPtr<UBoxComponent> Box_Execution;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lee|Finish|Collision")
	TObjectPtr<UBoxComponent> Box_Assassination;

	UPROPERTY(EditDefaultsOnly, Category = "Lee|Finish|Setup")
	FVector ExecutionBoxExtent = FVector(60.f, 80.f, 90.f);

	UPROPERTY(EditDefaultsOnly, Category = "Lee|Finish|Setup")
	FVector ExecutionBoxOffset = FVector(90.f, 0.f, 0.f);

	UPROPERTY(EditDefaultsOnly, Category = "Lee|Finish|Setup")
	FVector AssassinationBoxExtent = FVector(60.f, 80.f, 90.f);

	UPROPERTY(EditDefaultsOnly, Category = "Lee|Finish|Setup")
	FVector AssassinationBoxOffset = FVector(-90.f, 0.f, 0.f);

	// Indicator가 부착될 Enemy Mesh 소켓(기본: 가슴)
	UPROPERTY(EditDefaultsOnly, Category = "Lee|Finish|Indicator")
	FName IndicatorSocketName = TEXT("spine_03");

	// 에디터에서 박스 범위를 시각적으로 확인하는 디버그 옵션 (Shipping 빌드 제외)
	UPROPERTY(EditAnywhere, Category = "Lee|Finish|Debug")
	bool bDrawDebugBoxes = false;

private:
	UFUNCTION()
	void OnExecutionBoxBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnExecutionBoxEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UFUNCTION()
	void OnAssassinationBoxBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnAssassinationBoxEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void HandleBeginOverlap(AActor* OtherActor, ELeeFinishType Type);
	void HandleEndOverlap(AActor* OtherActor, ELeeFinishType Type);

	bool IsPlayerActor(const AActor* Actor) const;
	bool CanBeExecutedBy(AActor* Player) const;
	bool CanBeAssassinatedBy(AActor* Player) const;

	UAbilitySystemComponent* GetOwnerAbilitySystem() const;

	TSet<TWeakObjectPtr<AActor>> PlayersInExecutionBox;
	TSet<TWeakObjectPtr<AActor>> PlayersInAssassinationBox;
};
