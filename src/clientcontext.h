#ifndef STEAMCLIENTWRAP_H
#define STEAMCLIENTWRAP_H
#define STEAMWORKS_CLIENT_INTERFACES

#include "Steamworks.h"

#pragma once
class SteamClientContext
{
public:
	SteamClientContext();
	~SteamClientContext();

	SteamClientContext(SteamClientContext const&) = delete;
	SteamClientContext& operator=(SteamClientContext const&) = delete;

	bool Init();

	IClientAppManager* ClientAppManager();
	IClientApps* ClientApps();
	IClientUtils* ClientUtils();
	IClientUser* ClientUser();
	IClientRemoteStorage* ClientRemoteStorage();

	void RunCallbacks();

	bool BGetCallback(CallbackMsg_t* msg);
	void FreeLastCallback();

private:
	IClientEngine* m_pClientEngine;
	IClientAppManager* m_pClientAppManager;
	IClientApps* m_pClientApps;
	IClientUser* m_pClientUser;
	IClientUtils* m_pClientUtils;
	IClientRemoteStorage* m_pClientRemoteStorage;

	HSteamPipe m_hPipe;
	HSteamUser m_hUser;

	bool m_bInitialized;
};

SteamClientContext* GClientContext();

#endif // !STEAMCLIENTWRAP_H