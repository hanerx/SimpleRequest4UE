// Fill out your copyright notice in the Description page of Project Settings.


#include "SimpleRequest.h"

#include "HttpModule.h"

USimpleRequest::USimpleRequest()
{
}

USimpleRequest::~USimpleRequest()
{
	if(Status<=Downloading)
	{
		Cancel();
	}
}

void USimpleRequest::Reset()
{
	Status = Init;
	CacheData.Empty();
}

void USimpleRequest::StartDownload()
{
}

void USimpleRequest::Cancel()
{
}

void USimpleRequest::Pause()
{
}

bool USimpleRequest::Retry()
{
	if (Status<=Downloading || Status>=EndOfStatus || FailedTime >= MAX_FAILURE_TIMES)
	{
		return false;
	}
	return false;
}

FString USimpleRequest::GetDownloadContentAsString()
{
	FString OutString;
	return OutString;
}

TArray<uint8> USimpleRequest::GetDownloadContent()
{
	TArray<uint8> OutData;
	if (Status == Success)
	{
		if (!FFileHelper::LoadFileToArray(OutData, *GetFullSavePath()))
		{
		}
	}
	return OutData;
}

bool USimpleRequest::DumpCacheToFile()
{
	if (!FPaths::DirectoryExists(SavePath))
	{
		if (!FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*SavePath))
		{
			return false;
		}
	}
	if (!FFileHelper::SaveArrayToFile(TArray<uint8>(), *GetFullSavePath()))
	{
		return false;
	}
	int32 Index = -1;
	CacheData.StableSort();
	for (int32 i=0;i<CacheData.Num();i++)
	{
		FFrameStruct& FrameStruct=CacheData[i];
		if (!FrameStruct.IsValid() || FPlatformFileManager::Get().GetPlatformFile().FileSize(*GetFullSavePath()) !=
			FrameStruct.StartOffset)
		{
			break;
		}
		if (!FFileHelper::SaveArrayToFile(FrameStruct.FrameData, *GetFullSavePath(), &IFileManager::Get(),
		                                  EFileWrite::FILEWRITE_Append))
		{
			break;
		}
		Index=i;
	}
	if(Index<0)
	{
		return false;
	}
	CacheData.RemoveAtSwap(Index);
	return true;
}

void USimpleRequest::StartMainDownload()
{
	while(DownloadingRequests.Num()<MaxThreat)
	{
		const TSharedPtr<IHttpRequest,ESPMode::ThreadSafe> FrameRequest=FHttpModule::Get().CreateRequest();
		FrameRequest->SetURL(URL);
		FrameRequest->SetVerb(TEXT("GET"));
		FrameRequest->OnProcessRequestComplete();
		FrameRequest->OnRequestProgress();
		DownloadingRequests.Add(FrameRequest);
		FrameRequest->ProcessRequest();
	}
}

void USimpleRequest::StartHeadDownload()
{
	HeadRequest=FHttpModule::Get().CreateRequest();
	HeadRequest->SetURL(URL);
	HeadRequest->SetVerb(TEXT("HEAD"));
	HeadRequest->OnHeaderReceived().BindUObject(this,&USimpleRequest::OnHeadRequestHeaderReceived);
	HeadRequest->OnProcessRequestComplete().BindUObject(this,&USimpleRequest::OnHeadRequestComplete);
	HeadRequest->ProcessRequest();
}

void USimpleRequest::OnHeadRequestHeaderReceived(FHttpRequestPtr Request, const FString& HeaderName,
	const FString& NewHeaderValue)
{
	if(HeaderName==TEXT("Content-Length"))
	{
		TotalSize=FCString::Atoi64(*NewHeaderValue);
	}
}

void USimpleRequest::OnHeadRequestComplete(FHttpRequestPtr Request, FHttpResponsePtr Response,
	bool bConnectedSuccessfully)
{
	if(bConnectedSuccessfully)
	{
		HeadRequest.Reset();
		StartMainDownload();
	}else
	{
		SetStatusToFail();
	}
}

void USimpleRequest::SetStatusToFail()
{
	Status=Failed;
	FailedTime++;
}
