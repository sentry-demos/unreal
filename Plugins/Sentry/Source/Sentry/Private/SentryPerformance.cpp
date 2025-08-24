// Copyright (c) 2024 Sentry. All Rights Reserved.

#include "SentryPerformance.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Stats/Stats.h"
#include "Stats/StatsData.h"
#include "Stats/StatsMisc.h"
#include "HAL/IConsoleManager.h"
#include "RenderingThread.h"
#include "RHI.h"
#include "Engine/GameViewportClient.h"
#include "SentrySubsystem.h"
#include "SentrySettings.h"
#include "SentryVariant.h"
#include "SentryLibrary.h"
#include "SentryTransaction.h"
#include "SentryTransactionContext.h"
#include "SentrySpan.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/FileHelper.h"

DECLARE_STATS_GROUP(TEXT("SentryTower Performance"), STATGROUP_SentryTowerPerformance, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("Performance Monitoring"), STAT_PerformanceMonitoring, STATGROUP_SentryTowerPerformance);

void USentryPerformance::Initialize()
{
	// Load settings from SentrySettings
	const USentrySettings* Settings = GetDefault<USentrySettings>();
	if (Settings)
	{
		bIsMonitoringEnabled = Settings->EnablePerformanceMonitoring;
		FPSDropThreshold = Settings->FPSDropThreshold;
		DropSeverityThreshold = Settings->DropSeverityThreshold;
		MaxHistoryFrames = Settings->MaxHistoryFrames;
		PerformanceCheckInterval = Settings->PerformanceCheckInterval;
	}

	// Initialize rolling FPS buffer
	RollingFPSBuffer.SetNum(MaxHistoryFrames);
	for (int32 i = 0; i < MaxHistoryFrames; i++)
	{
		RollingFPSBuffer[i] = 60.0f; // Initialize with reasonable default
	}

	// Register for end frame callback
	if (GEngine)
	{
		OnEndFrameHandle = FCoreDelegates::OnEndFrame.AddUObject(this, &USentryPerformance::UpdatePerformanceMetrics);
	}

#if STATS
	// Enable stats collection using PrimaryEnableAdd
	FThreadStats::PrimaryEnableAdd();
	
	UE_LOG(LogTemp, Log, TEXT("SentryPerformance initialized with stats collection enabled"));
#else
	UE_LOG(LogTemp, Log, TEXT("SentryPerformance initialized (stats disabled in build)"));
#endif
}

void USentryPerformance::Deinitialize()
{
	// Unregister from end frame callback
	if (OnEndFrameHandle.IsValid())
	{
		FCoreDelegates::OnEndFrame.Remove(OnEndFrameHandle);
		OnEndFrameHandle.Reset();
	}

#if STATS
	// Disable stats collection
	FThreadStats::PrimaryEnableSubtract();
#endif
}

void USentryPerformance::SetMonitoringEnabled(bool bEnabled)
{
	bIsMonitoringEnabled = bEnabled;
	UE_LOG(LogTemp, Log, TEXT("Performance monitoring %s"), bEnabled ? TEXT("enabled") : TEXT("disabled"));
}

void USentryPerformance::SetFPSDropThreshold(float Threshold)
{
	FPSDropThreshold = FMath::Max(1.0f, Threshold);
}

void USentryPerformance::SetDropSeverityThreshold(float Threshold)
{
	DropSeverityThreshold = FMath::Clamp(Threshold, 0.0f, 1.0f);
}

FFrameTimingData USentryPerformance::GetCurrentFrameTimings()
{
	FFrameTimingData TimingData;

	UWorld* World = nullptr;
	if (GEngine && GEngine->GetWorldContexts().Num() > 0)
	{
		World = GEngine->GetWorldContexts()[0].World();
	}
	
	if (!World)
	{
		return TimingData;
	}

	// Get basic frame timing
	const float DeltaTime = World->GetDeltaSeconds();
	TimingData.TotalFrameTime = DeltaTime * 1000.0f; // Convert to milliseconds
	TimingData.FPS = DeltaTime > 0.0f ? 1.0f / DeltaTime : 0.0f;

	return TimingData;
}

