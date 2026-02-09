#include "TbcLfg.h"

#include "Game\Interactions.h"
#include "Geometry\Coordinate.h"

#include "Memory\Endscene.h"
#include "Memory\Hacks.h"

#include "WoW API\Lua.h"
#include "WoW API\Chat.h"
#include "WoW API\Spell.h"
#include "WoW API\CGLoot.h"

#include <time.h>
#include <unordered_set>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <map>

DWORD LFGState = LFGSTATE_START;

static char* trim(char* s) {
	char* end;
	while (isspace((unsigned char)*s)) s++;
	if (*s == 0) return s;
	end = s + strlen(s) - 1;
	while (end > s && isspace((unsigned char)*end)) end--;
	end[1] = '\0';
	return s;
}

WOWPOS* read_xyz_list(const char* filename, size_t* out_count) {
	FILE* f = fopen(filename, "r");
	if (!f) {
		if (out_count) *out_count = 0;
		return NULL;
	}

	size_t cap = 16;
	size_t count = 0;
	WOWPOS* arr = (WOWPOS*)malloc(cap * sizeof(WOWPOS));
	if (!arr) { fclose(f); if (out_count) *out_count = 0; return NULL; }

	char line[1024];
	while (fgets(line, sizeof(line), f)) {
		char* p = line;
		// trim whitespace/newline
		p = trim(p);
		if (*p == '\0') continue;

		// split by commas
		char* tok1 = strtok(p, ",");
		char* tok2 = tok1 ? strtok(NULL, ",") : NULL;
		char* tok3 = tok2 ? strtok(NULL, ",") : NULL;
		char* tok4 = tok3 ? strtok(NULL, ",") : NULL;
		if (!tok1 || !tok2 || !tok3 || !tok4) continue; // skip malformed lines

		tok1 = trim(tok1); tok2 = trim(tok2); tok3 = trim(tok3); tok4 = trim(tok4);

		char* endptr;
		float x = strtof(tok1, &endptr);
		if (endptr == tok1) continue;
		float y = strtof(tok2, &endptr);
		if (endptr == tok2) continue;
		float z = strtof(tok3, &endptr);
		if (endptr == tok3) continue;
		float r = strtof(tok4, &endptr);
		if (endptr == tok4) continue;

		if (count >= cap) {
			cap *= 2;
			WOWPOS* tmp = (WOWPOS*)realloc(arr, cap * sizeof(WOWPOS));
			if (!tmp) break;
			arr = tmp;
		}
		arr[count].X = x;
		arr[count].Y = y;
		arr[count].Z = z;
		arr[count].Null = r;
		count++;
	}

	fclose(f);
	if (out_count) *out_count = count;
	return arr;
}

BOOL StartLFG(INT nTimeLimit)
{
	Thread *thread = NULL;

	if (nTimeLimit <= 0) return false;

	thread = Thread::FindType(Thread::eType::Bot);
	if (thread)
	{
		thread->stop();
		return false;
	}

	thread = Thread::Create(LFGMoving, (LPVOID)nTimeLimit, Thread::eType::Bot, Thread::ePriority::High);
	thread->setTask(Thread::eTask::LFG);
	return true;
}


/*WOWPOS* SortWaypointsByDistance(WOWPOS* waypoints, size_t size)
{
	WOWPOS *sorted = (WOWPOS *)malloc(sizeof(WOWPOS) * size);

	size_t sublistIndex = 0;
	size_t closestIndex = 0;

	while (sublistIndex < size)
	{
		for (int x = sublistIndex; x < size; x++)
		{
			if (GetDistance3D(waypoints[x], LocalPlayer.pos()) < GetDistance3D(waypoints[closestIndex], LocalPlayer.pos()))
				closestIndex = x;
		}
	}

	sublistIndex++;
	return sorted;
}*/

UINT GetNextClosestWaypoint(WOWPOS *waypoints, size_t listSize)
{
	bool consider = true;
	INT closest = -1;
	if (waypoints == NULL) {
		return closest;
	}

	for (int x = listSize-1; x >= 0; x--)
	{
		if (waypoints[x].X == 0 && waypoints[x].Y == 0 && waypoints[x].Z == 0) {
			continue;
		}
		if (waypoints[x].X == 1 && waypoints[x].Y == 1 && waypoints[x].Z == 1) {
			continue;
		}
		if (closest == -1) 
		{
			closest = x;
		}
		else if (GetDistance3D(waypoints[x], LocalPlayer.pos()) < GetDistance3D(waypoints[closest], LocalPlayer.pos()))
		{
			closest = x;
		}
	}
	return closest;
}


