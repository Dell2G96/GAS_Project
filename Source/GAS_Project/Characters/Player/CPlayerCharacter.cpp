// CPlayerCharacter.cpp
#include "CPlayerCharacter.h"

#include "AbilitySystemComponent.h"
#include "CPlayerController.h"
#include "CPlayerState.h"
#include "EnhancedInputSubsystems.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/Components/CWeaponComponent.h"
#include "GAS_Project/GAS/CAbilitySystemComponent.h"
#include "GAS_Project/GAS/CAbilitySystemStatics.h"
#include "GAS_Project/GAS/CAttributeSet.h"
#include "GAS_Project/GAS/Abilities/GA_Dead.h"
#include "GAS_Project/Widgets/OverHeadStatsGauge.h"
#include "Kismet/GameplayStatics.h"

ACPlayerCharacter::ACPlayerCharacter()
{
    PrimaryActorTick.bCanEverTick = false;

    GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);

    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    UCharacterMovementComponent* Move = GetCharacterMovement();
    Move->bOrientRotationToMovement = true;
    Move->RotationRate = FRotator(0.f, 540.f, 0.f);
    Move->JumpZVelocity = 500.f;
    Move->AirControl = 0.35f;
    Move->MaxWalkSpeed = 400.f;
    Move->MinAnalogWalkSpeed = 20.f;
    Move->BrakingDecelerationWalking = 2000.f;
    Move->BrakingDecelerationFalling = 1500.f;

    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(GetRootComponent());
    CameraBoom->TargetArmLength = 600.f;
    CameraBoom->bUsePawnControlRotation = true;

    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    Camera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    Camera->bUsePawnControlRotation = false;

    WeaponComponent = CreateDefaultSubobject<UCWeaponComponent>(TEXT("WeaponComponent"));
    WeaponComponent->SetIsReplicated(true);
    
}

void ACPlayerCharacter::BeginPlay()
{
    Super::BeginPlay();

    OwnerController = Cast<ACPlayerController>(GetController());
    CachedPlayerState = GetPlayerState<ACPlayerState>();
    ConfigureOverHeadStatusWidget();
}


void ACPlayerCharacter::ServerSideInit()
{
    ACPlayerState* PS = GetPlayerState<ACPlayerState>();
    if (!PS)
    {
        return;
    }
    CAbilitySystemComponent = Cast<UCAbilitySystemComponent>(PS->GetAbilitySystemComponent());
    if (CAbilitySystemComponent)
    {
        CAbilitySystemComponent->ServerSideInit();
        CAbilitySystemComponent->InitAbilityActorInfo(PS,this);
    }
    
}

void ACPlayerCharacter::ClientSideInit()
{
    ACPlayerState* PS = GetPlayerState<ACPlayerState>();

    if (!PS)
    {
        return;
    }
    CAbilitySystemComponent = Cast<UCAbilitySystemComponent>(PS->GetAbilitySystemComponent());
    if (CAbilitySystemComponent)
    {
        CAbilitySystemComponent->InitAbilityActorInfo(PS,this);
    }
 }

void ACPlayerCharacter::BindGASChangeDelegate()
{
    Super::BindGASChangeDelegate();

    CAbilitySystemComponent = Cast<UCAbilitySystemComponent>(GetAbilitySystemComponent());
    if (CAbilitySystemComponent)
    {
        CAbilitySystemComponent->RegisterGameplayTagEvent(UCAbilitySystemStatics::GetDeadStatTag()).AddUObject(this,&ThisClass::DeathTagUpdated);
    }
   
}

void ACPlayerCharacter::DeathTagUpdated(const FGameplayTag Tag, int32 NewCount)
{
    //Super::DeathTagUpdated(Tag, NewCount);
    //UE_LOG(LogTemp,Warning,TEXT("ACCharacter::StartDeathSequence"));
    OnDead();
    // if (CAbilitySystemComponent)
    // {
    //     CAbilitySystemComponent->CancelAllAbilities();
    // }
  //  PlayDeathAnim();
    SetStatusGaugeEnable(false);
    
    //GetCharacterMovement()->SetMovementMode(MOVE_None);
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ACPlayerCharacter::MaxHealthUpdated(const struct FOnAttributeChangeData& Data)
{
    Super::MaxHealthUpdated(Data);
}

void ACPlayerCharacter::MaxManaUpdated(const struct FOnAttributeChangeData& Data)
{
    Super::MaxManaUpdated(Data);
}

void ACPlayerCharacter::PawnClientRestart()
{
    Super::PawnClientRestart();  // ✅ 부모 호출 (중요!)

    // ✅ PS 할당
    ACPlayerState* PS = Cast<ACPlayerState>(GetPlayerState());
    
    if (!PS)
    {
        UE_LOG(LogTemp, Error, TEXT("ClientSideInit: PlayerState is NULL!"));
        return;
    }
    
    // ✅ PlayerState의 ASC 사용
    if (GetAbilitySystemComponent())
    {
        GetAbilitySystemComponent()->InitAbilityActorInfo(PS, this);
    }
    
    // ✅ 클라이언트 컨트롤러에서 Input 재설정
    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = 
            ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
        {
            UE_LOG(LogTemp, Warning, TEXT("PawnClientRestart: Input subsystem ready"));
        }
    }
}


