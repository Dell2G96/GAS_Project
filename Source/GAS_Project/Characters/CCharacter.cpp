// CCharacter.cpp
#include "CCharacter.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Net/UnrealNetwork.h"
#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "GAS_Project/GAS/CAbilitySystemComponent.h"
#include "GAS_Project/GAS/CAbilitySystemStatics.h"
#include "GAS_Project/GAS/CAttributeSet.h"
#include "GAS_Project/Widgets/OverHeadStatsGauge.h"
#include "Kismet/GameplayStatics.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"

ACCharacter::ACCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    
    
    // ✅ 기본적으로 생성 (AI 캐릭터용)
    // PlayerCharacter는 override에서 nullptr로 설정 가능
    CAbilitySystemComponent = CreateDefaultSubobject<UCAbilitySystemComponent>("CAbility System Component");
    CAttributeSet = CreateDefaultSubobject<UCAttributeSet>("CAttribute Set");
    
    OverHeadWidgetComponent = CreateDefaultSubobject<UWidgetComponent>("OverHead Widget Component");
    OverHeadWidgetComponent->SetupAttachment(GetRootComponent());
    
    BindGASChangeDelegate();
}

void ACCharacter::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ACCharacter, TeamID);
}

UAbilitySystemComponent* ACCharacter::GetAbilitySystemComponent() const
{
    return nullptr;
}

class UAttributeSet* ACCharacter::GetAttributeSet() const
{
    return nullptr;
}

void ACCharacter::Server_SendGameplayEventToSelf_Implementation(const FGameplayTag& EventTag,
                                                                const FGameplayEventData& EventData)
{
    UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, EventTag, EventData);
}

bool ACCharacter::Server_SendGameplayEventToSelf_Validate(const FGameplayTag& EventTag,
    const FGameplayEventData& EventData)
{
    return true;

}

void ACCharacter::ServerSideInit()
{
    CAbilitySystemComponent->InitAbilityActorInfo(this,this);
    CAbilitySystemComponent->ServerSideInit();
}

void ACCharacter::ClientSideInit()
{
    CAbilitySystemComponent->InitAbilityActorInfo(this,this);

}

bool ACCharacter::IsLocallyControlledByPlayer() const
{
    return GetController() && GetController()->IsLocalPlayerController();
}

const TMap<ECabilityInputID, TSubclassOf<class UGameplayAbility>>& ACCharacter::GetAbilities() const
{
    return CAbilitySystemComponent->GetAbilities();
}

void ACCharacter::BeginPlay()
{
    Super::BeginPlay();
    ConfigureOverHeadStatusWidget();
    MeshRelativeTransform = GetMesh()->GetRelativeTransform();
}

void ACCharacter::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);

    // 서버에서 실행될때 컨트롤러가 있고, 그게 픒레이어 가 아니라면 -> AI 컨트롤러라면
    if (NewController && !NewController->IsPlayerController())
    {
        ServerSideInit();
    }
}

void ACCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void ACCharacter::BindGASChangeDelegate()
{
    if (CAbilitySystemComponent)
    {
        CAbilitySystemComponent->RegisterGameplayTagEvent(UCAbilitySystemStatics::GetDeadStatTag()).AddUObject(this,&ACCharacter::DeathTagUpdated);
        CAbilitySystemComponent->RegisterGameplayTagEvent(UCAbilitySystemStatics::GetStunStatTag()).AddUObject(this,&ACCharacter::StunTagUpdated);
        
        CAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UCAttributeSet::GetMaxHealthAttribute()).AddUObject(this, &ACCharacter::MaxHealthUpdated);
        CAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UCAttributeSet::GetMaxManaAttribute()).AddUObject(this, &ACCharacter::MaxManaUpdated);
    }
}

void ACCharacter::DeathTagUpdated(const FGameplayTag Tag, int32 NewCount)
{
    if (NewCount != 0)
    {
        StartDeathSequence();
    }
    else
    {
        //Respawn();
        return;
    }
}

void ACCharacter::StunTagUpdated(const FGameplayTag Tag, int32 NewCount)
{
    if (IsDead()) return;
    if (NewCount != 0)
    {
        OnStun();
        PlayAnimMontage(StunMontage);
    }
    else
    {
        OnRecoverFromStun();			
        StopAnimMontage(StunMontage);
    }
}

// void ACCharacter::AimTagUpdated(const FGameplayTag Tag, int32 NewCount)
// {
//     SetIsAimming(NewCount != 0);
// }

void ACCharacter::MaxHealthUpdated(const FOnAttributeChangeData& Data)
{
    if (IsValid(CAttributeSet))
    {
        CAttributeSet->RescaleHealth();
    }
}

void ACCharacter::MaxManaUpdated(const FOnAttributeChangeData& Data)
{
    if (IsValid(CAttributeSet))
    {
        CAttributeSet->RescaleMana();
    }
}

