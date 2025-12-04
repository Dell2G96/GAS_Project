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
#include "GAS_Project/Components/CWeaponComponent.h"
#include "GAS_Project/GAS/CAbilitySystemComponent.h"
#include "GAS_Project/GAS/CAbilitySystemStatics.h"
#include "GAS_Project/GAS/CAttributeSet.h"
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
    ACPlayerCharacter* OwnerCharacter = Cast<ACPlayerCharacter>(GetController());
    if (!PS)
    {
        return;
    }
    if (UCAbilitySystemComponent* ASC = Cast<UCAbilitySystemComponent>(PS->GetAbilitySystemComponent()))
    {
       // ASC->ServerSideInit();
        ASC->InitAbilityActorInfo(PS,this);
    }
    
}

void ACPlayerCharacter::ClientSideInit()
{
    ACPlayerState* PS = GetPlayerState<ACPlayerState>();

    if (!PS)
    {
        return;
    }
    if (UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent())
    {
        ASC->InitAbilityActorInfo(PS,this);
    }
 }

void ACPlayerCharacter::BindGASChangeDelegate()
{
    Super::BindGASChangeDelegate();

    CAbilitySystemComponent = Cast<UCAbilitySystemComponent>(GetAbilitySystemComponent());
    if (CAbilitySystemComponent)
    {
        CAbilitySystemComponent->RegisterGameplayTagEvent(UCAbilitySystemStatics::GetDeadStatTag()).AddUObject(this,&ACPlayerCharacter::DeathTagUpdated);
        
        CAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UCAttributeSet::GetMaxHealthAttribute()).AddUObject(this, &ACPlayerCharacter::MaxHealthUpdated);
        CAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UCAttributeSet::GetMaxManaAttribute()).AddUObject(this, &ACPlayerCharacter::MaxManaUpdated);
    }
}

void ACPlayerCharacter::DeathTagUpdated(const FGameplayTag Tag, int32 NewCount)
{
    Super::DeathTagUpdated(Tag, NewCount);
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
    ACPlayerState* PS = GetPlayerState<ACPlayerState>();
    
    if (!PS)
    {
        UE_LOG(LogTemp, Error, TEXT("ClientSideInit: PlayerState is NULL!"));
        return;
    }
    
    // ✅ PlayerState의 ASC 사용
    if (UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent())
    {
        ASC->InitAbilityActorInfo(PS, this);
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
    ACPlayerState* PS = GetPlayerState<ACPlayerState>();

    if (PS)
    {
        return PS->GetAbilitySystemComponent();
    }
    return nullptr;
}

UAttributeSet* ACPlayerCharacter::GetAttributeSet() const
{
    ACPlayerState* PS = GetPlayerState<ACPlayerState>();

    if (PS)
    {
        return PS->GetAttributeSet();
    }
    return nullptr;
}

void ACPlayerCharacter::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);

    ACPlayerState* PS = GetPlayerState<ACPlayerState>();
    //check(PS);
    if (!PS)
    {
        return;
    }

    CAbilitySystemComponent = Cast<UCAbilitySystemComponent> (PS->GetAbilitySystemComponent());

    if (CAbilitySystemComponent)
    {
        CAbilitySystemComponent->InitAbilityActorInfo(PS,this);
        ConfigureOverHeadStatusWidget();
        CAbilitySystemComponent->ServerSideInit();
        BindGASChangeDelegate();
    }
}

void ACPlayerCharacter::OnRep_PlayerState()
{
    Super::OnRep_PlayerState();
    ACPlayerState* PS = GetPlayerState<ACPlayerState>();
    
    if (!PS)
    {
        UE_LOG(LogTemp,Warning,TEXT("ClientSideInit: PlayerState is NULL!"));
        return;
    }
    CAbilitySystemComponent = Cast<UCAbilitySystemComponent> (PS->GetAbilitySystemComponent());
    if (CAbilitySystemComponent)
    {
        CAbilitySystemComponent->InitAbilityActorInfo(PS, this);   // 클라 동기화
        ConfigureOverHeadStatusWidget();
    }
    
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
    //ToDo CPlayerController
}

void ACPlayerCharacter::OnRespawn()
{
    //ToDo CPlayerController
}
