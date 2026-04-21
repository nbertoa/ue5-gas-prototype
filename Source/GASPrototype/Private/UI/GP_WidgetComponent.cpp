#include "UI/GP_WidgetComponent.h"
#include "AbilitySystem/GP_AbilitySystemComponent.h"
#include "AbilitySystem/GP_AttributeSet.h"
#include "Blueprint/WidgetTree.h"
#include "Characters/GP_BaseCharacter.h"
#include "UI/GP_AttributeWidget.h"

UGP_WidgetComponent::UGP_WidgetComponent()
{
	// -------------------------------------------------------------------------
	// TICK CONFIGURATION
	// -------------------------------------------------------------------------
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	bTickInEditor = false;
}

void UGP_WidgetComponent::InitWidget()
{
	Super::InitWidget();

	UWorld* World = GetWorld();
	if (World && World->IsGameWorld())
	{
		InitAbilitySystemData();
	}
}

void UGP_WidgetComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!IsASCInitialized())
	{
		InitAbilitySystemData();
	}

	if (!IsASCInitialized())
	{
		if (BaseCharacter.IsValid())
		{
			BaseCharacter->OnASCInitialized.AddDynamic(this,
			                                           &ThisClass::OnASCInitialized);
		}
	}
	else
	{
		InitializeAttributeDelegate();
	}
}

void UGP_WidgetComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AbilitySystemComponent.IsValid())
	{
		for (const TTuple<FGameplayAttribute, FGameplayAttribute>& Pair : AttributeMap)
		{
			AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(Pair.Key).RemoveAll(this);
			AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(Pair.Value).RemoveAll(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void UGP_WidgetComponent::InitAbilitySystemData()
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	BaseCharacter = Cast<AGP_BaseCharacter>(OwnerActor);

	if (!ensureMsgf(BaseCharacter.IsValid(),
	                TEXT("UGP_WidgetComponent attached to non-AGP_BaseCharacter: %s"),
	                *OwnerActor->GetName()))
	{
		return;
	}

	AttributeSet = Cast<UGP_AttributeSet>(BaseCharacter->GetAttributeSet());
	AbilitySystemComponent = Cast<UGP_AbilitySystemComponent>(BaseCharacter->GetAbilitySystemComponent());
}

bool UGP_WidgetComponent::IsASCInitialized() const
{
	return AbilitySystemComponent.IsValid() && AttributeSet.IsValid();
}

void UGP_WidgetComponent::InitializeAttributeDelegate()
{
	if (!ensureMsgf(AttributeSet.IsValid(),
	                TEXT("InitializeAttributeDelegate called with invalid AttributeSet!")))
	{
		return;
	}

	if (!AttributeSet->bAttributesInitialized)
	{
		AttributeSet->OnAttributesInitialized.AddDynamic(this,
		                                                 &ThisClass::BindToAttributeChanges);
	}
	else
	{
		BindToAttributeChanges();
	}
}

void UGP_WidgetComponent::OnASCInitialized(UAbilitySystemComponent* ASC,
                                           UAttributeSet* AS)
{
	AbilitySystemComponent = Cast<UGP_AbilitySystemComponent>(ASC);
	AttributeSet = Cast<UGP_AttributeSet>(AS);

	if (IsASCInitialized())
	{
		InitializeAttributeDelegate();
	}
}

void UGP_WidgetComponent::BindToAttributeChanges()
{
	UUserWidget* UserWidget = GetUserWidgetObject();

	if (!ensureMsgf(UserWidget,
	                TEXT("UGP_WidgetComponent::BindToAttributeChanges - No Widget Class assigned in Blueprint!")))
	{
		return;
	}

	for (const TTuple<FGameplayAttribute, FGameplayAttribute>& Pair : AttributeMap)
	{
		BindWidgetToAttributeChanges(UserWidget,
		                             Pair);

		if (UserWidget->WidgetTree)
		{
			UserWidget->WidgetTree->ForEachWidget([this, Pair](UWidget* ChildWidget)
			{
				BindWidgetToAttributeChanges(ChildWidget,
				                             Pair);
			});
		}
	}
}

void UGP_WidgetComponent::BindWidgetToAttributeChanges(UWidget* WidgetObject,
                                                       const TTuple<FGameplayAttribute, FGameplayAttribute>& Pair)
{
	if (!WidgetObject)
	{
		return;
	}

	UGP_AttributeWidget* AttributeWidget = Cast<UGP_AttributeWidget>(WidgetObject);
	if (!IsValid(AttributeWidget) || !AttributeWidget->MatchesAttributes(Pair))
	{
		return;
	}

	// Set the avatar actor reference so Blueprint can use it for
	// positioning floating text or directing visual effects.
	AttributeWidget->AvatarActor = BaseCharacter;

	// Push initial values with OldValue = 0 since there's no previous state.
	AttributeWidget->OnAttributeChange(Pair,
	                                   AttributeSet.Get(),
	                                   0.0f);

	TWeakObjectPtr<UGP_AttributeWidget> WeakWidget(AttributeWidget);

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(Pair.Key).AddUObject(this,
		&ThisClass::OnAttributeChangedCallback,
		WeakWidget,
		Pair);

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(Pair.Value).AddUObject(this,
		&ThisClass::OnAttributeChangedCallback,
		WeakWidget,
		Pair);
}

void UGP_WidgetComponent::OnAttributeChangedCallback(const FOnAttributeChangeData& Data,
                                                     TWeakObjectPtr<UGP_AttributeWidget> WeakWidget,
                                                     TTuple<FGameplayAttribute, FGameplayAttribute> Pair)
{
	if (WeakWidget.IsValid() && AttributeSet.IsValid())
	{
		// Forward the OldValue from the GAS callback data so Blueprint
		// can calculate deltas (e.g., floating damage numbers).
		WeakWidget->OnAttributeChange(Pair,
		                              AttributeSet.Get(),
		                              Data.OldValue);
	}
}
