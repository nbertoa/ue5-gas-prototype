#include "Tasks/GP_AttributeChangeTask.h"

#include "AbilitySystemComponent.h"

UGP_AttributeChangeTask* UGP_AttributeChangeTask::ListenForAttributeChange(
	UAbilitySystemComponent* AbilitySystemComponent,
	FGameplayAttribute Attribute)
{
	// Create the task as a root UObject (no outer).
	// The task must survive independently of the actor that created it
	// because the ASC delegate needs a valid target to call back into.
	UGP_AttributeChangeTask* WaitForAttributeChangeTask = NewObject<UGP_AttributeChangeTask>();
	WaitForAttributeChangeTask->ASC = AbilitySystemComponent;
	WaitForAttributeChangeTask->AttributeToListenFor = Attribute;

	// Soft Fail: If the ASC is invalid, clean up the task and return nullptr.
	// The async action system handles nullptr gracefully (the node simply won't fire).
	if (!IsValid(AbilitySystemComponent))
	{
		WaitForAttributeChangeTask->RemoveFromRoot();
		return nullptr;
	}

	// Bind to the ASC's native attribute change delegate.
	// This delegate fires every time the specified attribute is modified
	// by a Gameplay Effect, ability, or direct setter call.
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(Attribute).AddUObject(WaitForAttributeChangeTask,
		&UGP_AttributeChangeTask::AttributeChanged);

	return WaitForAttributeChangeTask;
}

void UGP_AttributeChangeTask::EndTask()
{
	// Unregister from the ASC delegate to prevent callbacks into a dying object.
	// The weak pointer check ensures we don't crash if the ASC was already destroyed.
	if (ASC.IsValid())
	{
		ASC->GetGameplayAttributeValueChangeDelegate(AttributeToListenFor).RemoveAll(this);
	}

	// Notify the async action system that this task is complete.
	SetReadyToDestroy();

	// Request immediate garbage collection instead of waiting for the next GC cycle.
	MarkAsGarbage();
}

void UGP_AttributeChangeTask::AttributeChanged(const FOnAttributeChangeData& Data)
{
	// Re-broadcast the GAS data through our Blueprint-friendly delegate.
	// This unpacks the C++ struct into individual float values that
	// Blueprint designers can use directly without casting or struct access.
	OnAttributeChanged.Broadcast(Data.Attribute,
	                             Data.NewValue,
	                             Data.OldValue);
}
