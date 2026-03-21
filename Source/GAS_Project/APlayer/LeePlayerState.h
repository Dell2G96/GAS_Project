// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "ModularPlayerState.h"
#include "GameFramework/PlayerState.h"
#include "GAS_Project/AMessage/LeeVerbMessage.h"
#include "GAS_Project/ATeam/LeeTeamAgentInterface.h"
#include "GAS_Project/System/LeeGameplayTagStack.h"
#include "LeePlayerState.generated.h"

UENUM()
enum class ELeePlayerConnectionType
{
	// 실행중인 플레이어
	Player = 0,

	// 관전 플레이어
	LiveSpectator,

	// 오프라인에서 데모 녹화??
	ReplaySpectator,

	// 비활성호된 플레이어 (연결끊김)
	InactivePlayer
};


UCLASS(Config=Game)
class GAS_PROJECT_API ALeePlayerState : public AModularPlayerState, public IAbilitySystemInterface, public  ILeeTeamAgentInterface
{
	GENERATED_BODY()

public:
	ALeePlayerState(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category="Lee|PlayerState")
	class ALeePlayerController* GetLeePlayerController();

	UFUNCTION(BlueprintCallable, Category="Lee|PlayerState")
	class ULeeAbilitySystemComponent* GetLeeAbilitySystemComponent() const
	{return AbilitySystemComponent ;}
	class UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	

	template<class T>
	const T* GetPawnData() const { return Cast<T>(PawnData); }
	
	void SetPawnData(const class ULeePawnData* InPawnData);


	//~AActor interface
	virtual void PreInitializeComponents() final;
	virtual void PostInitializeComponents() final;
	//~AActor interface

	//~APlayerState interface
	virtual void Reset() override;
	virtual void ClientInitialize(AController* C) override;
	virtual void CopyProperties(APlayerState* PlayerState) override;
	virtual void OnDeactivated() override;
	virtual void OnReactivated() override;
	//~APlayerState interface

	//~ILyraTeamAgentInterface interface
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual FOnLeeTeamIndexChangedDelegate* GetOnTeamIndexChangedDelegate() override;
	//~End of ILyraTeamAgentInterface interface

	static const FName NAME_LeeAbilityReady;

	void SetPlayerConnectionType(ELeePlayerConnectionType NewType);
	ELeePlayerConnectionType GetPlayerConnectionType() const {return MyPlayerConnectionType;}


	// 팀 > 분대
	// 플레이어가 속한 분대 ID 반환
	UFUNCTION(BlueprintCallable)
	int32 GetSquadId() const { return MySquadID;}

	// 플레이어가 속한 팀 ID 반환
	UFUNCTION(BlueprintCallable)
	int32 GetTeamId()const { return GenericTeamIdToInteger(MyTeamID); }


	void SetSquadId(int32 NewSquadId);

	// 태그에 지정된 수만큼 스택을 추가 ( 1미만이면 아무작업도 안함)
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Teams)
	void AddStatTagStack(FGameplayTag Tag, int32 StackCount);

	// 태그에 지정된 수만큼 스택을 제거
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Teams)
	void RemoveStatTagStack(FGameplayTag Tag, int32 StackCount);

	// 지정된 태그의 스택 수 반환
	UFUNCTION(BlueprintCallable, Category=Teams)
	int32 GetStatTagStackCount(FGameplayTag Tag) const;

	// 지정된 태그의 스택이 하나 이상 존재하면 True 반환
	UFUNCTION(BlueprintCallable, Category=Teams)
	bool HasStatTag(FGameplayTag Tag) const;

	// 이 플레이어에게만 메세지 전송
	UFUNCTION(Client, Unreliable, BlueprintCallable, Category="Lee|PlayerState")
	void ClientBroadcastMessage(const FLeeVerbMessage Message);

	// 관전에 사용되는 , 플레이어의 복제된 뷰 회전값을 반환
	FRotator GetReplicatedViewRotation() const;

	// 복제된 뷰 회전값을 설정 (서버에서만 유효하다)
	void SetReplicatedViewRotation(const FRotator& NewRotation);

private:
	void OnExperienceLoaded(const class ULeeExperienceDefinition* CurrentExperience);


protected:
	UFUNCTION()
	void OnRep_PawnData();
	
	UPROPERTY(ReplicatedUsing = OnRep_PawnData)
	TObjectPtr<const class ULeePawnData> PawnData;


private:
	UPROPERTY(VisibleAnywhere, Category="Lee|PlayerState")
	TObjectPtr<ULeeAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<const class ULeeSoulsStatSet> HealthSet;

	UPROPERTY(Replicated)
	ELeePlayerConnectionType MyPlayerConnectionType;

	UPROPERTY()
	FOnLeeTeamIndexChangedDelegate OnTeamChangedDelegate;

	UPROPERTY(ReplicatedUsing=OnRep_MyTeamID)
	FGenericTeamId MyTeamID;

	UPROPERTY(ReplicatedUsing=OnRep_MySquadID)
	int32 MySquadID;

	UPROPERTY(Replicated)
	FLeeGameplayTagStackContainer StatTags;

	UPROPERTY(Replicated)
	FRotator ReplicatedViewRotation;
	




	
private:
	UFUNCTION()
	void OnRep_MyTeamID(FGenericTeamId OldTeamID);

	UFUNCTION()
	void OnRep_MySquadID();
	
};

