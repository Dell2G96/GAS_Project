// CPlayerCharacter.cpp
#include "CPlayerCharacter.h"

#include "AbilitySystemComponent.h"
#include "CPlayerController.h"
#include "CPlayerState.h"
#include "EnhancedInputSubsystems.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GAS_Project/Components/CWeaponComponent.h"
#include "GAS_Project/GAS/CAbilitySystemComponent.h"
#include "GAS_Project/GAS/CAttributeSet.h"

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
    CAbilitySystemComponent = nullptr;
    CAttributeSet = nullptr;
}

void ACPlayerCharacter::BeginPlay()
{
    Super::BeginPlay();

    OwnerController = Cast<ACPlayerController>(GetController());
    CachedPlayerState = GetPlayerState<ACPlayerState>();
}

void ACPlayerCharacter::ServerSideInit()
{
    if (bAbilitySystemInitialized)
    {
        return;
    }
    ACPlayerState* PS = GetPlayerState<ACPlayerState>();
    if (!PS)
    {
        return;
    }
    if (UCAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent())
    {
        //ASC->ServerSideInit();
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
    return Super::GetAbilitySystemComponent();
}

UAttributeSet* ACPlayerCharacter::GetAttributeSet() const
{
    ACPlayerState* PS = GetPlayerState<ACPlayerState>();

    if (PS)
    {
        return PS->GetAttributeSet();
    }
    return Super::GetAttributeSet();
}

void ACPlayerCharacter::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);

    ACPlayerState* PS = GetPlayerState<ACPlayerState>();
    PS = GetPlayerState<ACPlayerState>();
    
    check(PS);
    
    if (UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent())
    {
        ASC->InitAbilityActorInfo(PS,this);
        if (HasAuthority())
        {
            if (auto* MyASC = Cast<UCAbilitySystemComponent>(ASC))
            {
                //MyASC->ServerSideInit();
            }
        }
    }
}

void ACPlayerCharacter::OnRep_PlayerState()
{
    Super::OnRep_PlayerState();
    ACPlayerState* PS = GetPlayerState<ACPlayerState>();
    if (!PS) return;

    if (UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent())
    {
        ASC->InitAbilityActorInfo(PS, this);   // 클라 동기화
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
