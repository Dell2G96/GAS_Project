// CWeapon.h
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GAS_Project/Utils/CStructTypes.h"
#include "CWeapon.generated.h"

UCLASS()
class GAS_PROJECT_API ACWeapon : public AActor
{
	GENERATED_BODY()

public:
	ACWeapon();
public:
	UPROPERTY()
	struct FWeaponConfig CurrentWeaponConfig;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon")
	USkeletalMeshComponent* WeaponMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon")
	class UBoxComponent* WeaponCollisionBox;

	UPROPERTY(EditDefaultsOnly, Category="Weapon")
	TSubclassOf<class UGameplayAbility> AbilitiesToGrant;

public:
	FORCEINLINE UBoxComponent* GetWeaponCollisionBox() const { return WeaponCollisionBox; }
	
};
