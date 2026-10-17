// // Fill out your copyright notice in the Description page of Project Settings.
//
// #pragma once
//
// #include "CoreMinimal.h"
// #include "LeeGameplayAbility.h"
// #include "LeeGameplayAbility_EnemyRangedAttack.generated.h"
//
// /**
//  * Enemy 원거리 공격 어빌리티의 식별용 베이스 클래스.
//  *
//  * 실제 발사/투사체 로직은 이 클래스를 부모로 하는 BP에서 구현한다.
//  * C++이 담당하는 것은 식별 태그(MyTags.Abilities.Enemy.Range.Attack)를 AbilityTags에 붙이는 것뿐이며,
//  * AI의 Lee Activate Ability Task가 이 태그로 어빌리티를 찾아 활성화한다.
//  *
//  * [주석 처리 사유] SoulsCore에는 아직 Enemy 원거리 공격 GA가 없고, 근접과 마찬가지로
//  * BP의 Ability Tags로 해결할 수 있어 별도 C++ 클래스가 필요 없다.
//  * 빌드 검증만 마치고 비활성화한다.
//  */
// UCLASS()
// class GAS_PROJECT_API ULeeGameplayAbility_EnemyRangedAttack : public ULeeGameplayAbility
// {
// 	GENERATED_BODY()
//
// public:
// 	ULeeGameplayAbility_EnemyRangedAttack(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
// };