BOOL LFGIsLocalPlayerBeingAttacked()
{
	Object CurrentObject, NextObject;

	if (!(ReadProcessMemory(WoW::handle, (LPVOID)(g_dwCurrentManager + Offsets::FirstEntry), &CurrentObject.BaseAddress, sizeof(CurrentObject.BaseAddress), NULL)))
	{
		memset(&CurrentObject, 0, sizeof(CurrentObject));
		return true;
	}

	CurrentObject = DGetObjectEx(CurrentObject.BaseAddress, BaseObjectInfo);
	while (ValidObject(CurrentObject))
	{
		if (CurrentObject.Type_ID == OT_UNIT)
		{
			CurrentObject = DGetObjectEx(CurrentObject.BaseAddress, BaseObjectInfo | UnitFieldInfo);
			if (CurrentObject.UnitField.Target == LocalPlayer.guid().low)
			{
				return true;
			}
		}

		NextObject = DGetObjectEx(CurrentObject.Next_Ptr, BaseObjectInfo);
		if (NextObject.BaseAddress == CurrentObject.BaseAddress)
			break;
		else
			CurrentObject = NextObject;
	}

	return false;
}

VOID LFGUpdatePosition()
{
	SendKey(' ', WM_KEYDOWN);
	Sleep(20);

	SendKey(' ', WM_KEYUP);
}

VOID LFGLeaveParty()
{
	SendKey('G', WM_KEYDOWN);
	Sleep(20);
	SendKey('G', WM_KEYUP);
}

BOOL CanLFG()
{
	UINT retry_time = 0;
	BOOL ret;
	for (; retry_time < 3; retry_time++) {
		ret = LocalPlayer.isAlive() && WoW::InGame();
		if (ret) { return ret; }
		else { Sleep(1000); continue; }
	}
	return ret;
}

