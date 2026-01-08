// CPlayerCharacter.cpp
#include "CPlayerCharacter.h"

#include "AbilitySystemComponent.h"
#include "CPlayerController.h"
#include "CPlayerState.h"
#include "EnhancedInputSubsystems.h"
#include "MotionWarpingComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/Components/CWeaponComponent.h"
#include "GAS_Project/GAS/CAbilitySystemComponent.h"
#include "GAS_Project/GAS/CAbilitySystemStatics.h"
#include "GAS_Project/GAS/CAttributeSet.h"
#include "Net/UnrealNetwork.h"

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

    // 런타임에 빈 이름(None)으로 들어가서 워핑이 안 걸리는 걸 방지합니다.
    RollWarpTargetName = FName(TEXT("RollingDirection"));
}


void ACPlayerCharacter::BeginPlay()
{
    Super::BeginPlay();

    OwnerController = Cast<ACPlayerController>(GetController());
    CachedPlayerState = GetPlayerState<ACPlayerState>();
    ConfigureOverHeadStatusWidget();
    SetGenericTeamId(1);
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

void ACPlayerCharacter::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ThisClass, bKnockdown);
    DOREPLIFETIME(ThisClass, bIsTargetLocked);
    DOREPLIFETIME(ThisClass, TargetLockActor);
    
    // DOREPLIFETIME(ThisClass, RollWarpData);
    
}

void ACPlayerCharacter::ApplyRollWarpTarget_Local(const FVector& Dir2D, float Distance)
{
    if (!MotionWarpingComponent)
    {
        return;
        // 모션워핑 컴포넌트가 없으면 적용 불가입니다.
    }

    FVector Dir = Dir2D;
    // 입력 방향을 복사합니다.

    Dir.Z = 0.f;
    // 수평 방향으로 고정합니다.

    if (Dir.SizeSquared() < 0.0001f)
    {
        Dir = GetActorForwardVector();
        // 방향이 0에 가까우면 전방으로 방어합니다.

        Dir.Z = 0.f;
        // 전방도 수평으로 고정합니다.
    }

    Dir.Normalize();
    // 정규화합니다.

    const float SafeDistance = FMath::Max(0.f, Distance);
    // 음수 거리를 방어합니다.

    const FVector TargetLocation = GetActorLocation() + (Dir * SafeDistance);
    // 목표 위치를 계산합니다.

    const FRotator TargetRotation = FRotationMatrix::MakeFromX(Dir).Rotator();
    // 목표 회전을 계산합니다.

    MotionWarpingComponent->AddOrUpdateWarpTargetFromLocationAndRotation(
        RollWarpTargetName,
        TargetLocation,
        TargetRotation);
    // 워프 타깃을 업데이트합니다.
}

void ACPlayerCharacter::Server_SetRollWarpData_Implementation(const FVector_NetQuantizeNormal& Direction,
    float Distance)
{
    const FVector_NetQuantizeNormal SafeDir = (Direction.IsNearlyZero())
        ? FVector_NetQuantizeNormal(GetActorForwardVector())
        : Direction;
    // 서버에서 방향이 0이면 전방으로 방어합니다.

    const float SafeDistance = FMath::Max(0.f, Distance);
    // 서버에서 거리도 방어합니다.

    ApplyRollWarpTarget_Local(SafeDir, SafeDistance);
    // 서버에서도 워프 타깃을 세팅합니다.

    Multicast_ApplyRollWarpData(SafeDir, SafeDistance);
    // 모든 클라에 워프 타깃을 적용합니다.
}

void ACPlayerCharacter::Multicast_ApplyRollWarpData_Implementation(const FVector_NetQuantizeNormal& Direction,
    float Distance)
{
    ApplyRollWarpTarget_Local(Direction, Distance);
    // 각 클라에서 워프 타깃을 세팅합니다.
}

