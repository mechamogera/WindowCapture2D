// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "TouchWindow.h"
#include "TouchWindowManager.generated.h"

/**
 * 
 */
UCLASS(BlueprintType, Blueprintable)
class WINDOWCAPTURE2D_API UTouchWindowManager : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	bool Initialize(int TouchNum = 1);

	UFUNCTION(BlueprintPure)
	UTouchWindow* GetTouch(int Index);

private:
	void Update();

	void InjectTouch(UINT32 count, const POINTER_TOUCH_INFO* info);

private:
	UPROPERTY(Transient)
	TArray<UTouchWindow*> Touches;
};
