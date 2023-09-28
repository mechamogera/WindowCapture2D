// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/AllowWindowsPlatformAtomics.h"
#include "Windows/PreWindowsApi.h"

#include "winuser.h"

#include "Windows/PostWindowsApi.h"
#include "Windows/HideWindowsPlatformAtomics.h"
#include "Windows/HideWindowsPlatformTypes.h"

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "TouchWindow.generated.h"

/**
 * 
 */
UCLASS(BlueprintType, Blueprintable)
class WINDOWCAPTURE2D_API UTouchWindow : public UObject
{
	GENERATED_BODY()

public: 
	static FString DebugInfo(POINTER_TOUCH_INFO info);

public:
	DECLARE_MULTICAST_DELEGATE(FOnUpdateDelegate);

	FOnUpdateDelegate OnUpdateDelegate;

public:
	void Initialize(int Id);

	UFUNCTION(BlueprintCallable)
	void TouchDown(int x, int y);
	
	UFUNCTION(BlueprintCallable)
	void TouchHover(int x, int y);

	UFUNCTION(BlueprintCallable)
	void TouchRelease(int x, int y);

	bool ShouldUpdate();

	POINTER_TOUCH_INFO GetTouchInfo() { return TouchInfo; }

private:
	enum State
	{
		Release,
		Hover,
		Touch
	};

	enum class StateDetail : POINTER_FLAGS
	{
		None = POINTER_FLAG_NONE,
		HoverStart = POINTER_FLAG_UPDATE | POINTER_FLAG_INRANGE,
		HoverMove = POINTER_FLAG_UPDATE | POINTER_FLAG_INRANGE,
		HoverEnd = POINTER_FLAG_UPDATE,
		TouchStart = POINTER_FLAG_DOWN | POINTER_FLAG_INRANGE | POINTER_FLAG_INCONTACT,
		TouchMove = POINTER_FLAG_UPDATE | POINTER_FLAG_INRANGE | POINTER_FLAG_INCONTACT,
		TouchEnd = POINTER_FLAG_UP | POINTER_FLAG_INRANGE,
		Cancel = POINTER_FLAG_UPDATE | POINTER_FLAG_CANCELED,
	};

private:
	void SetTouchInfo(StateDetail StateDatail, int x, int y);
	void UpdateState(State NewState, int x, int y);

private:
	State CurrentState = State::Release;
	StateDetail CurrentStateDetail = StateDetail::None;
	POINTER_TOUCH_INFO TouchInfo = { 0 };
	int Marge = 2;
	int PrevX = 0;
	int PrevY = 0;
};