UINT LFGMoving(UINT nTimeLimit)
{
	/*557:法力
	543:城墙
	545:地窟
	555:迷宫
	558:奥金尼
	540:PS
	556:STK
	554:能源
	546:沼泽
	553:生态
	547:奴隶
	542:鲜血*/
	std::unordered_set<int> mapset = { 557, 543, 545, 555, 558, 540, 556, 554, 546, 553, 547, 542 };
	std::map<int, int> wait_time_map;
	for (int ele : mapset) {
		wait_time_map[ele] = 120;
	}
	wait_time_map[547] = 160;
	wait_time_map[555] = 130;

	int wait_time = 120;

	INT curMapID = -3;

	WOWPOS* waypoints = NULL;
	INT* movetype = NULL;

	CHAR szBuffer[256], name[256];

	DWORD ret = NULL;

	UINT waypointListSize = 0;
	UINT curWaypointIndex = 0; // current waypoint index.
	UINT closeWaypointIndex = 0;

	bool flyMode = false;
	float hackSpeed = 120;

	Thread* currentThread = Thread::GetCurrent();

	time_t startTime;
	time_t endTime;
	time(&startTime);


	LFGState = LFGSTATE_START;

	// Setup code so we can call lua ingame.
	Memory::Endscene.Start();

	sprintf(szBuffer, "LFG set to run for %d minutes, curt :%ll", nTimeLimit, startTime);
	LogAppend(szBuffer);
	LogFile(szBuffer);

	srand((UINT)time(NULL));
	

	//Lua::DoString("DinnerDank.start = GetTime()");

	while (currentThread->running())
	{
		if (!CanLFG()) break;
		AttachWoW();
		//if (LocalPlayer.mapId() == 0) { Attach}
		sprintf_s(szBuffer, "cur map %d, last map %d", LocalPlayer.mapId(), curMapID);
		log(szBuffer);
		if (LocalPlayer.mapId() != curMapID || waypoints == NULL) {
			curMapID = LocalPlayer.mapId();

			if (mapset.find(LocalPlayer.mapId()) != mapset.end()) {
				// Initialize waypoints and pathing.
				char filename[15];
				sprintf_s(filename, "point%d.txt", LocalPlayer.mapId());
				waypoints = read_xyz_list(filename, &waypointListSize);
				curWaypointIndex = GetNextClosestWaypoint(waypoints, waypointListSize);
				LFGState = LFGSTATE_START;
				time(&startTime);
			}
			continue;
		}

		if (waypoints == NULL) {
			Sleep(300);
			continue;
		}
		//sprintf_s(szBuffer, "monitor time :%lld", startTime);
		//log(szBuffer);
		//closeWaypointIndex = GetNextClosestWaypoint(waypoints, waypointListSize); //TODO 1.cal next index, not close one 2.state change

		if (!Hack::Collision::isM2Enabled())
		{
			Hack::Collision::M2(true);
		}

		float hp = float(LocalPlayer.health()) / float(LocalPlayer.maxHealth());
		if (hp < 0.7 && LocalPlayer.inCombat())
		{
			LFGState = LFGSTATE_HEAL;
		}
		float dis = GetDistance3D(LocalPlayer.pos(), waypoints[curWaypointIndex]);
		sprintf(szBuffer, "state: %d , index: %d, hp %f, distance: %f", LFGState, curWaypointIndex + 1, hp, dis);
		log(szBuffer);
		LogFile(szBuffer);

		UINT t;

		switch (LFGState)
		{
		case LFGSTATE_START:

			if (dis > 2)
			{
				DWORD action = GetCTMAction();
				//sprintf_s(szBuffer, "action: %d", action);
				//log(szBuffer);
				LogFile(szBuffer);
				if (action != 4)
				{
					if (waypoints[curWaypointIndex].Null != 1) {
						ClickToMove(waypoints[curWaypointIndex]);
					}
					else if (waypoints[curWaypointIndex].Null == 1) {
						Hack::Movement::Teleport(waypoints[curWaypointIndex]); // ** Need to change teleport
						LFGUpdatePosition();
					}

					sprintf_s(szBuffer, "Moving to waypoint %d at (%0.2f, %0.2f, %0.2f)", curWaypointIndex, waypoints[curWaypointIndex].X, waypoints[curWaypointIndex].Y, waypoints[curWaypointIndex].Z);
					log(szBuffer);
					LogFile(szBuffer);

					ret = WaitOnWoW();
					if (ret == WOW_UNRESPONSIVE || ret == WOW_CRASHED)
					{
						log("WoW is unresponsive or it has crashed. Stopping");
						Thread::GetCurrent()->stop();
					}
				}
			}
			else
			{
				if (waypoints[curWaypointIndex].Null > 2) {
					UINT t = UINT(waypoints[curWaypointIndex].Null) * 1000;
					Sleep(t);
				}
				if (curWaypointIndex == waypointListSize - 1)
				{
					LFGState = LFGSTATE_END;
				}
				else {
					curWaypointIndex += 1;
				}
			}

			break;

		case LFGSTATE_HEAL:
			Sleep(1000);
			LFGState = LFGSTATE_START;
			break;

		case LFGSTATE_WAIT:
			t = UINT(waypoints[curWaypointIndex].Null) * 1000;
			Sleep(t);
			LFGState = LFGSTATE_START;
			break;

		case LFGSTATE_END:
			time(&endTime);
			Sleep(1000);
			sprintf_s(szBuffer, "time diff: %lld", endTime - startTime);
			log(szBuffer);
			auto wait = wait_time_map.find(LocalPlayer.mapId());
			if (wait != wait_time_map.end()) {
				wait_time = wait->second;
			}
			if (endTime - startTime > wait_time) { //cool down ready
				//Lua::DoString("LeaveParty()");
				LFGLeaveParty();
				Sleep(3000);
				time(&startTime);
				LFGState = LFGSTATE_START;
				if (waypoints != NULL) { free(waypoints); waypoints = NULL; }
				AttachWoW();
			}
			else if (endTime - startTime > wait_time - 20) {
				TurnToTarget();
			}
			Sleep(1000);
			break;
		}

		Sleep(150);

	}

	SetDlgItemText(g_hwMainWindow, IDC_MAIN_TOOLS_BUTTON_HACK_WATCH, "Start LFG");
	//sprintf(szBuffer, "Stopped after collecting %d nodes");
	//LogAppend(szBuffer);

	vlog("WoWInfoLoop: Delete thread LFG");
	currentThread->exit();
	//if (g_DFish_Logout)
	//	Logout();

	return true;
}