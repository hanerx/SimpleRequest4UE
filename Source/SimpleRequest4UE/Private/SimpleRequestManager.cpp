// Fill out your copyright notice in the Description page of Project Settings.


#include "SimpleRequestManager.h"

TArray<USimpleRequest*> USimpleRequestManager::GetAllRequest()
{
	TArray<USimpleRequest*> OutData;
	for(const auto& Tuple:Requests)
	{
		OutData.Add(Tuple.Value);
	}
	return OutData;
}

USimpleRequest* USimpleRequestManager::GetRequest(const FString& Guid)
{
	const FGuid InGuid=FGuid(Guid);
	return GetRequest(InGuid);
}

USimpleRequest* USimpleRequestManager::GetRequest(const FGuid& Guid)
{
	if(!Guid.IsValid())
	{
		return nullptr;
	}
	return *Requests.Find(Guid);
}

bool USimpleRequestManager::AddRequest(const FString& InURL, FString& OutGuid)
{
	FGuid Guid;
	const bool bFlag=AddRequest(InURL,Guid);
	if(bFlag)
	{
		OutGuid=Guid.ToString();
	}
	return bFlag;
}

bool USimpleRequestManager::AddRequest(const FString& InURL, FGuid& OutGuid)
{
	FPlatformMisc::CreateGuid(OutGuid);
	USimpleRequest* SimpleRequest=Requests.Add(OutGuid,NewObject<USimpleRequest>());
	SimpleRequest->SetURL(InURL);
	SimpleRequest->AddToRoot();
	SimpleRequest->StartDownload();
	return true;
}
