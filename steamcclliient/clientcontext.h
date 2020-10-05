#ifndef CLIENTCONTEXT_H
#define CLIENTCONTEXT_H

#include "Steamworks.h"

#pragma once
class SteamClientContext
{
public:
    SteamClientContext();
    ~SteamClientContext();

    bool Init();

    IClientAppManager* ClientAppManager();
    IClientApps* ClientApps();
    IClientUtils* ClientUtils();
    IClientUser* ClientUser();
    IClientRemoteStorage* ClientRemoteStorage();
    IClientShortcuts* ClientSortcuts();
    IClientBilling* ClientBilling();

    void RunCallbacks();

    bool BGetCallback(CallbackMsg_t* msg);
    void FreeLastCallback();

private:
    IClientEngine* m_pClientEngine;
    IClientAppManager* m_pClientAppManager;
    IClientApps* m_pClientApps;
    IClientShortcuts* m_pClientSortcuts;
    IClientUser* m_pClientUser;
    IClientUtils* m_pClientUtils;
    IClientRemoteStorage* m_pClientRemoteStorage;
    IClientBilling* m_pClientBilling;

    HSteamPipe m_hPipe;
    HSteamUser m_hUser;

    SteamClientContext(SteamClientContext const&) = delete;
    SteamClientContext& operator=(SteamClientContext const&) = delete;

    bool m_bInitialized;
};

SteamClientContext* GClientContext();

#endif // CLIENTCONTEXT_H