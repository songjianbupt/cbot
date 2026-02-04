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

DWORD LFGState = LFGSTATE_START;

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

UINT LFGMoving(UINT nTimeLimit)
{
	// Northrend herb profile
	WOWPOS waypoints557[] = { //lingmu
		{ -49.06,5.511,-0.954},//0
		{-71.216,-33.418,-0.954},//1
		{-67.787,-61.102,-0.814},//2
		{-85.248,-84.59,-1.942},//3
		{-92.025,-93.586,-1.95},//4
		{-77.199,-112.101,-0.607},//5
		{-120.386,-51.061,3.923},//6
		{-104.47,-52.006,16.787 },//7 teleport
		{-93.568,-32.721,16.787 },//8
		{-107.019,10.795,16.792 },//9
		{-164.148,9.605,16.792  },//10
	};

	WOWPOS waypoints543[] = { //chengqiang
		{-1330.706,1661.333,68.797 },
{-1289.834,1673.449,68.551 },
{-1247.39,1623.399,68.541  },
{-1248.695,1573.672,68.473 },
{-1293.925,1531.175,68.592 },
{-1232.794,1464.285,68.571 },
{-1185.935,1454.923,68.443 },
{-1172.913,1501.208,68.457 },
{-1196.688,1527.912,68.518 },
{-1210.818,1537.78,68.557  },
{-1218.849,1523.139,68.715 },
{-1222.656,1502.996,71.102 },
{-1239.031,1503.911,77.114 },
{-1248.727,1513.926,82.848 },
{-1250.729,1529.445,88.399 },
{-1241.424,1536.684,90.195 },
{-1240.172,1554.895,90.928 },
{-1250.224,1586.675,91.122 },
{-1288.37,1607.478,91.778  },
{-1321.346,1654.032,91.837 },
{-1365.784,1706.843,83.406 },
{-1385.039,1727.142,82.327 },
	};
	WOWPOS waypoints545[] = {//zengqi_diku
		{-4.008,-5.406,-4.487     },
{-3.405,-45.839,-14.808   },
{-5.513,-75.221,-19.923   },
{-11.374,-97.346,-21.628  },
{-44.996,-109.781,-22.081 },
{-68.872,-118.784,-18.481 },
{-76.139,-136.797,-18.414 },
{-48.799,-175.842,-20.343 },
{-19.77,-208.358,-22.13   },
{-36.741,-214.951,-18.235 },
{-90.896,-269.331,-9.563  },
{-95.338,-395.82,-7.824   },
{-94.775,-453.915,7.56    },
{-95.097,-519.567,8.229   },
	};
	WOWPOS waypoints555[] = {//migong
	{-31.676,-4.084,-1.128    },
{-53.34,-34.677,-1.129    },
{-64.171,-114.732,-0.224  },
{-101.165,-142.055,3.447  },
{-154.861,-125.104,6.43   },
{-157.742,-104.85,8.012   },
{-182.368,-46.009,8.072   },
{-229.052,-40.033,8.072   },
{-307.546,-39.155,8.395   },
		{1,1,1},
{-431.55,-40.327,12.687   },
{-430.099,-203.854,12.689 },
{-432.016,-265.725,12.688 },
{-285.905,-264.382,12.681 },
{-240.746,-248.851,17.086 },
{-193.587,-264.315,17.086 },
{-156.609,-265.226,17.085 },
{-155.762,-462.323,17.078 }, };
	WOWPOS waypoints558[] = {//aojinni
	{225.535,3.661,-0.08       },
{268.654,-7.846,-0.058     },
{266.882,28.889,13.442     },
{211.49,30.301,26.644      },
{213.699,-30.903,26.591    },
{236.586,-35.632,26.837    },
{237.829,-146.26,26.591    },
{196.651,-165.134,26.593   },
{-16.653,-163.101,25.742   },
{-142.797,-165.543,26.59   },
{-142.93,-245.072,26.598   },
{-128.575,-287.256,26.35   },
{-144.607,-333.328,26.591  },
{-143.402,-386.102,26.591  },
{-59.696,-396.86,26.588    },
{47.96,-395.44,26.585      }, };
	WOWPOS waypoints540[] = {//posui
	{-35.327,-11.111,-13.561 },
{-18.621,0.347,-13.164   },
{64.582,5.072,-13.217    },
{71.018,43.013,-13.222   },
{71.811,258.312,-13.198  },
{152.606,265.205,-13.208 },
{245.524,268.211,-13.204 },
{269.875,317.492,-2.253  },
{508.718,315.889,1.939   },
{517.624,293.466,1.926   },
{517.392,62.854,1.92     },
{429.281,57.915,2.042    },
{377.316,4.747,1.51      },
{370.884,-80.951,1.912   },
{251.754,-84.241,4.937   }, };
	WOWPOS waypoints556[] = { //STK
	{-35.327,-11.111,-13.561   },
{-18.621,0.347,-13.164     },
{64.582,5.072,-13.217      },
{71.018,43.013,-13.222     },
{71.811,258.312,-13.198    },
{152.606,265.205,-13.208   },
{245.524,268.211,-13.204   },
{269.875,317.492,-2.253    },
{508.718,315.889,1.939     },
{517.624,293.466,1.926    },
{517.392,62.854,1.92      },
{429.281,57.915,2.042     },
{377.316,4.747,1.51       },
{370.884,-80.951,1.912    },
{268.484,-84.022,3.029    },
{30.404,1.44,0.006         },
{41.024,2.254,0.005        },
{42.668,45.652,0.007       },
{41.564,74.408,-0.613      },
{22.281,96.65,0.271        },
{-41.398,101.117,0.007     },
{-69.618,112.638,0.007     },
{-69.991,172.329,0.009     },
{-118.091,172.856,0.01     },
{-206.791,169.334,0.011    },
{-268.756,177.006,0.034    },
{-269.163,141.669,13.565   },
{-213.295,143.604,26.767   },
{-213.734,207.565,26.723   },
{-240.741,205.25,27.009    },
{-246.746,307.381,26.731   },
{-209.926,329.564,26.671   },
{-157.947,288.817,27.607   },
{-101.398,288.351,26.488   },
{-18.398,285.565,26.731    },
{12.336,286.775,26.688     }, };
	WOWPOS waypoints554[] = {//nengyuanjian
	{29.778,29.151,0.005    },
{33.538,60.826,0.163    },
{70.176,61.134,14.034   },
{85.104,73.401,14.925   },
{184.073,76.987,-0.006  },
{220.151,53.25,-0.005   },
{261.806,52.512,-1.609 },
{0,0,0                  },
		{275.231, 52.682, 25.386},//TODO teleport
{291.274,50.564,25.386  },
{277.236,-10.219,25.386 },//10
{1,1,1                  },
{277.11,-20.593,26.327  },
{246.047,-22.785,26.328 },
{1,1,1                  },
{1,1,1                  },
{140.122,15.909,24.875  },
		{141,24.996,24.875},
{1,1,1                  },
{1,1,1                  },
{137.326,124.038,26.41  }, };
	WOWPOS waypoints546[] = {//沼泽
		{68.665,-55.644,-2.752   },
{56.472,-99.789,-2.748   },
{-10.632,-117.722,-4.534 },
{-2.678,-229.438,-4.536  },
{-71.349,-275.502,-3.001 },
{-115.06,-291.388,3.862  },
{-109.425,-256.882,21.54 },
{-96.288,-258.966,23.478 },
{-115.713,-357.651,34.41 },
{-53.669,-393.024,30.908 },
{2.972,-357.466,27.778   },
{19.314,-307.177,31.943  },
{81.103,-281.298,32.182  },
{91.233,-260.621,28.907  },
{100.621,-228.546,24.83  },
{143.776,-175.685,32.282 },
{167.651,-95.263,26.57   },
{191.097,-49.484,26.779  },
{222.629,-21.131,27.563  },
{178.738,11.55,26.792    },
	};

	WOWPOS waypoints553[] = {//生态
	{1.043,11.636,-2.49     },
{-2.298,128.345,-5.54   },
{-2.526,155.315,-5.54   },
{-17.366,174.723,-5.54  },
{-11.112,197.083,-5.54  },
{1.924,231.705,-5.54    },
{-26.419,258.983,-3.746 },
{-43.763,288.529,-1.865 },
{-15.422,288.234,-1.006 },
{76.326,288.762,-5.429  },
{133.007,282.295,-5.417 },
{161.995,281.216,-2.192 },
{155.37,291.224,-4.848  },
{115.194,332.143,-4.144 },
{113.262,339.757,-7.972 },
{89.263,377.883,-28.486 }, };
	WOWPOS waypoints547[] = {//围栏
	{124.978,-106.672,-1.562  },
{50.705,-52.027,-1.515    },
{-53.001,-21.435,-2.217   },
{-108.535,-10.454,-9.587  },
{-112.739,-42.766,-4.059  },
{-103.571,-117.525,-2.811 },
{-115.146,-133.839,-1.893 },
{-112.57,-196.587,-1.506  },
{-95.65,-321.842,-1.59    },
{-79.966,-440.992,-14.677 },
{-88.622,-439.841,-2.208  },
{-91.952,-484.012,-1.319  },
{-77.9,-536.2,-1.592      },
{-92.43,-621.292,15.713   },
{-96.176,-672.584,29.944  },
{-83.207,-740.455,37.171  },
{-118.461,-753.046,37.313 },
{-249.046,-680.274,27.684 }, };

	INT moveType557[] = { 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 };
	INT moveType543[22] = { 0 };
	INT moveType545[14] = { 0 };
	INT moveType555[18] = { 0 };
	INT moveType558[16] = { 0 };
	INT moveType540[15] = { 0 };
	INT moveType556[21] = { 0 };
	INT moveType554[17] = { 0 };
	moveType554[8] = 1;
	INT moveType546[20] = { 0 };
	moveType546[14] = 1;
	INT moveType553[16] = { 0 };
	moveType553[14] = 1;
	INT moveType547[18] = { 0 };
	moveType547[10] = 1;

	std::unordered_set<int> mapset = { 557, 543, 545, 555, 558, 540, 556, 554, 546, 553, 547 };

	INT curMapID = -3;

	WOWPOS *waypoints = NULL;
	INT* movetype = NULL;

	CHAR szBuffer[256], name[256];

	DWORD ret = NULL;

	UINT waypointListSize = 0;
	UINT curWaypointIndex = 0; // current waypoint index.
	UINT closeWaypointIndex = 0;
	
	bool flyMode = false;
	float hackSpeed = 120;

	Thread *currentThread = Thread::GetCurrent();

	time_t curTime;
	time_t endTime;
	time(&curTime);

	
	LFGState = LFGSTATE_START;

	// Setup code so we can call lua ingame.
	Memory::Endscene.Start();

	sprintf(szBuffer, "LFG set to run for %d minutes", nTimeLimit);
	LogAppend(szBuffer);
	LogFile(szBuffer);

	srand((UINT)time(NULL));
	
	Lua::DoString("DinnerDank.start = GetTime()");

	while(currentThread->running())
	{
		if (!LocalPlayer.isAlive() || !WoW::InGame()) break;

		if (LocalPlayer.mapId() != curMapID) {
			curMapID = LocalPlayer.mapId();
			if (LocalPlayer.mapId() == 557)
			{
				waypoints = waypoints557;
				movetype = moveType557;
				waypointListSize = sizeof(waypoints557) / sizeof(WOWPOS);
			}
			else if (LocalPlayer.mapId() == 543) {
				waypoints = waypoints543;
				movetype = moveType543;
				waypointListSize = sizeof(waypoints543) / sizeof(WOWPOS);
			}
			else if (LocalPlayer.mapId() == 545) {
				waypoints = waypoints545;
				movetype = moveType545;
				waypointListSize = sizeof(waypoints545) / sizeof(WOWPOS);
			}
			else if (LocalPlayer.mapId() == 555) {
				waypoints = waypoints555;
				movetype = moveType555;
				waypointListSize = sizeof(waypoints555) / sizeof(WOWPOS);
			}
			else if (LocalPlayer.mapId() == 558) {
				waypoints = waypoints558;
				movetype = moveType558;
				waypointListSize = sizeof(waypoints558) / sizeof(WOWPOS);
			}
			else if (LocalPlayer.mapId() == 540) {
				waypoints = waypoints540;
				movetype = moveType540;
				waypointListSize = sizeof(waypoints540) / sizeof(WOWPOS);
			}
			else if (LocalPlayer.mapId() == 556) {
				waypoints = waypoints556;
				movetype = moveType556;
				waypointListSize = sizeof(waypoints556) / sizeof(WOWPOS);
			}
			else if (LocalPlayer.mapId() == 554) {
				waypoints = waypoints554;
				movetype = moveType554;
				waypointListSize = sizeof(waypoints554) / sizeof(WOWPOS);
			}
			else if (LocalPlayer.mapId() == 546) {
				waypoints = waypoints546;
				movetype = moveType546;
				waypointListSize = sizeof(waypoints546) / sizeof(WOWPOS);
			}
			else if (LocalPlayer.mapId() == 553) {
				waypoints = waypoints553;
				movetype = moveType553;
				waypointListSize = sizeof(waypoints553) / sizeof(WOWPOS);
			}
			else if (LocalPlayer.mapId() == 547) {
				waypoints = waypoints547;
				movetype = moveType547;
				waypointListSize = sizeof(waypoints547) / sizeof(WOWPOS);
			}

			if (mapset.find(LocalPlayer.mapId()) != mapset.end()) {
				// Initialize waypoints and pathing.
				curWaypointIndex = GetNextClosestWaypoint(waypoints, waypointListSize);
				LFGState = LFGSTATE_START;
				time(&curTime);
			}
			
			continue;
		}

		if (waypoints == NULL) {
			Sleep(300);
			continue;
		}
		//closeWaypointIndex = GetNextClosestWaypoint(waypoints, waypointListSize); //TODO 1.cal next index, not close one 2.state change

		if (!Hack::Collision::isM2Enabled())
		{
			Hack::Collision::M2(true);
		}

		float hp = float(LocalPlayer.health()) / float(LocalPlayer.maxHealth());
		if ( hp < 0.7 && LocalPlayer.inCombat())
		{
			LFGState = LFGSTATE_HEAL;
		}

		if (waypoints[curWaypointIndex].X == 0 && waypoints[curWaypointIndex].Y == 0 && waypoints[curWaypointIndex].Z == 0) {
			LFGState = LFGSTATE_IDLE;
		}
		if (waypoints[curWaypointIndex].X == 1 && waypoints[curWaypointIndex].Y == 1 && waypoints[curWaypointIndex].Z == 1) {
			LFGState = LFGSTATE_WAIT;
		}
		sprintf(szBuffer, "state: %d , index: %d, hp %f", LFGState, curWaypointIndex, hp);
		log(szBuffer);
		LogFile(szBuffer);
		float dis = GetDistance3D(LocalPlayer.pos(), waypoints[curWaypointIndex]);
		sprintf_s(szBuffer, "distance: %f", dis);
		log(szBuffer);

		switch(LFGState)
		{
		case LFGSTATE_START:
			
			if (dis > 1)
			{
				DWORD action = GetCTMAction();
				sprintf_s(szBuffer, "action: %d", action);
				log(szBuffer);
				LogFile(szBuffer);
				if (action != 4)
				{
					if (movetype[curWaypointIndex] == 0) {
						ClickToMove(waypoints[curWaypointIndex]);
					}
					else {
						Hack::Movement::Teleport(waypoints[curWaypointIndex]); // ** Need to change teleport
						LFGUpdatePosition();
					}
					//LFGState = LFGSTATE_RUNNING;
					//curWaypointIndex = closeWaypointIndex;
					

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
			Sleep(3000);
			LFGState = LFGSTATE_START;
			if (curWaypointIndex == waypointListSize - 1)
			{
				LFGState = LFGSTATE_END;
			}
			else {
				curWaypointIndex += 1;
			}
			break;

		case LFGSTATE_IDLE:
			LFGUpdatePosition();
			if (abs(waypoints[curWaypointIndex].Z - LocalPlayer.pos().Z) > 2)
			{
				LFGState = LFGSTATE_START;
				if (curWaypointIndex == waypointListSize - 1)
				{
					LFGState = LFGSTATE_END;
				}
				else {
					curWaypointIndex += 1;
				}
			}
			break;

		case LFGSTATE_END:
			time(&endTime);
			std::string& var = Lua::GenerateLocalVariable(10);
			Lua::vDo(
				"local isin = IsInLFGDungeon() "
				"%s = isin == true and 1 or (isin == false and 2 or (isin == nil and 0)) ", var.c_str()
			);
			std::string retstr = Lua::GetText(var);
			sprintf(szBuffer, "drungeon %s ", retstr.c_str());
			log(szBuffer);
			/*if (endTime - curTime > 120) { //cool down ready
				Lua::DoString("RunMacroText(\" / script LeaveParty()\")");
				Sleep(3000);
				Lua::DoString("RunMacroText(\"/invite 七漆骑\")");
				Sleep(3000);
				Lua::DoString("/dungeonfinder");
				Sleep(3000);
				time(&curTime);
				LFGState = LFGSTATE_START;
				waypoints = NULL;
			}*/
			Sleep(1000);
			break;
		}
		
		Sleep(300);

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