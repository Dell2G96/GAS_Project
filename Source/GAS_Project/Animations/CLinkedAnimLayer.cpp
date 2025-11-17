// Fill out your copyright notice in the Description page of Project Settings.


#include "CLinkedAnimLayer.h"

#include "CPlayerAnim.h"

class UCPlayerAnim* UCLinkedAnimLayer::GetPlayerAnimInstance() const
{
	return Cast<UCPlayerAnim>(GetOwningComponent()->GetAnimInstance());
}
