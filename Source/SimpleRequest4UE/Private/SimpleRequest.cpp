// Fill out your copyright notice in the Description page of Project Settings.


#include "SimpleRequest.h"

#include "HttpModule.h"

USimpleRequest::USimpleRequest()
{
}

USimpleRequest::~USimpleRequest()
{
	if (Status <= Downloading)
	{
		Cancel();
	}
}

void USimpleRequest::Reset()
{
	Status = Init;
	CacheData.Empty();
	for (auto Request : DownloadingRequests)
	{
		Request.Reset();
	}
	DownloadingRequests.Empty();
	HeadRequest.Reset();
}

void USimpleRequest::StartDownload()
{
	if (TotalSize == 0)
	{
		StartHeadDownload();
	}
	else
	{
		StartMainDownload();
	}
}

void USimpleRequest::Cancel()
{
	Status = Canceled;
	for (auto Request : DownloadingRequests)
	{
		if (Request && Request.IsValid())
		{
			Request->CancelRequest();
		}
	}
	if (HeadRequest && HeadRequest.IsValid())
	{
		HeadRequest->CancelRequest();
	}
	DumpCacheToFile();
	CacheData.Empty();
}

void USimpleRequest::Pause()
{
}

bool USimpleRequest::Retry()
{
	if (Status <= Downloading || Status >= EndOfStatus || FailedTime >= MAX_FAILURE_TIMES)
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
	for (int32 i = 0; i < CacheData.Num(); i++)
	{
		FFrameStruct& FrameStruct = CacheData[i];
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
		Index = i;
	}
	if (Index < 0)
	{
		return false;
	}
	CacheData.RemoveAtSwap(Index);
	return true;
}

void USimpleRequest::StartMainDownload()
{
	if(!LastFrame.IsValid())
	{
		LastFrame=FFrameStruct(GetDownloadContent().Num(),GetDownloadContent().Num()+FrameLength);
	}
	while (DownloadingRequests.Num() < MaxThreat && CacheData.Num() < MaxThreat&&LastFrame.IsValid())
	{
		const TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> FrameRequest = FHttpModule::Get().CreateRequest();
		FrameRequest->SetURL(URL);
		FrameRequest->SetVerb(TEXT("GET"));
		FrameRequest->SetHeader(TEXT("Range"),LastFrame.ToString());
		int32 Index = DownloadingRequests.Add(FrameRequest);
		FrameRequest->OnProcessRequestComplete().BindLambda(
			[this,Index,FrameRequest](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
			{
				this->OnFrameRequestComplete(LastFrame,Index, Request, Response, bConnectedSuccessfully);
				FrameRequest->OnProcessRequestComplete().Unbind();
				FrameRequest->OnRequestProgress().Unbind();
			});
		FrameRequest->OnRequestProgress().BindLambda(
			[this,Index](FHttpRequestPtr Request, int32 BytesSent, int32 BytesReceived)
			{
				this->OnFrameRequestProcess(LastFrame,Index, Request, BytesSent, BytesReceived);
			});
		FrameRequest->ProcessRequest();
		LastFrame=LastFrame.GetNextFrame(TotalSize,FrameLength);
	}
}

void USimpleRequest::StartHeadDownload()
{
	HeadRequest = FHttpModule::Get().CreateRequest();
	HeadRequest->SetURL(URL);
	HeadRequest->SetVerb(TEXT("HEAD"));
	HeadRequest->OnHeaderReceived().BindUObject(this, &USimpleRequest::OnHeadRequestHeaderReceived);
	HeadRequest->OnProcessRequestComplete().BindUObject(this, &USimpleRequest::OnHeadRequestComplete);
	HeadRequest->ProcessRequest();
}

void USimpleRequest::OnHeadRequestHeaderReceived(FHttpRequestPtr Request, const FString& HeaderName,
                                                 const FString& NewHeaderValue)
{
	if (HeaderName == TEXT("Content-Length"))
	{
		TotalSize = FCString::Atoi64(*NewHeaderValue);
	}
}

void USimpleRequest::OnHeadRequestComplete(FHttpRequestPtr Request, FHttpResponsePtr Response,
                                           bool bConnectedSuccessfully)
{
	if (bConnectedSuccessfully)
	{
		HeadRequest.Reset();
		StartMainDownload();
	}
	else
	{
		SetStatusToFail();
	}
}

void USimpleRequest::OnFrameRequestComplete(FFrameStruct FrameStruct,int32 Index, FHttpRequestPtr Request, FHttpResponsePtr Response,
	bool bConnectedSuccessfully)
{
	if(Response==nullptr)
	{
		SetStatusToFail();
		
	}
}

void USimpleRequest::OnFrameRequestProcess(FFrameStruct FrameStruct,int32 Index, FHttpRequestPtr Request, int32 BytesSent, int32 BytesReceived)
{
}

void USimpleRequest::SetStatusToFail()
{
	Status = Failed;
	FailedTime++;
}
