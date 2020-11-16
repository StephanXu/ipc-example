#pragma once

#include <Windows.h>

#ifndef B_TIMER_H
#define B_TIMER_H

#include <MMSystem.h>
#pragma comment(lib, "winmm.lib")

class Timer
{
public:
	Timer()
	{
		TimerRunning_Flag = false;
		TimerID = 0;
	}

	void SetTimer(unsigned int Delay, LPTIMECALLBACK TimerProc, DWORD_PTR lpParam = 0, unsigned int Resolution = 10)
	{
		TimerID = timeSetEvent(Delay, Resolution, TimerProc, lpParam, TIME_PERIODIC);
		TimerRunning_Flag = true;
	}

	void StopTimer()
	{
		if (TimerRunning_Flag)
			timeKillEvent(TimerID);
		TimerRunning_Flag = false;
	}

	unsigned int GetTimerID()
	{
		return TimerID;
	}

	bool GetTimerState()
	{
		return TimerRunning_Flag;
	}

	/*
	void CALLBACK TimeProc(UINT uID, UINT uMsg, DWORD dwUsers, DWORD dw1, DWORD dw2)
	{
	cout << *(string*)dwUsers << endl;
	}
	*/
private:
	unsigned int TimerID;
	bool TimerRunning_Flag;
};

#endif
