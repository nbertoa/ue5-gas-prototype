#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "Components/WidgetComponent.h"
#include "GP_WidgetComponent.generated.h"

struct FOnAttributeChangeData;
class UAbilitySystemComponent;
class UGP_AttributeSet;
class UGP_AbilitySystemComponent;
class AGP_BaseCharacter;
class UWidget;
class UGP_AttributeWidget;

/**
 * @class UGP_WidgetComponent
 * @brief Memory-safe, event-driven WidgetComponent linking GAS data to World Space UI.
 *
 * @section Architecture
 * This component acts as a bridge between the Gameplay Ability System and UMG widgets.
 * It discovers AttributeWidget children in the widget tree, matches them to attribute
 * pairs (Current/Max), and binds them to ASC delegates for reactive UI updates.
 *
 * @section Memory Safety
 * All delegate bindings use AddUObject tied to this component's lifetime,
 * and EndPlay performs explicit cleanup of all registered delegates.
 * This prevents dangling callbacks when the owning actor is destroyed
 * but the ASC survives (e.g., player ASC on PlayerState).
 */
UCLASS(ClassGroup=(Custom),
	meta=(BlueprintSpawnableComponent))
class GASPROTOTYPE_API UGP_WidgetComponent : public UWidgetComponent
{
	GENERATED_BODY()

public:
	UGP_WidgetComponent();

	/**
	 * @brief Called when the widget is first created.
	 * Attempts early initialization of GAS data if the world is a game world.
	 */
	virtual void InitWidget() override;

protected:
	/**
	 * @brief Initializes GAS bindings or subscribes to deferred initialization.
	 * Handles the timing problem where the ASC may not be ready at BeginPlay.
	 */
	virtual void BeginPlay() override;

	/**
	 * @brief Cleans up all GAS delegate bindings to prevent dangling callbacks.
	 * Critical for memory safety — without this, destroyed widgets could receive
	 * callbacks from a surviving ASC (e.g., player ASC on PlayerState).
	 */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/**
	 * @brief Maps each Current attribute to its corresponding Max attribute.
	 * Example: Key = Health, Value = MaxHealth.
	 * Configured in Blueprint to define which attribute pairs this widget displays.
	 */
	UPROPERTY(EditAnywhere,
		Category = "GAS|UI")
	TMap<FGameplayAttribute, FGameplayAttribute> AttributeMap;

private:
	/** @brief Weak reference to the owning character. */
	TWeakObjectPtr<AGP_BaseCharacter> BaseCharacter;

	/** @brief Weak reference to the ASC for delegate registration/cleanup. */
	TWeakObjectPtr<UGP_AbilitySystemComponent> AbilitySystemComponent;

	/** @brief Weak reference to the AttributeSet for reading attribute values. */
	TWeakObjectPtr<UGP_AttributeSet> AttributeSet;

	/** @brief Caches GAS component references from the owning character. */
	void InitAbilitySystemData();

	/** @brief Returns true if both ASC and AttributeSet are valid. */
	bool IsASCInitialized() const;

	/**
	 * @brief Sets up the delegate chain for attribute initialization.
	 * If attributes are already initialized, binds immediately.
	 * Otherwise, subscribes to OnAttributesInitialized to defer binding.
	 */
	void InitializeAttributeDelegate();

	/**
	 * @brief Evaluates a single widget and binds it to GAS delegates if it matches the attribute pair.
	 * Pushes initial values immediately and registers for future change notifications.
	 *
	 * @param WidgetObject The widget to evaluate (may or may not be a GP_AttributeWidget).
	 * @param Pair The Current/Max attribute pair to match against.
	 */
	void BindWidgetToAttributeChanges(UWidget* WidgetObject,
	                                  const TTuple<FGameplayAttribute, FGameplayAttribute>& Pair);

	/**
	 * @brief Deferred initialization callback when ASC becomes ready.
	 * Called by the BaseCharacter's OnASCInitialized delegate.
	 */
	UFUNCTION()
	void OnASCInitialized(UAbilitySystemComponent* ASC,
	                      UAttributeSet* AS);

	/**
	 * @brief Iterates the widget tree and binds matching AttributeWidgets to GAS delegates.
	 * Called when attributes are confirmed initialized and ready for reading.
	 */
	UFUNCTION()
	void BindToAttributeChanges();

	/**
	 * @brief Centralized callback for attribute change delegates.
	 * Bound via AddUObject to tie the delegate lifecycle to this component.
	 * Forwards the change notification to the target AttributeWidget if still valid.
	 *
	 * @param Data The attribute change data from GAS (unused directly, triggers the update).
	 * @param WeakWidget Weak reference to the target widget to update.
	 * @param Pair The Current/Max attribute pair to pass to the widget for value reading.
	 */
	void OnAttributeChangedCallback(const FOnAttributeChangeData& Data,
	                                TWeakObjectPtr<UGP_AttributeWidget> WeakWidget,
	                                TTuple<FGameplayAttribute, FGameplayAttribute> Pair);
};
