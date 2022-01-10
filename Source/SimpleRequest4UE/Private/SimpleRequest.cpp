// Fill out your copyright notice in the Description page of Project Settings.


#include "SimpleRequest.h"

USimpleRequest::USimpleRequest()
{
}

USimpleRequest::~USimpleRequest()
{
}

void USimpleRequest::Reset()
{
	Status=Init;
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

FString USimpleRequest::GetDownloadContentAsString()
{
	FString OutString;
	return OutString;
}

TArray<uint8> USimpleRequest::GetDownloadContent()
{
	TArray<uint8> OutData;
	if(Status==Success)
	{
		if(!FFileHelper::LoadFileToArray(OutData,*GetFullSavePath()))
		{
		}
	}
	return OutData;
}

bool USimpleRequest::DumpCacheToFile()
{
	if(!FPaths::DirectoryExists(SavePath))
	{
		if(!FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*SavePath))
		{
			
			return false;
		}
	}
	if(!FFileHelper::SaveArrayToFile(TArray<uint8>(),*GetFullSavePath()))
	{
		return false;
	}

	CacheData.StableSort();
	for(const auto& FrameStruct:CacheData)
	{
		if(!FrameStruct.IsValid()||FPlatformFileManager::Get().GetPlatformFile().FileSize(*GetFullSavePath())!=FrameStruct.StartOffset)
		{
			return false;
		}
		if(!FFileHelper::SaveArrayToFile(FrameStruct.FrameData,*GetFullSavePath(),&IFileManager::Get(),EFileWrite::FILEWRITE_Append))
		{
			return false;
		}
	}
	CacheData.Empty();
	return true;
}
