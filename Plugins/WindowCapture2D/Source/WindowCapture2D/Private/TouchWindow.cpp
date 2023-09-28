// Fill out your copyright notice in the Description page of Project Settings.


#include "TouchWindow.h"

FString UTouchWindow::DebugInfo(POINTER_TOUCH_INFO info)
{
	FString Flag;
	switch (static_cast<StateDetail>(info.pointerInfo.pointerFlags))
	{
	case StateDetail::None:
		Flag = TEXT("None");
		break;
	case StateDetail::HoverStart:
		Flag = TEXT("HoverStart or HoverMove");
		break;
	case StateDetail::HoverEnd:
		Flag = TEXT("HoverEnd");
		break;
	case StateDetail::TouchStart:
		Flag = TEXT("TouchStart");
		break;
	case StateDetail::TouchMove:
		Flag = TEXT("TouchMove");
		break;
	case StateDetail::TouchEnd:
		Flag = TEXT("TouchEnd");
		break;
	case StateDetail::Cancel:
		Flag = TEXT("Cancel");
		break;
	default:
		Flag = FString::Format(TEXT("Unknown({0})"), { info.pointerInfo.pointerFlags });
		break;
	}

	FString InfoStr;
	InfoStr = FString::Format(TEXT(
		"\tID:{0}\n"
		"\tX:{1}\n"
		"\tY:{2}\n"
		"\tLeft:{3}\n"
		"\tRight:{4}\n"
		"\tTop:{5}\n"
		"\tBottom:{6}\n"
		"\tPointerFlag:{7}\n"
		"\tPointerType:{8}\n"
		"\tTouchFlag:{9}\n"
		"\tOrientation:{10}\n"
		"\tPressure:{11}\n"
		"\tTouchMask:{12}\n"),
		{ info.pointerInfo.pointerId,
		  info.pointerInfo.ptPixelLocation.x,
		  info.pointerInfo.ptPixelLocation.y,
		  info.rcContact.left,
		  info.rcContact.right,
		  info.rcContact.top,
		  info.rcContact.bottom,
		  *Flag,
		  static_cast<int>(info.pointerInfo.pointerType),
		  info.touchFlags,
		  info.orientation,
		  info.pressure,
		  info.touchMask
		});
	return InfoStr;
}

void UTouchWindow::Initialize(int Id)
{
	TouchInfo.pointerInfo.pointerType = PT_TOUCH;
	TouchInfo.pointerInfo.pointerId = Id;
	TouchInfo.touchFlags = TOUCH_FLAG_NONE;
	TouchInfo.orientation = 90;
	TouchInfo.pressure = 32000;
	TouchInfo.touchMask = TOUCH_MASK_CONTACTAREA | TOUCH_MASK_ORIENTATION | TOUCH_MASK_PRESSURE;
	SetTouchInfo(StateDetail::None, 100, 100);
}

void UTouchWindow::TouchDown(int x, int y)
{
	UpdateState(State::Touch, x, y);
}

void UTouchWindow::TouchHover(int x, int y)
{
	UpdateState(State::Hover, x, y);
}

void UTouchWindow::TouchRelease(int x, int y)
{
	UpdateState(State::Release, x, y);
}

bool UTouchWindow::ShouldUpdate()
{
	return static_cast<StateDetail>(TouchInfo.pointerInfo.pointerFlags) != StateDetail::None;
}

void UTouchWindow::SetTouchInfo(StateDetail StateDatail, int x, int y)
{
	TouchInfo.pointerInfo.pointerFlags = static_cast<POINTER_FLAGS>(StateDatail);
	TouchInfo.pointerInfo.ptPixelLocation.x = x;
	TouchInfo.pointerInfo.ptPixelLocation.y = y;
	TouchInfo.rcContact.left = x - Marge;
	TouchInfo.rcContact.right = x + Marge;
	TouchInfo.rcContact.top = y - Marge;
	TouchInfo.rcContact.bottom = y + Marge;
}

void UTouchWindow::UpdateState(State NewState, int x, int y)
{
	if (NewState == CurrentState)
	{
		switch (CurrentState)
		{
		case State::Hover:
			SetTouchInfo(StateDetail::HoverMove, x, y);
			OnUpdateDelegate.Broadcast();
			break;
		case State::Touch:
			SetTouchInfo(StateDetail::TouchMove, x, y);
			OnUpdateDelegate.Broadcast();
			break;
		}

		PrevX = x;
		PrevY = y;

		return;
	}

	switch (CurrentState)
	{
	case State::Release:
		switch (NewState)
		{
		case State::Touch:
			SetTouchInfo(StateDetail::HoverStart, x, y);
			OnUpdateDelegate.Broadcast();
			break;
		}
		break;
	case State::Hover:
		switch (NewState)
		{
		case State::Release:
			SetTouchInfo(StateDetail::HoverEnd, PrevX, PrevY);
			OnUpdateDelegate.Broadcast();
			break;
		case State::Touch:
			SetTouchInfo(StateDetail::HoverMove, x, y);
			OnUpdateDelegate.Broadcast();
			break;
		}
		break;
	case State::Touch:
		switch (NewState)
		{
		case State::Release:
			SetTouchInfo(StateDetail::TouchEnd, PrevX, PrevY);
			OnUpdateDelegate.Broadcast();
			SetTouchInfo(StateDetail::HoverEnd, PrevX, PrevY);
			OnUpdateDelegate.Broadcast();
			break;
		case State::Hover:
			SetTouchInfo(StateDetail::TouchEnd, PrevX, PrevY);
			OnUpdateDelegate.Broadcast();
			break;
		}
		break;
	}

	switch (NewState)
	{
	case State::Release:
		SetTouchInfo(StateDetail::None, x, y);
		break;
	case State::Hover:
		SetTouchInfo(StateDetail::HoverStart, x, y);
		OnUpdateDelegate.Broadcast();
		SetTouchInfo(StateDetail::HoverMove, x, y);
		break;
	case State::Touch:
		SetTouchInfo(StateDetail::TouchStart, x, y);
		OnUpdateDelegate.Broadcast();
		SetTouchInfo(StateDetail::TouchMove, x, y);
		break;
	}

	CurrentState = NewState;
	PrevX = x;
	PrevY = y;
}