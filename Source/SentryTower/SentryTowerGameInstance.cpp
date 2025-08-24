// Copyright (c) 2024 Sentry. All Rights Reserved.

#include "SentryTowerGameInstance.h"

#include "HttpModule.h"
#include "SentryLibrary.h"
#include "SentrySettings.h"
#include "SentrySpan.h"
#include "SentrySubsystem.h"
#include "SentryTransaction.h"
#include "SentryTransactionContext.h"
#include "SentryPerformance.h"
#include "Interfaces/IHttpResponse.h"

void USentryTowerGameInstance::Init()
{
	Super::Init();

	// Initialize Sentry with environment variable override for DSN
	USentrySubsystem* SentrySubsystem = GEngine->GetEngineSubsystem<USentrySubsystem>();
	if (SentrySubsystem)
	{
		FString EnvironmentDsn = FPlatformMisc::GetEnvironmentVariable(TEXT("SENTRY_DSN"));
		if (!EnvironmentDsn.IsEmpty())
		{
			// Override DSN with environment variable
			UE_LOG(LogTemp, Log, TEXT("Using SENTRY_DSN environment variable!"));
			SentrySubsystem->InitializeWithSettings(FConfigureSettingsNativeDelegate::CreateLambda([EnvironmentDsn](USentrySettings* Settings)
			{
				Settings->Dsn = EnvironmentDsn;
			}));
		}
		else
		{
			// Use default settings
			UE_LOG(LogTemp, Log, TEXT("SENTRY_DSN environment variable not set, using default settings"));
			SentrySubsystem->Initialize();
		}
	}

	if (FParse::Param(FCommandLine::Get(), TEXT("NullRHI")))
	{
		// For CI simulation (no RHI available) copy pre-made screenshot to dest where Unreal SDK can pick it up during crash handling
		const FString FakeScreenshotPath = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Resources"), TEXT("screenshot.png"));
		
		// Add screenshot attachment to Sentry
		if (SentrySubsystem)
		{
			// Create the attachment
			USentryAttachment* ScreenshotAttachment = USentryLibrary::CreateSentryAttachmentWithPath(
				FakeScreenshotPath,
				TEXT("screenshot.png"),
				TEXT("image/png")
			);

			// Add to subsystem scope (will be included with all events)
			if (ScreenshotAttachment)
			{
				SentrySubsystem->AddAttachment(ScreenshotAttachment);
			}
		}
	}

	// Performance monitoring is now handled automatically by SentrySubsystem
	// based on the settings in ProjectSettings > Plugins > SentrySDK > Performance Monitoring
	UE_LOG(LogTemp, Log, TEXT("Performance monitoring managed by SentrySubsystem"));
}

void USentryTowerGameInstance::BuyUpgrade(const FOnBuyComplete& OnBuyComplete)
{
	USentrySubsystem* Sentry = GEngine->GetEngineSubsystem<USentrySubsystem>();

	// Trigger a test performance drop to demonstrate monitoring
	TriggerTestPerformanceDrop();

	USentryTransaction* CheckoutTransaction = Sentry->StartTransaction(TEXT("checkout"), TEXT("http.client"));

	USentrySpan* ProcessSpan = CheckoutTransaction->StartChildSpan(TEXT("task"), TEXT("process_upgrade_data"));

	TSharedPtr<FJsonObject> UpgradeDataJsonObject = MakeShareable(new FJsonObject());
	UpgradeDataJsonObject->SetStringField(TEXT("UpgradeName"), TEXT("NewTower"));
	UpgradeDataJsonObject->SetStringField(TEXT("PlayerEmail"), TEXT("player@sentry-tower.com"));

	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	if (!FJsonSerializer::Serialize(UpgradeDataJsonObject.ToSharedRef(), Writer))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to serialize JSON object"));
	}

	FPlatformProcess::Sleep(0.1f);

	ProcessSpan->Finish();

	USentrySpan* CheckoutSpan = CheckoutTransaction->StartChildSpan(TEXT("task"), TEXT("checkout_request"));
	FString Domain = TEXT("https://aspnetcore.empower-plant.com");
	FString Endpoint = TEXT("/checkout");
	FString CheckoutURL = Domain + Endpoint;

	FHttpModule* Http = &FHttpModule::Get();
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = Http->CreateRequest();

	HttpRequest->SetURL(CheckoutURL);
	HttpRequest->SetVerb("POST");
	HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

	HttpRequest->SetContentAsString(JsonString);

	HttpRequest->OnProcessRequestComplete().BindLambda([=](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
	{
		CheckoutSpan->Finish();

		USentrySpan* ResponseSpan = CheckoutTransaction->StartChildSpan(TEXT("task"), TEXT("process_checkout_response"));			
		ensureMsgf(bWasSuccessful && Response.IsValid() && Response->GetResponseCode() == 200, TEXT("Checkout HTTP request failed"));

		if (bWasSuccessful && Response.IsValid() && Response->GetResponseCode() == 200)
		{
			UE_LOG(LogTemp, Log, TEXT("Checkout completed"));
			OnBuyComplete.ExecuteIfBound(true);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Checkout failed"));
			OnBuyComplete.ExecuteIfBound(false);
		}

		ResponseSpan->Finish();
		CheckoutTransaction->Finish();
	});

	HttpRequest->ProcessRequest();
}

void USentryTowerGameInstance::TriggerTestPerformanceDrop()
{
	USentrySubsystem* SentrySubsystem = GEngine->GetEngineSubsystem<USentrySubsystem>();
	USentryPerformance* PerformanceMonitor = SentrySubsystem ? SentrySubsystem->GetPerformanceMonitor() : nullptr;
	
	if (!PerformanceMonitor)
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot trigger test performance drop - performance monitoring not available"));
		return;
	}

	// Get current frame data and populate it with detailed stats
	FFrameTimingData TestFrameData = PerformanceMonitor->GetCurrentFrameTimings();

	// Create a test performance drop event
	FPerformanceDropEvent TestEvent;
	TestEvent.DropSeverity = 1.0f;
	TestEvent.FrameData = TestFrameData;
	TestEvent.PreviousAverageFPS = PerformanceMonitor->GetAverageFPS();
	TestEvent.DropReason = TEXT("Test performance drop triggered manually from SentryTower game");

	// Broadcast the event (this will trigger any bound delegates)
	PerformanceMonitor->OnPerformanceDrop.Broadcast(TestEvent);

	UE_LOG(LogTemp, Warning, TEXT("Test performance drop triggered with detailed stats"));
}

void USentryTowerGameInstance::OnPerformanceDropDetected(const FPerformanceDropEvent& DropEvent)
{
	UE_LOG(LogTemp, Warning, TEXT("Performance drop detected in Game Instance: %s (FPS: %.1f, Severity: %.2f)"), 
		*DropEvent.DropReason, DropEvent.FrameData.FPS, DropEvent.DropSeverity);

	// You can add additional handling here, such as:
	// - Automatically reducing graphics settings
	// - Showing a warning to the player
	// - Logging additional context for debugging
	// - Triggering gameplay adjustments to maintain performance
}
