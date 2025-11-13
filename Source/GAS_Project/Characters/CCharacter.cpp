#include "CCharacter.h"
#include "GAS_Project/GAS/CAbilitySystemComponent.h"
#include "GAS_Project/GAS/CAttributeSet.h"
#include "GAS_Project/GAS/Abilities/CGameplayAbility.h"
#include "Net/UnrealNetwork.h"



ACCharacter::ACCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	//렌더링 여부와 관계없이 본(뼈대) 트랜스폼을 틱(tick)하고 새로 고침 — Dedicate 서버에서 본 업데이트를 수행하기 위함
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
}

void ACCharacter::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, bAlive)
}

UAbilitySystemComponent* ACCharacter::GetAbilitySystemComponent() const
{
	return nullptr;

}

void ACCharacter::GiveStartUpAbilities()
{
	if (!IsValid(GetAbilitySystemComponent())) return;

	for (const auto& Ability : StartupAbilities)
	{
		FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(Ability);
		GetAbilitySystemComponent()->GiveAbility(AbilitySpec);
	}
}

void ACCharacter::InitAttributes() const
{
	checkf(IsValid(InitializeAttributesEffects), TEXT("InitializeAttributeEffect not Set"));

	FGameplayEffectContextHandle ContextHandle = GetAbilitySystemComponent()->MakeEffectContext();
	FGameplayEffectSpecHandle SpecHandle = GetAbilitySystemComponent()->MakeOutgoingSpec(InitializeAttributesEffects, 1.f,ContextHandle);
	GetAbilitySystemComponent()->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}

void ACCharacter::OnHealthChanged(const struct FOnAttributeChangeData& AttributeChangeData)
{
	if (AttributeChangeData.NewValue <= 0.f)
	{
		HandleDeath();
	}
}

void ACCharacter::HandleRespawn()
{
	bAlive = true;

}

void ACCharacter::HandleDeath()
{
	bAlive = false;
	if (IsValid(GEngine))
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, FString::Printf(TEXT("%s has died!"), *GetName()));
	}
}

void ACCharacter::ResetAttributes()
{
	checkf(IsValid(ResetAttributesEffects), TEXT("InitializeAttributeEffect not Set"));

	FGameplayEffectContextHandle ContextHandle = GetAbilitySystemComponent()->MakeEffectContext();
	FGameplayEffectSpecHandle SpecHandle = GetAbilitySystemComponent()->MakeOutgoingSpec(ResetAttributesEffects, 1.f,ContextHandle);
	GetAbilitySystemComponent()->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}