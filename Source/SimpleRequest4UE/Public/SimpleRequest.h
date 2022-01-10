// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SimpleRequest.generated.h"

#define MAX_THREAT_FOR_REQUEST 4
#define FRAME_LENGTH 10485760

UENUM(BlueprintType)
enum ERequestStatus
{
	Init,
	PreDownload,
	HeadRequest,
	Downloading,
	Success,
	Failed,
	EndOfStatus
};

USTRUCT()
struct FFrameStruct
{
	GENERATED_BODY()
	FFrameStruct(const int64 StartOffset, const int64 EndOffset):StartOffset(StartOffset),EndOffset(EndOffset){}
	FFrameStruct(const FFrameStruct& Other){StartOffset=Other.StartOffset;EndOffset=Other.EndOffset;}
	FFrameStruct():StartOffset(0),EndOffset(0){}
public:
	int64 StartOffset;
	int64 EndOffset;
	TArray<uint8> FrameData;
public:
	friend bool operator==(const FFrameStruct& Lhs, const FFrameStruct& RHS)
	{
		return Lhs.StartOffset == RHS.StartOffset
			&& Lhs.EndOffset == RHS.EndOffset;
	}

	friend bool operator!=(const FFrameStruct& Lhs, const FFrameStruct& RHS)
	{
		return !(Lhs == RHS);
	}

	friend bool operator<(const FFrameStruct& Lhs, const FFrameStruct& RHS)
	{
		return Lhs.StartOffset<RHS.StartOffset;
	}
	
	friend bool operator>(const FFrameStruct& Lhs, const FFrameStruct& RHS)
	{
		return Lhs.StartOffset>RHS.StartOffset;
	}

	FString ToString() const
	{
		return FString::Printf(TEXT("bytes=%lld-%lld"),StartOffset,EndOffset);
	}

	bool IsValid() const
	{
		return StartOffset<EndOffset;
	}
};

/**
 * @author Hanerx
 * The main struct for request
 * 
 */
UCLASS()
class SIMPLEREQUEST4UE_API USimpleRequest : public UObject
{
	GENERATED_BODY()
public:
	USimpleRequest();
	virtual ~USimpleRequest() override;
public:
	UFUNCTION(BlueprintCallable,Category="SimpleRequest|Download")
	void Reset();

	UFUNCTION(BlueprintCallable,Category="SimpleRequest|Download")
	void StartDownload();

	UFUNCTION(BlueprintCallable,Category="SimpleRequest|Download")
	void Cancel();

	UFUNCTION(BlueprintCallable,Category="SimpleRequest|Download")
	void Pause();

	UFUNCTION(BlueprintCallable,Category="SimpleRequest|Data")
	FString GetDownloadContentAsString();

	UFUNCTION(BlueprintCallable,Category="SimpleRequest|Data")
	TArray<uint8> GetDownloadContent();

public:
	UFUNCTION(BlueprintCallable,Category="SimpleRequest|Setting")
	FORCEINLINE void SetIfDownloadToFile(const bool bIfDownloadToFile) { bDownloadToFile = bIfDownloadToFile; }

	UFUNCTION(BlueprintCallable,Category="SimpleReques|Setting")
	FORCEINLINE bool GetIfDownloadToFile() const { return bDownloadToFile; }

	UFUNCTION(BlueprintCallable,Category="SimpleRequest|Setting")
	FORCEINLINE void SetSavePath(const FString& InSavePath) { SavePath = InSavePath; }

	UFUNCTION(BlueprintCallable,Category="SimpleRequest|Setting")
	FORCEINLINE FString GetSavePath() const { return SavePath; }

	UFUNCTION(BlueprintCallable,Category="SimpleRequest|Setting")
	FORCEINLINE void SetURL(const FString& InURL){URL=InURL;}
	
	UFUNCTION(BlueprintCallable,Category="SimpleRequest|Setting")
	FORCEINLINE FString GetURL() const {return URL;}

	UFUNCTION(BlueprintCallable,Category="SimpleRequest|Setting")
	FORCEINLINE FString GetFullSavePath() const {return SavePath/FileName;}

	UFUNCTION(BlueprintCallable,Category="SimpleRequest|Setting")
	FORCEINLINE ERequestStatus GetStatus() const {return Status;}

protected:
	bool DumpCacheToFile();
	
private:
	bool bDownloadToFile;
	FString SavePath;
	FString FileName;

	int32 FrameLength=FRAME_LENGTH;
	FString URL;

	TArray<FFrameStruct> CacheData;

	ERequestStatus Status;
};
