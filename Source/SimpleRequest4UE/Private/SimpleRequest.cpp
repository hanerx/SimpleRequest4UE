// Fill out your copyright notice in the Description page of Project Settings.


#include "SimpleRequest.h"

#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"

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
	for (auto FrameStruct : DownloadingRequests)
	{
		if(FrameStruct.FrameStatus!=Success)
		{
			FrameStruct.Request.Reset();
			FrameStruct.FrameStatus=Init;
		}
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
		GenerateFrameStructs();
		StartMainDownload();
	}
}

void USimpleRequest::Cancel()
{
	Status = Canceled;
	for (auto FrameStruct : DownloadingRequests)
	{
		if (FrameStruct.Request && FrameStruct.Request.IsValid()&&FrameStruct.FrameStatus<Success)
		{
			FrameStruct.Request->CancelRequest();
			FrameStruct.FrameStatus=Canceled;
		}
	}
	if (HeadRequest && HeadRequest.IsValid())
	{
		HeadRequest->CancelRequest();
	}
	DumpCacheToFile();
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

FString USimpleRequest::GetDownloadContentAsString() const
{
	FString OutString;
	if (Status == Success)
	{
		if (!FFileHelper::LoadFileToString(OutString, *GetFullSavePath()))
		{
		}
	}
	return OutString;
}

TArray<uint8> USimpleRequest::GetDownloadContent() const
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

int64 USimpleRequest::GetAlreadyDownloadedSize() const
{
	return FPlatformFileManager::Get().GetPlatformFile().FileSize(*GetFullSavePath());
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
	for (auto& FrameStruct:DownloadingRequests)
	{
		if (!FrameStruct.IsValid() || FPlatformFileManager::Get().GetPlatformFile().FileSize(*GetFullSavePath()) !=
			FrameStruct.StartOffset||FrameStruct.FrameStatus!=WaitForDump)
		{
			continue;
		}
		if (!FFileHelper::SaveArrayToFile(FrameStruct.FrameData, *GetFullSavePath(), &IFileManager::Get(),
		                                  EFileWrite::FILEWRITE_Append))
		{
			return false;
		}
		FrameStruct.FrameData.Empty();
		FrameStruct.FrameStatus=Success;
	}
	return true;
}

void USimpleRequest::StartMainDownload()
{
	int32 Flag=0;
	for(int32 Index=0;Index<DownloadingRequests.Num();Index++)
	{
		auto& FrameStruct=DownloadingRequests[Index];
		if(FrameStruct.FrameStatus>Init&&FrameStruct.FrameStatus<Success)
		{
			Flag++;
		}
		else if(Flag<MaxThreat)
		{
			FrameStruct.Request = FHttpModule::Get().CreateRequest();
			FrameStruct.Request->SetURL(URL);
			FrameStruct.Request->SetVerb(TEXT("GET"));
			FrameStruct.Request->SetHeader(TEXT("Range"),FrameStruct.ToString());
			FrameStruct.Request->OnProcessRequestComplete().BindLambda(
				[this,Index,FrameStruct](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
				{
					this->OnFrameRequestComplete(FrameStruct,Index, Request, Response, bConnectedSuccessfully);
					FrameStruct.Request->OnProcessRequestComplete().Unbind();
					FrameStruct.Request->OnRequestProgress().Unbind();
				});
			FrameStruct.Request->OnRequestProgress().BindLambda(
				[this,Index,FrameStruct](FHttpRequestPtr Request, int32 BytesSent, int32 BytesReceived)
				{
					this->OnFrameRequestProcess(FrameStruct,Index, Request, BytesSent, BytesReceived);
				});
			FrameStruct.Request->ProcessRequest();
			FrameStruct.FrameStatus=Downloading;
			Flag++;
		}
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

void USimpleRequest::GenerateFrameStructs()
{
	if(DownloadingRequests.Num()>0)
	{
		return;
	}
	FFrameStruct FrameStruct=FFrameStruct(GetDownloadContent().Num(),GetDownloadContent().Num()+FrameLength<TotalSize?GetDownloadContent().Num()+FrameLength:TotalSize);
	while (FrameStruct.IsValid())
	{
		DownloadingRequests.Add(FrameStruct);
		FrameStruct=FrameStruct.GetNextFrame(TotalSize,FrameLength);
	}
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
		GenerateFrameStructs();
		StartMainDownload();
	}
	else
	{
		SetStatusToFail();
	}
}

void USimpleRequest::OnFrameRequestComplete(FFrameStruct& FrameStruct,int32 Index, FHttpRequestPtr Request, FHttpResponsePtr Response,
	bool bConnectedSuccessfully)
{
	if(Response==nullptr||!EHttpResponseCodes::IsOk(Response->GetResponseCode()))
	{
		SetStatusToFail();
		FrameStruct.FrameStatus=Failed;
		FrameStruct.Request.Reset();
		return;
	}
	FrameStruct.FrameData=Response->GetContent();
	FrameStruct.FrameStatus=WaitForDump;
	FrameStruct.Request.Reset();
	DumpCacheToFile();
	StartMainDownload();
}

void USimpleRequest::OnFrameRequestProcess(FFrameStruct& FrameStruct,int32 Index, FHttpRequestPtr Request, int32 BytesSent, int32 BytesReceived)
{
	FrameStruct.CurrentSize=BytesReceived;
}

void USimpleRequest::SetStatusToFail()
{
	Status = Failed;
	FailedTime++;
}
