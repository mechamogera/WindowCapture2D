// Fill out your copyright notice in the Description page of Project Settings.


#include "TouchWidnow.h"

bool UTouchWidnow::Initialize()
{
	BOOL InitRes = ::InitializeTouchInjection(10, TOUCH_FEEDBACK_NONE);
	if (!InitRes)
	{
		UE_LOG(LogTemp, Warning, TEXT("InitFailed: %d %d"), InitRes, ::GetLastError());
		return false;
	}

	TouchInfo.pointerInfo.pointerType = PT_TOUCH;
	TouchInfo.pointerInfo.pointerId = 0;
	TouchInfo.touchFlags = TOUCH_FLAG_NONE;
	TouchInfo.orientation = 90;
	TouchInfo.pressure = 32000;
	TouchInfo.touchMask = TOUCH_MASK_CONTACTAREA | TOUCH_MASK_ORIENTATION | TOUCH_MASK_PRESSURE;


	return true;
}

void UTouchWidnow::TouchDown(int x, int y)
{
	UpdateState(State::Touch, x, y);
}

void UTouchWidnow::TouchHover(int x, int y)
{
	UpdateState(State::Hover, x, y);
}

void UTouchWidnow::TouchRelease(int x, int y)
{
	UpdateState(State::Release, x, y);
}

bool UTouchWidnow::InjectTouch(StateDetail StateDatail, int x, int y)
{
	TouchInfo.pointerInfo.pointerFlags = static_cast<POINTER_FLAGS>(StateDatail);
	TouchInfo.pointerInfo.ptPixelLocation.x = x;
	TouchInfo.pointerInfo.ptPixelLocation.y = y;
	TouchInfo.rcContact.left = x - Marge;
	TouchInfo.rcContact.right = x + Marge;
	TouchInfo.rcContact.top = y - Marge;
	TouchInfo.rcContact.bottom = y + Marge;
	BOOL Result = ::InjectTouchInput(1, &TouchInfo);
	if (!Result)
	{
		UE_LOG(LogTemp, Warning, TEXT("FailedtoInject: %d %d"), Result, ::GetLastError());
		return false;
	}
	return true;
}

bool UTouchWidnow::UpdateState(State NewState, int x, int y)
{
	if (NewState == CurrentState)
	{
		PrevX = x;
		PrevY = y;

		switch (CurrentState)
		{
		case State::Release:
			return true;
		case State::Hover:
			return InjectTouch(StateDetail::HoverMove, x, y);
		case State::Touch:
			return InjectTouch(StateDetail::TouchMove, x, y);
		}
	}

	switch (CurrentState)
	{
	case State::Hover:
		switch (NewState)
		{
		case State::Release:
			InjectTouch(StateDetail::HoverEnd, PrevX, PrevY);
			break;
		case State::Touch:
			InjectTouch(StateDetail::HoverMove, x, y);
			break;
		}
		break;
	case State::Touch:
		InjectTouch(StateDetail::TouchEnd, PrevX, PrevY);
		break;
	}

	switch (NewState)
	{
	case State::Hover:
		InjectTouch(StateDetail::HoverStart, x, y);
	case State::Touch:
		InjectTouch(StateDetail::TouchStart, x, y);
	}

	CurrentState = NewState;
	PrevX = x;
	PrevY = y;

	return true;
}