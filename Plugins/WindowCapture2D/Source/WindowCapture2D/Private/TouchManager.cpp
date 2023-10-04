// Fill out your copyright notice in the Description page of Project Settings.


#include "TouchManager.h"
#include "WindowCapture2DLog.h"

bool UTouchManager::Initialize(int TouchNum)
{
	Touches.Empty();

	for (int i = 0; i < TouchNum; i++)
	{
		UTouch* Obj = NewObject<UTouch>();
		Obj->Initialize(i);
		Obj->OnUpdateDelegate.AddUObject(this, &UTouchManager::Update);
		Touches.Add(Obj);
	}

	BOOL InitRes = ::InitializeTouchInjection(TouchNum, POINTER_FEEDBACK_DEFAULT);
	if (!InitRes)
	{
		UE_WC2D_LOG(Error, TEXT("InitFailed: %d %d"), InitRes, ::GetLastError());
		return false;
	}
	return true;
}

UTouch* UTouchManager::GetTouch(int Index)
{
	return Touches[Index];
}

void UTouchManager::Update()
{
	TArray<POINTER_TOUCH_INFO> Infos;
	for (auto& Touch : Touches)
	{
		if (Touch->ShouldUpdate())
		{
			Infos.Add(Touch->GetTouchInfo());
		}
	}

	BOOL Result = ::InjectTouchInput(Infos.Num(), Infos.GetData());
	if (!Result)
	{
		UE_WC2D_LOG(Error, TEXT("FailedtoInject: %d %d"), Result, ::GetLastError());
		for (int32 i = 0; i < Infos.Num(); i++)
		{
			UE_LOG(LogTemp, Warning, TEXT("%s"), *UTouch::DebugInfo(Infos[i]));
		}

	}
}