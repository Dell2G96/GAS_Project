// CPlayerCharacter.cpp
#include "CPlayerCharacter.h"

#include "AbilitySystemComponent.h"
#include "CPlayerState.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GAS_Project/Components/CWeaponComponent.h"
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
}

UAbilitySystemComponent* ACPlayerCharacter::GetAbilitySystemComponent() const
{
    if (const ACPlayerState* PS = Cast<ACPlayerState>(GetPlayerState()))
    {
        return PS->GetAbilitySystemComponent();
    }
    return nullptr;
}

UAttributeSet* ACPlayerCharacter::GetAttributeSet() const
{
    if (const ACPlayerState* PS = Cast<ACPlayerState>(GetPlayerState()))
    {
        return PS->GetAttributeSet();
    }
    return nullptr;
}

// ✅ 완전 수정: PossessedBy (서버)
void ACPlayerCharacter::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);

    UE_LOG(LogTemp, Warning, TEXT(">>> PossessedBy: START"));

    // ✅ 1단계: PlayerState 획득 및 체크
    ACPlayerState* PS = Cast<ACPlayerState>(GetPlayerState());
    if (!PS)
    {
        UE_LOG(LogTemp, Error, TEXT("PossessedBy: PlayerState is NULL!"));
        return;
    }

    // ✅ 2단계: ASC 획득
    UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent();
    if (!ASC)
    {
        UE_LOG(LogTemp, Error, TEXT("PossessedBy: ASC is NULL!"));
        return;
    }

    // ✅ 3단계: 서버 검증
    if (!HasAuthority())
    {
        UE_LOG(LogTemp, Warning, TEXT("PossessedBy: Not authority, skipping"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT(">>> PossessedBy: Initializing ASC"));

    // ✅ 4단계: ASC 초기화 (중요: PlayerState를 Owner로!)
    ASC->InitAbilityActorInfo(PS, this);
    //                        ↑   ↑
    //                    Owner Avatar
    //              (PlayerState) (Character)

    OnASCInitialized.Broadcast(ASC, GetAttributeSet());

    // ✅ 5단계: 어빌리티 부여 (서버에서만)
    UE_LOG(LogTemp, Warning, TEXT(">>> PossessedBy: Calling GiveStartUpAbilities"));
    GiveStartUpAbilities();

    // ✅ 6단계: 속성 초기화
    InitAttributes();

    // ✅ 7단계: Health 변화 이벤트 바인드
    if (const UCAttributeSet* AS = Cast<UCAttributeSet>(GetAttributeSet()))
    {
        ASC->GetGameplayAttributeValueChangeDelegate(AS->GetHealthAttribute())
            .AddUObject(this, &ACPlayerCharacter::OnHealthChanged);
        
        UE_LOG(LogTemp, Warning, TEXT(">>> PossessedBy: Health delegate bound"));
    }

    UE_LOG(LogTemp, Warning, TEXT(">>> PossessedBy: COMPLETE"));
}

// ✅ 완전 수정: OnRep_PlayerState (클라이언트)
void ACPlayerCharacter::OnRep_PlayerState()
{
    Super::OnRep_PlayerState();

    UE_LOG(LogTemp, Warning, TEXT(">>> OnRep_PlayerState: START (Client)"));

    // ✅ 1단계: PlayerState와 ASC 획득
    ACPlayerState* PS = Cast<ACPlayerState>(GetPlayerState());
    if (!PS)
    {
        UE_LOG(LogTemp, Error, TEXT("OnRep_PlayerState: PlayerState is NULL!"));
        return;
    }

    UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent();
    if (!ASC)
    {
        UE_LOG(LogTemp, Error, TEXT("OnRep_PlayerState: ASC is NULL!"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT(">>> OnRep_PlayerState: Initializing ASC on client"));

    // ✅ 2단계: ASC 초기화 (클라이언트)
    ASC->InitAbilityActorInfo(PS, this);
    //                        ↑   ↑
    //                    Owner Avatar

    OnASCInitialized.Broadcast(ASC, GetAttributeSet());

    // ✅ 3단계: Health 변화 이벤트 바인드
    if (const UCAttributeSet* AS = Cast<UCAttributeSet>(GetAttributeSet()))
    {
        ASC->GetGameplayAttributeValueChangeDelegate(AS->GetHealthAttribute())
            .AddUObject(this, &ACPlayerCharacter::OnHealthChanged);
        
        UE_LOG(LogTemp, Warning, TEXT(">>> OnRep_PlayerState: Health delegate bound"));
    }

    UE_LOG(LogTemp, Warning, TEXT(">>> OnRep_PlayerState: COMPLETE"));
}
