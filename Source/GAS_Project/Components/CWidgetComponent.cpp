#include "CWidgetComponent.h"

#include "Blueprint/WidgetTree.h"
#include "GAS_Project/Characters/CCharacter.h"
#include "GAS_Project/GAS/CAbilitySystemComponent.h"


void UCWidgetComponent::BeginPlay()
{
	Super::BeginPlay();

	InitialAbilitySystemData();
	
	if (!IsASCInitialized())
	{
		BaseCharacter->OnASCInitialized.AddDynamic(this, &ThisClass::OnASCInitialized);
		return;
	}
	InitializeAttributeDelegate();
}

void UCWidgetComponent::InitialAbilitySystemData()
{
	BaseCharacter = Cast<ACCharacter>(GetOwner());
	AttributeSet = Cast<UCAttributeSet>(BaseCharacter->GetAttributeSet());
	AbilitySystemComponent = Cast<UCAbilitySystemComponent>(BaseCharacter->GetAbilitySystemComponent());

}

bool UCWidgetComponent::IsASCInitialized() const
{
	return AbilitySystemComponent.IsValid() && AttributeSet.IsValid();
}

void UCWidgetComponent::InitializeAttributeDelegate()
{
	if (!AttributeSet->bAttributesInitialized)
	{
		AttributeSet->OnAttributesInitialized.AddDynamic(this,&ThisClass::BindToAttributeChange);
	}
	else
	{
		BindToAttributeChange();
	}
}

void UCWidgetComponent::BindWidgetToAttributeChanges(UWidget* WidgetObject,
	const TTuple<FGameplayAttribute, FGameplayAttribute>& Pair) const
{
	// UCAttributeWidget* AttributeWidget = Cast<UCAttributeWidget>(WidgetObject);
	// if (!IsValid(AttributeWidget)) return;															// We only care about CC Attribute Widgets
	// if (!AttributeWidget->MatchesAttributes(Pair)) return;											// Only subscribe for matching Attributes
	//
	// AttributeWidget->OnAttributeChange(Pair, AttributeSet.Get());							// for initial values.
	//
	// AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(Pair.Key).AddLambda([this, AttributeWidget, &Pair](const FOnAttributeChangeData& AttributeChangeData)
	// {
	// 	AttributeWidget->OnAttributeChange(Pair, AttributeSet.Get());						// For changes during the game.
	// });
}

void UCWidgetComponent::OnASCInitialized(UAbilitySystemComponent* ASC, UAttributeSet* AS)
{
	AbilitySystemComponent = Cast<UCAbilitySystemComponent>(ASC);
	AttributeSet = Cast<UCAttributeSet>(AS);

	if (!IsASCInitialized()) return;
	
	InitializeAttributeDelegate();
}

void UCWidgetComponent::BindToAttributeChange()
{
	for (const TTuple<FGameplayAttribute, FGameplayAttribute>& Pair : AttributeMap)
	{
		BindWidgetToAttributeChanges(GetUserWidgetObject(), Pair);

		GetUserWidgetObject()->WidgetTree->ForEachWidget([this, &Pair](UWidget* ChildWidget)
		{
			BindWidgetToAttributeChanges(ChildWidget, Pair); 
		});
	}
}
