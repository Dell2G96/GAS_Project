// CCharacter.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "AttributeSet.h"
#include "GameplayTagContainer.h"
#include "CCharacter.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FASCInitialized, UAbilitySystemComponent*, ASC, UAttributeSet*, AS);

UCLASS(Abstract)
class GAS_PROJECT_API ACCharacter : public ACharacter, public IAbilitySystemInterface
{
    GENERATED_BODY()

public:
    ACCharacter();

    // 네트워킹
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // 생사 처리
    bool IsAlive() const { return bAlive; }
    void SetAlive(bool bAliveStatus) { bAlive = bAliveStatus; }

    UFUNCTION(BlueprintCallable, Category="GAS|Death")
    virtual void HandleRespawn();

    UFUNCTION(BlueprintCallable, Category="GAS|Death")
    void ResetAttributes();

protected:
    virtual void HandleDeath();
    void OnHealthChanged(const struct FOnAttributeChangeData& AttributeChangeData);

public:
    UFUNCTION(Server, Reliable, WithValidation)
    void Server_SendGameplayEventToSelf(const FGameplayTag& EventTag, const struct FGameplayEventData& EventData);

public:
    // ✅ 수정: 순수 가상 함수로 변경 (자식이 반드시 구현)
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
    virtual UAttributeSet* GetAttributeSet() const { return nullptr; }

    // ASC 초기화 브로드캐스트
    UPROPERTY(BlueprintAssignable)
    FASCInitialized OnASCInitialized;

protected:
    // 시작 능력/스탯 적용(서버 1회)
    void GiveStartUpAbilities();
    void InitAttributes() const;

protected:
    UPROPERTY(EditDefaultsOnly, Category="GAS|Abilities")
    TArray<TSubclassOf<class UGameplayAbility>> StartupAbilities;

    UPROPERTY(EditDefaultsOnly, Category="GAS|Effects")
    TSubclassOf<class UGameplayEffect> InitializeAttributesEffects;

    UPROPERTY(EditDefaultsOnly, Category="GAS|Effects")
    TSubclassOf<class UGameplayEffect> ResetAttributesEffects;

    UPROPERTY(BlueprintReadOnly, Replicated, Category="GAS|Attributes")
    bool bAlive = true;
};