float USentryPerformance::GetAverageFPS(int32 FrameCount) const
{
	if (FrameHistory.IsEmpty())
	{
		return 0.0f;
	}

	const int32 SamplesToUse = FMath::Min(FrameCount, FrameHistory.Num());
	float TotalFPS = 0.0f;

	for (int32 i = 0; i < SamplesToUse; i++)
	{
		const int32 Index = FrameHistory.Num() - 1 - i;
		TotalFPS += FrameHistory[Index].FPS;
	}

	return SamplesToUse > 0 ? TotalFPS / SamplesToUse : 0.0f;
}

TArray<FFrameTimingData> USentryPerformance::GetFrameTimingHistory(int32 FrameCount) const
{
	const int32 SamplesToReturn = FMath::Min(FrameCount, FrameHistory.Num());
	TArray<FFrameTimingData> Result;
	Result.Reserve(SamplesToReturn);

	for (int32 i = 0; i < SamplesToReturn; i++)
	{
		const int32 Index = FrameHistory.Num() - 1 - i;
		Result.Add(FrameHistory[Index]);
	}

	return Result;
}


void USentryPerformance::UpdatePerformanceMetrics()
{
	if (!bIsMonitoringEnabled)
	{
		return;
	}

	SCOPE_CYCLE_COUNTER(STAT_PerformanceMonitoring);

	// Get current frame timing data
	FFrameTimingData CurrentFrame = GetCurrentFrameTimings();

	// Add to history
	FrameHistory.Add(CurrentFrame);

	// Limit history size
	if (FrameHistory.Num() > MaxHistoryFrames)
	{
		FrameHistory.RemoveAt(0, FrameHistory.Num() - MaxHistoryFrames);
	}

	// Update rolling FPS buffer
	RollingFPSBuffer[RollingFPSIndex] = CurrentFrame.FPS;
	RollingFPSIndex = (RollingFPSIndex + 1) % MaxHistoryFrames;

	// Check for performance drops periodically
	const double CurrentTime = FPlatformTime::Seconds();
	if (CurrentTime - LastPerformanceCheckTime >= PerformanceCheckInterval)
	{
		AnalyzePerformanceDrop(CurrentFrame);
		LastPerformanceCheckTime = CurrentTime;
	}
}

void USentryPerformance::AnalyzePerformanceDrop(const FFrameTimingData& CurrentFrame)
{
	// Need sufficient history to analyze
	if (FrameHistory.Num() < 30)
	{
		return;
	}

	const float CurrentFPS = CurrentFrame.FPS;
	const float AverageFPS = GetAverageFPS(60); // Average over last 60 frames
	
	// Check if current FPS is significantly below threshold and average
	if (CurrentFPS < FPSDropThreshold && CurrentFPS < AverageFPS * 0.7f)
	{
		const float DropSeverity = CalculateDropSeverity(CurrentFPS, AverageFPS);
		
		// Only trigger if severity exceeds threshold
		if (DropSeverity >= DropSeverityThreshold)
		{
			// Now that we detected a drop, get detailed stats
			FFrameTimingData DetailedFrameData = CurrentFrame;
			PopulateDetailedStats(DetailedFrameData);

			FPerformanceDropEvent DropEvent;
			DropEvent.DropSeverity = DropSeverity;
			DropEvent.FrameData = DetailedFrameData;
			DropEvent.PreviousAverageFPS = AverageFPS;
			DropEvent.DropReason = TEXT("Performance drop detected");

			// Broadcast event
			OnPerformanceDrop.Broadcast(DropEvent);
			
			// Report to Sentry
			ReportPerformanceDropToSentry(DropEvent);

			UE_LOG(LogTemp, Warning, TEXT("Performance drop detected (Severity: %.2f, FPS: %.1f, Avg: %.1f)"), 
				DropSeverity, CurrentFPS, AverageFPS);
		}
	}
}

