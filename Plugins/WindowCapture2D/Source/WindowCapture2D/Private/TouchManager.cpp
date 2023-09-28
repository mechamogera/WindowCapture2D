// Fill out your copyright notice in the Description page of Project Settings.


#include "TouchManager.h"

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
		UE_LOG(LogTemp, Warning, TEXT("InitFailed: %d %d"), InitRes, ::GetLastError());
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

	InjectTouch(Infos.Num(), Infos.GetData());
	/*BOOL Result = ::InjectTouchInput(Infos.Num(), Infos.GetData());
	if (!Result)
	{
		UE_LOG(LogTemp, Warning, TEXT("FailedtoInject: %d %d"), Result, ::GetLastError());
	}*/
}

void UTouchManager::InjectTouch(UINT32 count, const POINTER_TOUCH_INFO* info)
{
	BOOL Result = ::InjectTouchInput(count, info);
	if (!Result)
	{
		UE_LOG(LogTemp, Warning, TEXT("FailedtoInject: %d %d"), Result, ::GetLastError());
		for (UINT32 i = 0; i < count; i++)
		{
			UE_LOG(LogTemp, Warning, TEXT("%s"), *UTouch::DebugInfo(info[i]));
		}

	}
}