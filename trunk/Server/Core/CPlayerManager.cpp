//============== IV: Multiplayer - http://code.iv-multiplayer.com ==============
//
// File: CPlayerManager.cpp
// Project: Server.Core
// Author(s): jenksta
//            Einstein
// License: See LICENSE in root directory
//
//==============================================================================
// TODO: Leave reason defines
// TODO: Leave reason scripting constants
// TODO: Server closed leave define
// TODO: Player banned leave define
// TODO: Player kicked leave define

#include "CPlayerManager.h"
#include "CNetworkManager.h"
#include "Scripting/CScriptingManager.h"
#include "CVehicleManager.h"
#include "CModuleManager.h"
#include "CEvents.h"

extern CNetworkManager * g_pNetworkManager;
extern CScriptingManager * g_pScriptingManager;
extern CVehicleManager * g_pVehicleManager;
extern CModuleManager * g_pModuleManager;
extern CEvents * g_pEvents;

CPlayerManager::CPlayerManager()
{
	for(EntityId x = 0; x < MAX_PLAYERS; x++)
		m_bActive[x] = false;

	g_pScriptingManager->RegisterConstant("STATE_TYPE_DISCONNECT", STATE_TYPE_DISCONNECT);
	g_pScriptingManager->RegisterConstant("STATE_TYPE_CONNECT", STATE_TYPE_CONNECT);
	g_pScriptingManager->RegisterConstant("STATE_TYPE_SPAWN", STATE_TYPE_SPAWN);
	g_pScriptingManager->RegisterConstant("STATE_TYPE_DEATH", STATE_TYPE_DEATH);
	g_pScriptingManager->RegisterConstant("STATE_TYPE_ONFOOT", STATE_TYPE_ONFOOT);
	g_pScriptingManager->RegisterConstant("STATE_TYPE_ENTERVEHICLE", STATE_TYPE_ENTERVEHICLE);
	g_pScriptingManager->RegisterConstant("STATE_TYPE_INVEHICLE", STATE_TYPE_INVEHICLE);
	g_pScriptingManager->RegisterConstant("STATE_TYPE_PASSENGER", STATE_TYPE_PASSENGER);
	g_pScriptingManager->RegisterConstant("STATE_TYPE_EXITVEHICLE", STATE_TYPE_EXITVEHICLE);
}

CPlayerManager::~CPlayerManager()
{
	for(EntityId x = 0; x < MAX_PLAYERS; x++)
	{
		if(DoesExist(x))
			Remove(x, 0);
	}
}

bool CPlayerManager::DoesExist(EntityId playerId)
{
	if(playerId < 0 || playerId >= MAX_PLAYERS)
		return false;

	return m_bActive[playerId];
}

void CPlayerManager::Add(EntityId playerId, String sPlayerName)
{
	if(DoesExist(playerId))
		Remove(playerId, 0);

	m_pPlayers[playerId] = new CPlayer(playerId, sPlayerName);

	if(m_pPlayers[playerId])
	{
		m_bActive[playerId] = true;
		m_pPlayers[playerId]->AddForWorld();
		m_pPlayers[playerId]->SetState(STATE_TYPE_CONNECT);
		CLogFile::Printf("[Connect] %s (%d) has connected to the server.", sPlayerName.Get(), playerId);
	}
}

void CPlayerManager::Add(EntityId playerId, char * sPlayerName)
{
	String playerName = sPlayerName;
	Add(playerId, playerName);
}

bool CPlayerManager::Remove(EntityId playerId, BYTE byteReason)
{
	if(!DoesExist(playerId))
		return false;

	CSquirrelArguments pArguments;
	pArguments.push(playerId);
	pArguments.push(byteReason);
	g_pEvents->Call("playerDisconnect", &pArguments);

	m_pPlayers[playerId]->SetState(STATE_TYPE_DISCONNECT);
	m_pPlayers[playerId]->DeleteForWorld();

	String strReason = "None";

	if(byteReason == 0)
		strReason = "Disconnected";
	else if(byteReason == 1)
		strReason = "Lost Connection";

	CLogFile::Printf("[Part] %s (%d) left the server (%s).", m_pPlayers[playerId]->GetName().Get(), playerId, strReason.Get());

	delete m_pPlayers[playerId];
	m_pPlayers[playerId] = NULL;
	m_bActive[playerId] = false;
	return true;
}

void CPlayerManager::Pulse()
{
	for(EntityId x = 0; x < MAX_PLAYERS; x++)
	{
		if(m_bActive[x])
			m_pPlayers[x]->Process();
	}
}

void CPlayerManager::HandleClientJoin(EntityId playerId)
{
	if(GetPlayerCount() > 1)
	{
		for(EntityId x = 0; x < MAX_PLAYERS; x++)
		{
			if(m_bActive[x] && x != playerId)
			{
				m_pPlayers[x]->AddForPlayer(playerId);
				m_pPlayers[x]->SpawnForPlayer(playerId);
			}
		}
	}
}

bool CPlayerManager::IsNameInUse(String sNick)
{
	return GetPlayerFromName(sNick) != INVALID_ENTITY_ID;
}

bool CPlayerManager::IsNameInUse(char * sNick)
{
	String szNick = sNick;
	return IsNameInUse(szNick);
}

EntityId CPlayerManager::GetPlayerFromName(String sNick)
{
	for(EntityId x = 0; x < MAX_PLAYERS; x++)
	{
		if(m_bActive[x])
		{
			if(!stricmp(m_pPlayers[x]->GetName(), sNick)) 
				return x;
		}
	}

	return INVALID_ENTITY_ID;
}

EntityId CPlayerManager::GetPlayerFromName(char * sNick)
{
	return GetPlayerFromName(sNick);
}

EntityId CPlayerManager::GetPlayerCount()
{
	EntityId playerCount = 0;

	for(EntityId x = 0; x < MAX_PLAYERS; x++)
	{
		if(m_bActive[x])
			playerCount++;
	}

	return playerCount;
}

CPlayer * CPlayerManager::GetAt(EntityId playerId)
{
	if(!DoesExist(playerId))
		return NULL;

	return m_pPlayers[playerId];
}