void ACCharacter::ConfigureOverHeadStatusWidget()
{
    if (!OverHeadWidgetComponent)
        return;

    IsPlayerControlled();

    // 내 케릭터라면 머리 위 위젯은 숨겨야됨 , 화면 중앙아래에 있기때문
    if (IsLocallyControlledByPlayer())
    {
        OverHeadWidgetComponent->SetHiddenInGame(true);
        return;
    }
	
    UOverHeadStatsGauge* OverheadStatsGuage = Cast<UOverHeadStatsGauge>(OverHeadWidgetComponent->GetUserWidgetObject());
    if (OverheadStatsGuage)
    {
        OverheadStatsGuage->ConfigureWithASC(GetAbilitySystemComponent());
        OverHeadWidgetComponent->SetHiddenInGame(false);
        GetWorldTimerManager().ClearTimer(HeadStatGuageVisibilityUpdateTimerHandle);
        GetWorldTimerManager().SetTimer(HeadStatGuageVisibilityUpdateTimerHandle,
            this,
            &ACCharacter::UpdateHeadGuageVisibility,
            HeadStatuGaugeVisiblityCheckUpdateGap,
            true);
    }
}

void ACCharacter::UpdateHeadGuageVisibility()
{
    APawn*  LocalPlayerPawn = UGameplayStatics::GetPlayerPawn(this,0	);
    if (LocalPlayerPawn)
    {
        float DistSquared = FVector::DistSquared(GetActorLocation(), LocalPlayerPawn->GetActorLocation());
        OverHeadWidgetComponent->SetHiddenInGame(DistSquared > HeadStatuGaugeVisiblityRangeSquared);
    }
}

void ACCharacter::SetStatusGaugeEnable(bool bIsEnabled)
{
    GetWorldTimerManager().ClearTimer(HeadStatGuageVisibilityUpdateTimerHandle);
    if (bIsEnabled)
    {
        ConfigureOverHeadStatusWidget();
    }
    else
    {
        OverHeadWidgetComponent->SetHiddenInGame(true);
    }
}

bool ACCharacter::IsDead() const
{
    return GetAbilitySystemComponent()->HasMatchingGameplayTag(UCAbilitySystemStatics::GetDeadStatTag());
}

void ACCharacter::RespawnImmediately()
{
    if (HasAuthority())
    {
        GetAbilitySystemComponent()->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(UCAbilitySystemStatics::GetDeadStatTag())); 
    }
}

void ACCharacter::DeathMontageFinished()
{
    if (IsDead())
    {
        SetRagdollEnabled(true);
		 
    }
}

void ACCharacter::SetRagdollEnabled(bool bIsEnabled)
{
    if (bIsEnabled)
    {
        GetMesh()->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
        GetMesh()->SetSimulatePhysics(true);
        GetMesh()->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
    }
    else
    {
        GetMesh()->SetSimulatePhysics(false);
        GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        GetMesh()->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
        GetMesh()->SetRelativeTransform(MeshRelativeTransform);
    }
}

void ACCharacter::PlayDeathAnim()
{
    if (DeathMontage)
    {
        float MontageDuration = PlayAnimMontage(DeathMontage);
        GetWorldTimerManager().SetTimer(DeathMontageTimerHandle, this, &ACCharacter::DeathMontageFinished, MontageDuration + DeathMontageFinishTimerShift);
    }
}

void ACCharacter::StartDeathSequence()
{
    
    OnDead();
    if (CAbilitySystemComponent)
    {
        CAbilitySystemComponent->CancelAllAbilities();
    }
    PlayDeathAnim();
    SetStatusGaugeEnable(false);

    //GetCharacterMovement()->SetMovementMode(MOVE_None);
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

}

void ACCharacter::Respawn()
{
    OnRespawn();
    //SetAIPerceptionStimuliSourceEnabled(true);
    SetRagdollEnabled(false);
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    //GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    GetMesh()->GetAnimInstance()->StopAllMontages(0.f);
    SetStatusGaugeEnable(true);

    if (HasAuthority() && GetController())
    {
        TWeakObjectPtr<AActor> StartSpot = GetController()->StartSpot;
        if (StartSpot.IsValid())
        {
            SetActorTransform(StartSpot->GetActorTransform());
        }
    }

    if (CAbilitySystemComponent)
    {
        CAbilitySystemComponent->ApplyFullStatEffect();
    }
}

void ACCharacter::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
    TeamID = NewTeamID;
}

FGenericTeamId ACCharacter::GetGenericTeamId() const
{
    return TeamID;
}

void ACCharacter::OnDead()
{
    //Overide In child		
}

void ACCharacter::OnRespawn()
{
    //Overide In child		
}


void ACCharacter::OnRep_TeamID()
{
    //Overide Inchild		
}

void ACCharacter::OnStun()
{
    //Overide Inchild		
}

void ACCharacter::OnRecoverFromStun()
{
    //Overide Inchild		
}