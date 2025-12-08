// CWeapon.h
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GAS_Project/Utils/CStructTypes.h"
#include "CWeapon.generated.h"

USTRUCT(BlueprintType)
struct FWeaponAttachSplite
{
	GENERATED_BODY()

public:
	// 붙일 부모 컴포넌트 (보통 캐릭터 Mesh)
	UPROPERTY()
	TObjectPtr<USceneComponent> Parent = nullptr;

	// 칼을 붙일 소켓 이름 
	UPROPERTY()
	FName WeaponSocketName = NAME_None;

	// 칼집을 붙일 소켓 이름 
	UPROPERTY()
	FName SheathSocketName = NAME_None;
};


UCLASS()
class GAS_PROJECT_API ACWeapon : public AActor
{
	GENERATED_BODY()

public:
	ACWeapon();

public:
	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon")
	USkeletalMeshComponent* WeaponMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon")
	USkeletalMeshComponent* SheathMesh;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon")
	class UBoxComponent* WeaponCollisionBox;

	UPROPERTY(EditDefaultsOnly, Category="Weapon")
	TSubclassOf<class UGameplayAbility> AbilitiesToGrant;

	// 블루프린트의 OnRep_AttachSplite 에 해당하는 Replicated 데이터
	UPROPERTY(ReplicatedUsing=OnRep_AttachSplite)
	FWeaponAttachSplite AttachSplite;

public:
	FORCEINLINE UBoxComponent* GetWeaponCollisionBox() const { return WeaponCollisionBox; }

	// 서버에서만 호출해서 Attach 정보를 세팅하는 함수
	UFUNCTION(BlueprintCallable, Category="Weapon|Attach")
	void SetupWeaponAttach(USceneComponent* InParent, FName InWeaponSocketName, FName InSheathSocketName);

	
protected:
	// Replication 설정
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// AttachSplite 가 복제됐을 때 호출되는 함수 (블루프린트의 OnRep_AttachSplite 이벤트에 해당)
	UFUNCTION()
	void OnRep_AttachSplite();

	// 실제로 WeaponMesh / SheathMesh 를 부모에 붙이는 내부 함수
	void ApplyAttachSplite();
};