float USentryPerformance::CalculateDropSeverity(float CurrentFPS, float AverageFPS) const
{
	if (AverageFPS <= 0.0f)
	{
		return 0.0f;
	}

	// Calculate severity based on percentage drop from average
	const float PercentageDrop = (AverageFPS - CurrentFPS) / AverageFPS;
	
	// Also factor in absolute FPS value (drops to very low FPS are more severe)
	float AbsoluteSeverity = 1.0f - (CurrentFPS / 60.0f); // Normalize against 60 FPS
	AbsoluteSeverity = FMath::Clamp(AbsoluteSeverity, 0.0f, 1.0f);
	
	// Combine both factors
	const float CombinedSeverity = (PercentageDrop * 0.7f) + (AbsoluteSeverity * 0.3f);
	
	return FMath::Clamp(CombinedSeverity, 0.0f, 1.0f);
}


void USentryPerformance::ProcessStackFrames(FRawStatStackNode const& StackNode, USentryTransaction* Transaction, USentrySpan* ParentSpan, int64 StartTime, int64 &TotalTime) const
{
	if (!Transaction)
	{
		return;
	}
	
	// Get stat information
	FString GroupNameString = StackNode.Meta.NameAndInfo.GetGroupName().ToString();
	FString StatNameString = StackNode.Meta.NameAndInfo.GetShortName().ToString();
	
	// Create span for this stat
	USentrySpan* CurrentSpan = nullptr;

	// Calculate timing for this stat (in milliseconds)
	float StatTimeMs = 0;
	int64 StatTimeUs = 0;
	if (StackNode.Meta.NameAndInfo.GetField<EStatDataType>() == EStatDataType::ST_int64)
	{
		StatTimeMs = FPlatformTime::ToMilliseconds(FromPackedCallCountDuration_Duration(StackNode.Meta.GetValue_int64()));
		StatTimeUs = (int64) (StatTimeMs * 1000);

		if (ParentSpan)
		{
			// Create child span
			CurrentSpan = ParentSpan->StartChildWithTimestamp(GroupNameString, StatNameString, StartTime);
		}
		else
		{
			// Create top-level span in transaction
			CurrentSpan = Transaction->StartChildSpanWithTimestamp(GroupNameString, StatNameString, StartTime);
			if (StatTimeUs > TotalTime) {
				TotalTime = StatTimeUs;
			}
		}
	}


	// Process children recursively
	if (StackNode.Children.Num() > 0)
	{
		TArray<FRawStatStackNode*> ChildArray;
		StackNode.Children.GenerateValueArray(ChildArray);
		ChildArray.Sort(FStatDurationComparer<FRawStatStackNode>());
		
		for (int32 Index = 0; Index < ChildArray.Num(); Index++)
		{
			ProcessStackFrames(*ChildArray[Index], Transaction, CurrentSpan, StartTime, TotalTime);
		}
	}

	// Finish the span if it was created
	if (CurrentSpan)
	{		
		// Finish the span
		CurrentSpan->FinishWithTimestamp(StartTime + StatTimeUs);
	}
}

void USentryPerformance::PopulateDetailedStats(FFrameTimingData& FrameData) const
{
#if STATS
	// Get the most recent frame stats from the local thread state
	if (FThreadStats::IsCollectingData())
	{
		FStatsThreadState const& StatsState = FStatsThreadState::GetLocalState();
		
		// Get the most recent frame data (condensed history with just 1 frame)
		TArray<FStatMessage> const& RecentStats = StatsState.GetCondensedHistory(StatsState.GetLatestValidFrame());
		
		// Build detailed report from recent frame stats data
		FString DetailedReport = TEXT("=== Latest Frame Performance Stats ===\n");
		
		for (const FStatMessage& StatMessage : RecentStats)
		{
			// Use FStatsUtils::DebugPrint to format each stat message properly
			FString StatLine = FStatsUtils::DebugPrint(StatMessage);
			DetailedReport += StatLine + TEXT("\n");
		}
		
		FrameData.DetailedStatsReport = DetailedReport;

		// Create Sentry transaction for detailed performance analysis
		if (USentrySubsystem* SentrySubsystem = GEngine->GetEngineSubsystem<USentrySubsystem>())
		{
			if (SentrySubsystem->IsEnabled())
			{	
				// Get current time in microseconds as base timestamp  
				int64 StartTime = FDateTime::UtcNow().ToUnixTimestamp() * 1000000; // Convert to microseconds
				
				// Create transaction context
				USentryTransactionContext* TransactionContext = USentryLibrary::CreateSentryTransactionContext(
					TEXT("frame_rendering_times"), 
					TEXT("performance.data")
				);
				
				USentryTransaction* PerformanceTransaction = SentrySubsystem->StartTransactionWithContextAndTimestamp(
					TransactionContext,
					StartTime
				);
				
				if (PerformanceTransaction)
				{
					// Get frame stack and process it into Sentry spans
					FRawStatStackNode Stack;
					StatsState.UncondenseStackStats(StatsState.GetLatestValidFrame(), Stack);
					Stack.AddSelf();
					Stack.CullByCycles(int64(1.0f / FPlatformTime::ToMilliseconds(1)));
					
					// Process stack frames as Sentry spans
					int64 TotalTime = 0;
					ProcessStackFrames(Stack, PerformanceTransaction, nullptr, StartTime, TotalTime);
					
					// Finish the transaction
					PerformanceTransaction->FinishWithTimestamp(StartTime + TotalTime);
					
					UE_LOG(LogTemp, Log, TEXT("Created detailed Sentry performance transaction with stat spans"));
				}
			}
		}
	}
#endif
}

