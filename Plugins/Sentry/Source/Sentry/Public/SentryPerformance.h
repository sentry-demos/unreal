// Copyright (c) 2024 Sentry. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Engine/Engine.h"
#include "RenderingThread.h"
#include "RHI.h"
#include "Stats/Stats.h"
#include "SentryPerformance.generated.h"

// Forward declarations
struct FRawStatStackNode;

USTRUCT(BlueprintType)
struct FFrameTimingData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	float TotalFrameTime = 0.0f;

	UPROPERTY(BlueprintReadOnly)
	float FPS = 0.0f;

	UPROPERTY(BlueprintReadOnly)
	FString DetailedStatsReport;

	UPROPERTY(BlueprintReadOnly)
	FString Timestamp;

	FFrameTimingData()
	{
		Timestamp = FDateTime::Now().ToString();
	}
};

USTRUCT(BlueprintType)
struct FPerformanceDropEvent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	float DropSeverity = 0.0f;

	UPROPERTY(BlueprintReadOnly)
	FFrameTimingData FrameData;

	UPROPERTY(BlueprintReadOnly)
	float PreviousAverageFPS = 0.0f;

	UPROPERTY(BlueprintReadOnly)
	FString DropReason;

	FPerformanceDropEvent() = default;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPerformanceDrop, const FPerformanceDropEvent&, DropEvent);

/**
 * Performance monitoring component that tracks frame times and detects performance drops
 * Managed by SentrySubsystem when performance monitoring is enabled in settings
 */
UCLASS()
class SENTRY_API USentryPerformance : public UObject
{
	GENERATED_BODY()

public:
	/** Initialize performance monitoring */
	void Initialize();
	/** Cleanup performance monitoring */
	void Deinitialize();

	/** Enable or disable performance monitoring */
	UFUNCTION(BlueprintCallable, Category = "Performance")
	void SetMonitoringEnabled(bool bEnabled);

	/** Check if monitoring is currently enabled */
	UFUNCTION(BlueprintPure, Category = "Performance")
	bool IsMonitoringEnabled() const { return bIsMonitoringEnabled; }

	/** Set the FPS threshold below which drops are detected */
	UFUNCTION(BlueprintCallable, Category = "Performance")
	void SetFPSDropThreshold(float Threshold);

	/** Set the minimum drop severity to trigger event (0.0-1.0) */
	UFUNCTION(BlueprintCallable, Category = "Performance")
	void SetDropSeverityThreshold(float Threshold);

	/** Get current frame timing data */
	UFUNCTION(BlueprintCallable, Category = "Performance")
	FFrameTimingData GetCurrentFrameTimings();

	/** Get average FPS over the last N frames */
	UFUNCTION(BlueprintCallable, Category = "Performance")
	float GetAverageFPS(int32 FrameCount = 60) const;

	/** Get frame timing history */
	UFUNCTION(BlueprintCallable, Category = "Performance")
	TArray<FFrameTimingData> GetFrameTimingHistory(int32 FrameCount = 60) const;


	/** Event fired when performance drop is detected */
	UPROPERTY(BlueprintAssignable)
	FOnPerformanceDrop OnPerformanceDrop;

protected:
	/** Called every frame to update performance metrics */
	void UpdatePerformanceMetrics();

	/** Analyze current frame for performance drops */
	void AnalyzePerformanceDrop(const FFrameTimingData& CurrentFrame);

	/** Calculate drop severity based on FPS difference */
	float CalculateDropSeverity(float CurrentFPS, float AverageFPS) const;

	/** Get detailed timing stats when performance drop is detected */
	void PopulateDetailedStats(FFrameTimingData& FrameData) const;

	/** Generate detailed timing report */
	FString GenerateTimingReport(const FFrameTimingData& FrameData) const;

	/** Create Sentry transaction with thread timing spans */
	void CreatePerformanceTransaction(const FPerformanceDropEvent& DropEvent);

	/** Send performance drop event to Sentry if integrated */
	void ReportPerformanceDropToSentry(const FPerformanceDropEvent& DropEvent);

	/** Processes Stack Frames and creates them as Sentry spans */
	void ProcessStackFrames(FRawStatStackNode const& StackNode, class USentryTransaction* Transaction, class USentrySpan* ParentSpan, int64 StartTime, int64 &TotalTime) const;

private:

	/** Frame timing history */
	TArray<FFrameTimingData> FrameHistory;

	/** Delegate handle for frame updates */
	FDelegateHandle OnEndFrameHandle;

	/** Last time we checked for performance drops */
	double LastPerformanceCheckTime = 0.0;

	/** Cached settings values loaded during initialization */
	bool bIsMonitoringEnabled = true;
	float FPSDropThreshold = 30.0f;
	float DropSeverityThreshold = 0.3f;
	int32 MaxHistoryFrames = 300;
	float PerformanceCheckInterval = 1.0f;

	/** Rolling average calculator */
	mutable TArray<float> RollingFPSBuffer;
	mutable int32 RollingFPSIndex = 0;
};