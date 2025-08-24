// Copyright (c) 2024 Sentry. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "SentryTowerGameInstance.generated.h"

DECLARE_DYNAMIC_DELEGATE_OneParam(FOnBuyComplete, bool, IsSuccessfull);

struct FPerformanceDropEvent;
class USentryPerformance;
class USentrySubsystem;

UCLASS()
class SENTRYTOWER_API USentryTowerGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;

	UFUNCTION(BlueprintCallable, Meta = (AutoCreateRefTerm = "OnBuyComplete"))
	void BuyUpgrade(const FOnBuyComplete& OnBuyComplete);

	/** Force trigger a test performance drop event for testing */
	UFUNCTION(BlueprintCallable, Category = "Testing")
	void TriggerTestPerformanceDrop();

	/** Called when a performance drop is detected */
	UFUNCTION()
	void OnPerformanceDropDetected(const FPerformanceDropEvent& DropEvent);
};