//
// void ACPlayerCharacter::ApplyRollWarpTarget_Local(const FVector& Dir2D, float Distance)
// {
//     if (!MotionWarpingComponent) return;
//
//     FVector Dir = Dir2D;                    // 입력 방향 복사
//     Dir.Z = 0.f;                            // Z축 성분 제거
//     if (Dir.SizeSquared() < 0.0001f)
//     {
//         Dir = GetActorForwardVector(); // 방향이 너무 작으면 전방 방향 사용
//         Dir.Z = 0.f;
//     }
//
//     Dir.Normalize();
//
//     // 현재 위치에서 거리만큼 이동한 지점을 워프타깃으로 지정
//     const FVector TargetLocation = GetActorLocation() + (Dir * Distance);
//
//     const FRotator TargetRotation = FRotationMatrix::MakeFromX(Dir).Rotator();
//
//     MotionWarpingComponent->AddOrUpdateWarpTargetFromLocationAndRotation(RollWarpTargetName, TargetLocation, TargetRotation);
//     
// }
//
// void ACPlayerCharacter::Multicast_ApplyRollWarpData_Implementation(const FVector_NetQuantizeNormal& Direction,
//     float Distance)
// {
// }
//
// void ACPlayerCharacter::Server_SetRollWarpData_Implementation(const FVector_NetQuantizeNormal& Direction,
//                                                               float Distance)
// {
//     RollWarpData.Direction = Direction;
//     // 서버에 방향을 저장합니다.
//
//     RollWarpData.Distance = Distance;
//     // 서버에 거리를 저장합니다.
//
//     RollWarpData.Sequence++;
//     // 같은 값이 반복돼도 변경으로 인식되도록 시퀀스를 증가시킵니다.
//
//     ApplyRollWarpTarget_Local(RollWarpData.Direction, RollWarpData.Distance);
//     // 서버에서도 워프 타깃을 즉시 반영합니다.
//
//     Multicast_ApplyRollWarpWarpData(RollWarpData.Direction, RollWarpData.Distance, RollWarpData.Sequence);
//     // 모든 클라에 즉시 반영하도록 멀티캐스트를 호출합니다.
//
//     ForceNetUpdate();
//     // 네트워크 업데이트를 당겨서 타이밍 꼬임을 줄입니다.
// }
//
// void ACPlayerCharacter::Multicast_ApplyRollWarpWarpData_Implementation(const FVector_NetQuantizeNormal& Direction,
//     float Distance,uint8 Sequence)
// {
//     ApplyRollWarpTarget_Local(Direction, Distance);
//     // 각 머신에서 워프 타깃을 적용합니다.
// }
//
// void ACPlayerCharacter::OnRep_RollWarpData()
// {
//     ApplyRollWarpTarget_Local(RollWarpData.Direction, RollWarpData.Distance);
//
// }
//
// uint8 ACPlayerCharacter::GetRollWarpSequence()
// {
//     return RollWarpData.Sequence;
//     // 복제 도착 시에도 워프 타깃을 재적용
// }
//
// FVector ACPlayerCharacter::GetRollWarpDirection()
// {
//     return RollWarpData.Direction;
//     // 저장된 방향을 반환
// }
//
// float ACPlayerCharacter::GetRollWarpDistance()
// {
//     return RollWarpData.Distance;
// }


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
        //StartKnockdownSequence();
        EnterKnockdown();
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


void ACPlayerCharacter::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
   Super::SetGenericTeamId(NewTeamID);
}

FGenericTeamId ACPlayerCharacter::GetGenericTeamId() const
{
    return Super::GetGenericTeamId();
}

void ACPlayerCharacter::EnterKnockdown()
{
    if (!HasAuthority()) return;
    
    if (!CAbilitySystemComponent) return;

    CAbilitySystemComponent->ApplyKnockdown();

    KnockdownRemainingTime = KnockdownTotalTime;
    StartKnockdownTimer(KnockdownRemainingTime);
}

void ACPlayerCharacter::BeginRevive(AActor* Reviver)
{
    if (!HasAuthority()) return;
    if (bBeingRevived) return;

    bBeingRevived = true;

    const float TimeLeft =GetWorldTimerManager().GetTimerRemaining(KnockdownTimerHandle);

    KnockdownRemainingTime = TimeLeft;
    GetWorldTimerManager().ClearTimer(KnockdownTimerHandle);
}

void ACPlayerCharacter::CancleRevive()
{
    if (!HasAuthority()) return;
    if (!bBeingRevived) return;

    bBeingRevived = false;
    StartKnockdownTimer(KnockdownRemainingTime);
}

void ACPlayerCharacter::CompleteRevive()
{
    if (!HasAuthority()) return;
    
    if (!CAbilitySystemComponent) return;

    bBeingRevived = false;
    CAbilitySystemComponent->TryRevive();
}

void ACPlayerCharacter::StartKnockdownTimer(float Time)
{
    GetWorldTimerManager().SetTimer(KnockdownTimerHandle,this,&ACPlayerCharacter::OnKnockdownExpired,Time,false);
}

void ACPlayerCharacter::OnKnockdownExpired()
{
    if (!HasAuthority()) return;
    
    if (!CAbilitySystemComponent) return;

    CAbilitySystemComponent->RemoveKnockdown();
    CAbilitySystemComponent->ApplyDeath();
}
