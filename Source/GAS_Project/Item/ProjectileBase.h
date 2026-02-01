// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "GameFramework/Actor.h"
#include "ProjectileBase.generated.h"

UENUM(BlueprintType)
enum class EProjectileDamagePolicy : uint8
{
	OnHit,
	OnBeginOverlap
};


UCLASS()
class GAS_PROJECT_API AProjectileBase : public AActor
{
	GENERATED_BODY()

public:
	AProjectileBase();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GAS|Projectile", meta = (ExposeOnSpawn, ClampMin="0.0"))
	float Damage{-25.f};

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GAS|Projectile")
	class UBoxComponent* ProjectileCollisionBox;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GAS|Projectile")
	class UNiagaraComponent* ProjectileNiagaraComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GAS|Projectile")
	class UProjectileMovementComponent* ProjectileMovementComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GAS|Projectile")
	EProjectileDamagePolicy ProjectileDamagePolicy = EProjectileDamagePolicy::OnHit;

	UFUNCTION()
	virtual void OnProjectileHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION()
	virtual void OnProjectileBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION(BlueprintImplementableEvent, meta=(DisplayName="On Spawn Projectile Hit FX"))
	void BP_OnSpawnProjectileHitFX(const FVector& HitLocation);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GAS|Projectile")
	TSubclassOf<class UGameplayEffect> DamageEffect;
private:
	void HandleApplyProjectaileDamage(APawn* InHitPawn,const struct FGameplayEventData& InPayload);

	TArray<AActor*> OverlappedActors;
	
};