UAbilitySystemComponent* ACPlayerCharacter::GetAbilitySystemComponent() const
{
    ACPlayerState* PS = Cast<ACPlayerState>(GetPlayerState());
    if (!IsValid(PS)) return nullptr;
    
    return PS->GetAbilitySystemComponent();
}

UAttributeSet* ACPlayerCharacter::GetAttributeSet() const
{
    ACPlayerState* PS = Cast<ACPlayerState>(GetPlayerState());
    if (!IsValid(PS)) return nullptr;

    return PS->GetAttributeSet();
}

void ACPlayerCharacter::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);

    if (!IsValid(GetAbilitySystemComponent()) || !HasAuthority()) return;

    GetAbilitySystemComponent()->InitAbilityActorInfo(GetPlayerState(), this);
    OnASCInitialized.Broadcast(GetAbilitySystemComponent(), GetAttributeSet());

    if (IsValid(GetAbilitySystemComponent()))
    {
        ConfigureOverHeadStatusWidget();
        //CAbilitySystemComponent->ServerSideInit();
        //BindGASChangeDelegate();
    }
    
    CAttributeSet = Cast<UCAttributeSet>(GetAttributeSet());
    if (!IsValid(CAttributeSet)) return;

    GetAbilitySystemComponent()->GetGameplayAttributeValueChangeDelegate(CAttributeSet->GetHealthAttribute()).AddUObject(this, &ThisClass::OnHealthChanged);
    
   
}

void ACPlayerCharacter::OnRep_PlayerState()
{
    Super::OnRep_PlayerState();
    
    if (!IsValid(GetAbilitySystemComponent())) return;

    GetAbilitySystemComponent()->InitAbilityActorInfo(GetPlayerState(), this);
    OnASCInitialized.Broadcast(GetAbilitySystemComponent(), GetAttributeSet());


    CAttributeSet = Cast<UCAttributeSet>(GetAttributeSet());
    if (!IsValid(CAttributeSet)) return;
	
    GetAbilitySystemComponent()->GetGameplayAttributeValueChangeDelegate(CAttributeSet->GetHealthAttribute()).AddUObject(this, &ThisClass::OnHealthChanged);
    GetAbilitySystemComponent()->GetGameplayAttributeValueChangeDelegate(CAttributeSet->GetStaminaAttribute()).AddUObject(this, &ThisClass::OnStaminaChanged);

    ConfigureOverHeadStatusWidget();
    
    // ACPlayerState* PS = GetPlayerState<ACPlayerState>();
    //
    // if (!PS)
    // {
    //     UE_LOG(LogTemp,Warning,TEXT("ClientSideInit: PlayerState is NULL!"));
    //     return;
    // }
    // CAbilitySystemComponent = Cast<UCAbilitySystemComponent> (PS->GetAbilitySystemComponent());
    // if (CAbilitySystemComponent)
    // {
    //     CAbilitySystemComponent->InitAbilityActorInfo(PS, this);   // 클라 동기화
    //     ConfigureOverHeadStatusWidget();
    //     BindGASChangeDelegate();
    // }
    
}

void ACPlayerCharacter::OnStun()
{
    //ToDo CPlayerController
}

void ACPlayerCharacter::OnRecoverFromStun()
{
    //ToDo CPlayerController
}


void ACPlayerCharacter::OnDead()
{
    UE_LOG(LogTemp,Warning,TEXT("//----------ClientSideDead----------//"));
    
    //PlayAnimMontage(DeathMontage);
    ACPlayerController* PC = Cast<ACPlayerController>(GetController());
    if (PC)
    {
        DisableInput(PC);
    }
}

void ACPlayerCharacter::OnRespawn()
{
    ACPlayerController* PC = Cast<ACPlayerController>(GetController());
    if (PC)
    {
        EnableInput(PC);
    }
}
