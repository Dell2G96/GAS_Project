// CCharacter.cpp
#include "CCharacter.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Net/UnrealNetwork.h"
#include "AbilitySystemComponent.h"
#include "GAS_Project/GAS/CAttributeSet.h"

ACCharacter::ACCharacter()
{
    PrimaryActorTick.bCanEverTick = false;
    
    // 전용서버에서도 본 업데이트
    GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
}

void ACCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ACCharacter, bAlive);
}

void ACCharacter::HandleRespawn()
{
    bAlive = true;
    UE_LOG(LogTemp, Log, TEXT("%s respawned"), *GetName());
}

void ACCharacter::HandleDeath()
{
    bAlive = false;
    if (IsValid(GEngine))
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, 
            FString::Printf(TEXT("%s has died!"), *GetName()));
    }
    UE_LOG(LogTemp, Warning, TEXT("%s has died!"), *GetName());
}

void ACCharacter::ResetAttributes()
{
    if (!IsValid(ResetAttributesEffects))
    {
        UE_LOG(LogTemp, Error, TEXT("ResetAttributes: ResetAttributesEffects not set"));
        return;
    }
    
    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    if (!ASC) return;

    FGameplayEffectContextHandle Ctx = ASC->MakeEffectContext();
    FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(ResetAttributesEffects, 1.f, Ctx);
    ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
}

void ACCharacter::OnHealthChanged(const FOnAttributeChangeData& Data)
{
    UE_LOG(LogTemp, Log, TEXT("OnHealthChanged: %f -> %f"), Data.OldValue, Data.NewValue);
    
    if (Data.NewValue <= 0.f && bAlive)
    {
        HandleDeath();
    }
}

UAbilitySystemComponent* ACCharacter::GetAbilitySystemComponent() const
{
    return nullptr;
}

void ACCharacter::Server_SendGameplayEventToSelf_Implementation(
    const FGameplayTag& EventTag, const FGameplayEventData& EventData)
{
    UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, EventTag, EventData);
    UE_LOG(LogTemp, Log, TEXT("[SERVER] SendGameplayEvent: %s"), *EventTag.ToString());
}

bool ACCharacter::Server_SendGameplayEventToSelf_Validate(
    const FGameplayTag& EventTag, const FGameplayEventData& EventData)
{
    return true;
}

// ✅ 수정: InputID를 포함한 어빌리티 부여
void ACCharacter::GiveStartUpAbilities()
{
    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    if (!ASC)
    {
        UE_LOG(LogTemp, Error, TEXT("GiveStartUpAbilities: ASC is NULL for %s"), *GetName());
        return;
    }

    if (!HasAuthority())
    {
        UE_LOG(LogTemp, Warning, TEXT("GiveStartUpAbilities: Not authority, skipping"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("=== GiveStartUpAbilities START for %s ==="), *GetName());

    for (const TSubclassOf<UGameplayAbility>& AbilityClass : StartupAbilities)
    {
        if (!AbilityClass)
        {
            UE_LOG(LogTemp, Warning, TEXT("  [SKIP] Null ability class"));
            continue;
        }

        // ✅ 중요: InputID는 Blueprint에서 설정해야 함
        // 기본적으로 INDEX_NONE으로 설정하고, Blueprint에서 오버라이드
        FGameplayAbilitySpec Spec(AbilityClass, 1, INDEX_NONE, this);
        FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(Spec);

        UE_LOG(LogTemp, Warning, TEXT("  [GRANTED] Ability: %s | Handle: %s"),
            *AbilityClass->GetName(),
            *Handle.ToString());
    }

    UE_LOG(LogTemp, Warning, TEXT("=== GiveStartUpAbilities END ==="));
}

void ACCharacter::InitAttributes() const
{
    if (!IsValid(InitializeAttributesEffects))
    {
        UE_LOG(LogTemp, Error, TEXT("InitAttributes: InitializeAttributesEffects not set"));
        return;
    }
    
    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    if (!ASC) return;

    FGameplayEffectContextHandle Ctx = ASC->MakeEffectContext();
    FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(InitializeAttributesEffects, 1.f, Ctx);
    ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
    
    UE_LOG(LogTemp, Log, TEXT("InitAttributes: Initialized for %s"), *GetOwner()->GetName());
}
