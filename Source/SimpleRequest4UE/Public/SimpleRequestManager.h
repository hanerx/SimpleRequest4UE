// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SimpleRequest.h"
#include "UObject/Object.h"
#include "SimpleRequestManager.generated.h"

/**
 * 
 */
UCLASS()
class SIMPLEREQUEST4UE_API USimpleRequestManager : public UObject
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable,Category="SimpleRequest|Manager")
	static USimpleRequestManager* Get()
	{
		static USimpleRequestManager* SimpleRequestManager = nullptr;
		if (!SimpleRequestManager)
		{
			SimpleRequestManager=NewObject<USimpleRequestManager>();
			SimpleRequestManager->AddToRoot();
		}
		return SimpleRequestManager;
	}

	UFUNCTION(BlueprintCallable,Category="SimpleRequest|Misc")
	static bool HasWifiConnection()
	{
		return FGenericPlatformMisc::HasActiveWiFiConnection();
	}
	
public:
	UFUNCTION(BlueprintCallable,Category="SimpleRequest|Manager")
	TArray<USimpleRequest*> GetAllRequest();

	UFUNCTION(BlueprintCallable,Category="SimpleRequest|Manager")
	USimpleRequest* GetRequest(const FString& Guid);
	USimpleRequest* GetRequest(const FGuid& Guid);

	UFUNCTION(BlueprintCallable,Category="SimpleRequest|Manager")
	bool AddRequest(const FString& InURL,FString& OutGuid);
	bool AddRequest(const FString& InURL,FGuid& OutGuid);

private:
	UPROPERTY()
	TMap<FGuid,USimpleRequest*> Requests;
};