FString USentryPerformance::GenerateTimingReport(const FFrameTimingData& FrameData) const
{
	FString Report = FString::Printf(TEXT("Performance Report - %s\n"), *FrameData.Timestamp);
	Report += FString::Printf(TEXT("FPS: %.1f\n"), FrameData.FPS);
	Report += FString::Printf(TEXT("Frame Time: %.2f ms\n"), FrameData.TotalFrameTime);
	return Report;
}

void USentryPerformance::ReportPerformanceDropToSentry(const FPerformanceDropEvent& DropEvent)
{
	// Try to get Sentry subsystem if available
	if (USentrySubsystem* SentrySubsystem = GEngine->GetEngineSubsystem<USentrySubsystem>())
	{
		if (SentrySubsystem->IsEnabled())
		{
				// Create a detailed message for Sentry
				const FString Message = FString::Printf(TEXT("Performance drop detected: %s (Severity: %.2f)"), 
					*DropEvent.DropReason, DropEvent.DropSeverity);
				
				// Add performance context using FSentryVariant constructors
				TMap<FString, FSentryVariant> SentryPerformanceContext;
				
				// Create Sentry variants for the performance data using constructors
				SentryPerformanceContext.Add(TEXT("current_fps"), FSentryVariant(DropEvent.FrameData.FPS));
				SentryPerformanceContext.Add(TEXT("average_fps"), FSentryVariant(DropEvent.PreviousAverageFPS));
				SentryPerformanceContext.Add(TEXT("drop_severity"), FSentryVariant(DropEvent.DropSeverity));
				
				// Set the performance context
				SentrySubsystem->SetContext(TEXT("performance"), SentryPerformanceContext);
				
				// Create temporary file for detailed stats report
				if (!DropEvent.FrameData.DetailedStatsReport.IsEmpty())
				{
					FString TempFilePath = FPaths::ProjectSavedDir() / TEXT("Temp") / TEXT("performance_stats.txt");
					
					// Ensure directory exists
					FString TempDir = FPaths::GetPath(TempFilePath);
					IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
					if (!PlatformFile.DirectoryExists(*TempDir))
					{
						PlatformFile.CreateDirectoryTree(*TempDir);
					}
					
					// Write detailed stats to file
					if (FFileHelper::SaveStringToFile(DropEvent.FrameData.DetailedStatsReport, *TempFilePath))
					{
						// Create Sentry attachment from the file
						USentryAttachment* StatsAttachment = USentryLibrary::CreateSentryAttachmentWithPath(
							TempFilePath,
							TEXT("performance_stats.txt"),
							TEXT("text/plain")
						);
						
						if (StatsAttachment)
						{
							SentrySubsystem->AddAttachment(StatsAttachment);
						}
					}
				}
				
				// Add breadcrumb for performance drop
				SentrySubsystem->AddBreadcrumbWithParams(
					Message,
					TEXT("performance"),
					TEXT("info"),
					SentryPerformanceContext,
					ESentryLevel::Warning
				);
				
			// Capture the performance drop as a warning message
			SentrySubsystem->CaptureMessage(Message, ESentryLevel::Warning);
		}
	}
}