// Copyright (c) 2024 Sentry. All Rights Reserved.

#include "SentryTowerGameInstance.h"

#include "HttpModule.h"
#include "SentryLibrary.h"
#include "SentrySpan.h"
#include "SentrySubsystem.h"
#include "SentryTransaction.h"
#include "Interfaces/IHttpResponse.h"

void USentryTowerGameInstance::Init()
{
	Super::Init();

	if (FParse::Param(FCommandLine::Get(), TEXT("NullRHI")))
	{
		// For CI simulation (no RHI available) copy pre-made screenshot to dest where Unreal SDK can pick it up during crash handling
		const FString FakeScreenshotPath = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Resources"), TEXT("screenshot.png"));
		
		// Get the Sentry subsystem
		USentrySubsystem* SentrySubsystem = GEngine->GetEngineSubsystem<USentrySubsystem>();
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
}

void USentryTowerGameInstance::BuyUpgrade(const FOnBuyComplete& OnBuyComplete)
{
	USentrySubsystem* Sentry = GEngine->GetEngineSubsystem<USentrySubsystem>();

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

		CheckoutTransaction->Finish();
	});

	HttpRequest->ProcessRequest();
}
