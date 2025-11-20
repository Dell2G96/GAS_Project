// Fill out your copyright notice in the Description page of Project Settings.


#include "CLinkedAnimLayer.h"
#include "Animation/AnimInstance.h"

class UCAniminstance* UCLinkedAnimLayer::GetPlayerAnimInstance() const
{
	return Cast<UCAniminstance>(GetOwningComponent()->GetAnimInstance());
}
