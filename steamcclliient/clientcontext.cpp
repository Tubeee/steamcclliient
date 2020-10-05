#include "clientcontext.h"

SteamClientContext::SteamClientContext() :
    m_pClientApps(nullptr),
    m_pClientAppManager(nullptr),
    m_pClientUtils(nullptr),
    m_pClientUser(nullptr),
    m_pClientEngine(nullptr),
    m_pClientRemoteStorage(nullptr),
    m_hPipe(0),
    m_hUser(0),
    m_bInitialized(false)
{
}

SteamClientContext::~SteamClientContext()
{
    if (m_pClientEngine)
    {
        if (m_hPipe)
        {
            if (m_hUser)
            {
                if (m_pClientUser->BLoggedOn())
                {
                    m_pClientUser->LogOff();
                }
                m_pClientEngine->ReleaseUser(m_hPipe, m_hUser);
            }
            
            // release whatever resources we can...
            Steam_ReleaseThreadLocalMemory(true);
            // drain pipe
            CallbackMsg_t dummy;
            while (Steam_BGetCallback(m_hPipe, &dummy))
            {
                Steam_FreeLastCallback(m_hPipe);
            }			
            
            m_pClientEngine->BReleaseSteamPipe(m_hPipe);
        }
        m_pClientEngine->BShutdownIfAllPipesClosed();
    }
}

bool SteamClientContext::Init()
{
    if (m_bInitialized)
    {
        return true;
    }

    if (!OpenAPI_LoadLibrary())
    {
        return false;
    }

    m_pClientEngine = (IClientEngine*)SteamInternal_CreateInterface(CLIENTENGINE_INTERFACE_VERSION);
    if (!m_pClientEngine)
    {
        return false;
    }

    m_hUser = m_pClientEngine->CreateGlobalUser(&m_hPipe);
    if (!m_hUser || !m_hPipe)
    {
        return false;
    }

    m_pClientUser = (IClientUser*)m_pClientEngine->GetIClientUser(m_hUser, m_hPipe);
    if (!m_pClientUser)
    {
        return false;
    }

    m_pClientApps = (IClientApps*)m_pClientEngine->GetIClientApps(m_hUser, m_hPipe);
    if (!m_pClientApps)
    {
        return false;
    }

    m_pClientAppManager = (IClientAppManager*)m_pClientEngine->GetIClientAppManager(m_hUser, m_hPipe);
    if (!m_pClientAppManager)
    {
        return false;
    }

    m_pClientUtils = (IClientUtils*)m_pClientEngine->GetIClientUtils(m_hPipe);
    if (!m_pClientUtils)
    {
        return false;
    }

    m_pClientRemoteStorage = (IClientRemoteStorage*)m_pClientEngine->GetIClientRemoteStorage(m_hUser, m_hPipe);
    if (!m_pClientRemoteStorage)
    {
        return false;
    }

    m_pClientSortcuts = (IClientShortcuts*)m_pClientEngine->GetIClientShortcuts(m_hUser, m_hPipe);
    if (!m_pClientSortcuts)
    {
        return false;
    }

    m_pClientBilling = (IClientBilling*)m_pClientEngine->GetIClientBilling(m_hUser, m_hPipe);
    if(!m_pClientBilling)
    {
        return false;
    }

    m_bInitialized = true;

    return true;
}

IClientApps* SteamClientContext::ClientApps()
{
    return m_pClientApps;
}

IClientAppManager* SteamClientContext::ClientAppManager()
{
    return m_pClientAppManager;
}

IClientUtils* SteamClientContext::ClientUtils()
{
    return m_pClientUtils;
}

IClientUser* SteamClientContext::ClientUser()
{
    return m_pClientUser;
}

IClientRemoteStorage* SteamClientContext::ClientRemoteStorage()
{
    return m_pClientRemoteStorage;
}

IClientShortcuts* SteamClientContext::ClientSortcuts()
{
    return m_pClientSortcuts;
}

IClientBilling* SteamClientContext::ClientBilling()
{
    return m_pClientBilling;
}

void SteamClientContext::RunCallbacks()
{
    Steam_RunCallbacks(m_hPipe, false);
}

bool SteamClientContext::BGetCallback(CallbackMsg_t* msg)
{
    return Steam_BGetCallback(m_hPipe, msg);
}

void SteamClientContext::FreeLastCallback()
{
    Steam_FreeLastCallback(m_hPipe);
}

SteamClientContext* GClientContext()
{
    static SteamClientContext client;
    return &client;
}