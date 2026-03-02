// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeEquipmentDefinition.h"

#include "LeeEquipmentInstance.h"

ULeeEquipmentDefinition::ULeeEquipmentDefinition(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	// 기본값으로 아래 클래스로 설정
	InstanceType = ULeeEquipmentInstance::StaticClass();
}
