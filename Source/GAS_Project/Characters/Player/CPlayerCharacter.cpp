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
#include "GAS_Project/MyTags.h"
#include "GAS_Project/Components/CWeaponComponent.h"
#include "GAS_Project/GAS/CAbilitySystemComponent.h"
#include "GAS_Project/GAS/CAbilitySystemStatics.h"
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
    WeaponComponent->SetIsReplicated(true);
    
}

void ACPlayerCharacter::BeginPlay()
{
    Super::BeginPlay();

    OwnerController = Cast<ACPlayerController>(GetController());
    CachedPlayerState = GetPlayerState<ACPlayerState>();
    ConfigureOverHeadStatusWidget();
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
    }
    
}


void ACPlayerCharacter::OnRep_PlayerState()
{
    Super::OnRep_PlayerState();
    
    if (!IsValid(GetAbilitySystemComponent())) return;

    GetAbilitySystemComponent()->InitAbilityActorInfo(GetPlayerState(), this);
    OnASCInitialized.Broadcast(GetAbilitySystemComponent(), GetAttributeSet());

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
        BindGASChangeDelegate();
        CAbilitySystemComponent->ServerSideInit();
        CAbilitySystemComponent->InitAbilityActorInfo(PS,this);
        CAbilitySystemComponent->RegisterGameplayTagEvent(UCAbilitySystemStatics::GetKnockdownStatTag()).AddUObject(this,&ACPlayerCharacter::KnockdownTagUpdated);

    }
}

void ACPlayerCharacter::ClientSideInit()
{
    // PlayerController 에서 호출
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


void ACPlayerCharacter::MaxHealthUpdated(const struct FOnAttributeChangeData& Data)
{
    Super::MaxHealthUpdated(Data);
}

void ACPlayerCharacter::MaxStaminaUpdated(const struct FOnAttributeChangeData& Data)
{
    Super::MaxStaminaUpdated(Data);
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



void ACPlayerCharacter::OnStun()
{
    //ToDo CPlayerController
}

void ACPlayerCharacter::OnRecoverFromStun()
{
    //ToDo CPlayerController
}

bool ACPlayerCharacter::IsKnockedDown() const
{
    return GetAbilitySystemComponent()->HasMatchingGameplayTag(MyTags::Status::Knockdown);
}

void ACPlayerCharacter::KnockdownTagUpdated(const FGameplayTag Tag, int32 NewCount)
{
    //여기서 NewCount 는 = Stack Count이다 즉 1이상이면 죽은것... Dead Effect가 증가하므로? 
    UE_LOG(LogTemp,Warning,TEXT("=== KnockdownTag ==="));
    if (NewCount != 0)
    {
        StartKnockdownSequence();
    }
    else
    {
        UE_LOG(LogTemp,Warning,TEXT("=== KnockdownTag ==="));
    }
}

void ACPlayerCharacter::StartKnockdownSequence()
{
    OnKnockdown();
    if (CAbilitySystemComponent)
    {
        CAbilitySystemComponent->CancelAllAbilities();
    
        FGameplayTagContainer Tag;
        Tag.AddTag(MyTags::Status::Knockdown);

        GetAbilitySystemComponent()->TryActivateAbilitiesByTag(Tag, false);
    }
}

void ACPlayerCharacter::OnKnockdown()
{
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
