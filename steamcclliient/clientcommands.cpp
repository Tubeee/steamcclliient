#include "clientcommands.h"
#include "clientcontext.h"
#include "utils.h"

ClientCommandManager::ClientCommandManager()
{
}

ClientCommandManager::~ClientCommandManager()
{
    while (!m_commandQue.empty())
    {
        delete m_commandQue.front();
        m_commandQue.pop();
    }
}

// we own the command object now
void ClientCommandManager::QueCommand(ClientCommandBase* command)
{
    m_commandQue.push(command);
}

void ClientCommandManager::RunCommands()
{
    if (m_commandQue.empty())
    {
        return;
    }

    if (!m_commandQue.front()->BFinished())
    {
        if (!m_commandQue.front()->BStarted())
        {
            m_commandQue.front()->Start();
        }
        else
        {
            m_commandQue.front()->Update();
        }
    }
    else
    {
        delete m_commandQue.front();
        m_commandQue.pop();
    }
}

uint32 ClientCommandManager::QueSize()
{
    return m_commandQue.size();
}

bool ClientCommandBase::BStarted()
{
    return m_started;
}

bool ClientCommandBase::BFinished()
{
    return m_finished;
}

// commands impl

// log on handler
ClientLogOnCommand::ClientLogOnCommand() : 
    m_loggedOnCb(this, &ClientLogOnCommand::OnLoggedOn),
    m_logonFailedCb(this, &ClientLogOnCommand::OnLogOnFailed)
{
}

ClientLogOnCommand::ClientLogOnCommand(std::string user, std::string password) :
    m_loggedOnCb(this, &ClientLogOnCommand::OnLoggedOn),
    m_logonFailedCb(this, &ClientLogOnCommand::OnLogOnFailed),
    m_password(password),
    m_userName(user)
{
}

ClientLogOnCommand::~ClientLogOnCommand()
{
}

void ClientLogOnCommand::Start()
{
    m_started = true;

    if (m_userName.empty())
    {
        std::cout << "Enter username: ";
        std::cin >> m_userName;
    }

    if (GClientContext()->ClientUser()->BHasCachedCredentials(m_userName.c_str()))
    {
        GClientContext()->ClientUser()->SetAccountNameForCachedCredentialLogin(m_userName.c_str(), false);
    }
    else
    {
        if (m_password.empty())
        {
            std::cout << "Enter password: ";
            std::cin >> m_password;
        }
        GClientContext()->ClientUser()->SetLoginInformation(m_userName.c_str(), m_password.c_str(), true);
    }

    GClientContext()->ClientUser()->LogOn(GClientContext()->ClientUser()->GetSteamID());
}

void ClientLogOnCommand::OnLoggedOn(SteamServersConnected_t *cbMsg)
{
    if (!m_started)
    {
        return;
    }

    std::cout << "Logged in" << std::endl;

    if (SetSteamAutoLoginUser(m_userName))
    {
        printf("%s is set as AutoLoginUser\n", m_userName.c_str());
    }

    m_finished = true;
}

void ClientLogOnCommand::OnLogOnFailed(SteamServerConnectFailure_t *cbMsg)
{
    if (!m_started)
    {
        return;
    }

    switch (cbMsg->m_eResult)
    {
        case k_EResultAccountLogonDenied:
            {
                std::cout << "Steam guard code required: ";
                std::string authCode;
                std::cin >> authCode;

                GClientContext()->ClientUser()->Set2ndFactorAuthCode(authCode.c_str(), false);
            }
            break;
        case k_EResultAccountLoginDeniedNeedTwoFactor:
            {
                std::cout << "Steam 2FA code required: ";
                std::string authCode;
                std::cin >> authCode;

                GClientContext()->ClientUser()->SetTwoFactorCode(authCode.c_str());
            }
            break;
        default:
            m_finished = true;
            printf("Could not log in! ( Eresult: %d )\n", cbMsg->m_eResult);
            return;
    }

    GClientContext()->ClientUser()->LogOn(GClientContext()->ClientUser()->GetSteamID());
}

// app install command
ClientInstallAppCommand::ClientInstallAppCommand(AppId_t appID): 
    m_appID(appID),
    m_appUpdateProgressCb(this, &ClientInstallAppCommand::OnAppUpdateProgress),
    m_stateChangedCb(this, &ClientInstallAppCommand::OnAppEventStateChanged),
    m_steamDisconnectedCb(this, &ClientInstallAppCommand::OnDisconnected)
{
}

ClientInstallAppCommand::~ClientInstallAppCommand()
{
}

void ClientInstallAppCommand::Start()
{
    m_started = true;

    if (!GClientContext()->ClientUser()->BLoggedOn())
    {
        m_finished = true;
        std::cout << "Not logged in!" << std::endl;
        return;
    }

    printf("Installing AppID %d\n", m_appID);

    if (GClientContext()->ClientAppManager()->GetAppInstallState(m_appID) == k_EAppStateFullyInstalled)
    {
        m_finished = true;
        std::cout << "App allready installed!" << std::endl;
        return;
    }

    if (!GClientContext()->ClientUser()->BIsSubscribedApp(m_appID))
    {
        SteamAPICall_t hCall = GClientContext()->ClientBilling()->RequestFreeLicenseForApps(&m_appID, 1);
        m_requestFreeLicenseResult.Set(hCall, this, &ClientInstallAppCommand::OnRequestFreeLicenseResult);
        printf("Requested free license for AppID: %d\n", m_appID);
        return;
    }

    GClientContext()->ClientAppManager()->InstallApp(m_appID, 0, false);
}

void ClientInstallAppCommand::OnDisconnected(SteamServersDisconnected_t* cbDisconnected)
{
    if (!m_started)
    {
        return;
    }

    m_finished = true;
    std::cout << "Disconnected from steam!" << std::endl;
}

void ClientInstallAppCommand::OnRequestFreeLicenseResult(RequestFreeLicenseResponse_t* pCallResult, bool IOFail)
{
    if (pCallResult->m_EResult != k_EResultOK)
    {
        m_finished = true;
        std::cout << "Request free license failed!" << std::endl;
        return;
    }

    printf("Installing AppID %d\n", m_appID);
    GClientContext()->ClientAppManager()->InstallApp(m_appID, 0, false);
}

void ClientInstallAppCommand::OnAppUpdateProgress(AppUpdateProgress_t* cbProgress)
{
    if (!m_started)
    {
        return;
    }

    if (cbProgress->m_nAppID == m_appID)
    {
        AppUpdateInfo_s appUpdate;
        GClientContext()->ClientAppManager()->GetUpdateInfo(m_appID, &appUpdate);
        ShowDownloadProgress(appUpdate.m_unBytesDownloaded, appUpdate.m_unBytesToDownload);
    }
}

void ClientInstallAppCommand::OnAppEventStateChanged(AppEventStateChange_t* cbStateChanged)
{
    if (cbStateChanged->m_nAppID != m_appID || !m_started)
    {
        return;
    }

    if (cbStateChanged->m_eAppError != k_EAppUpdateErrorNoError)
    {
        m_finished = true;
        printf("\nApp install error: %d\n", cbStateChanged->m_eAppError);
        return;
    }

    if (cbStateChanged->m_eNewState == k_EAppStateFullyInstalled)
    {
        m_finished = true;
        // just an ugly hack until better progress implemented
        ShowDownloadProgress(100, 100);

        std::cout << std::endl << "App fully installed!" << std::endl;
    }
}

// app uninstall comman
ClientUninstallAppCommand::ClientUninstallAppCommand(AppId_t appID):
    m_appID(appID),
    m_stateChangedCb(this, &ClientUninstallAppCommand::OnAppEventStateChanged)
{
}

ClientUninstallAppCommand::ClientUninstallAppCommand(AppId_t appID, std::string user):
    m_appID(appID),
    m_userName(user),
    m_stateChangedCb(this, &ClientUninstallAppCommand::OnAppEventStateChanged)
{
}

ClientUninstallAppCommand::~ClientUninstallAppCommand()
{
}

void ClientUninstallAppCommand::Start()
{
    m_started = true;

    if (GClientContext()->ClientAppManager()->GetAppInstallState(m_appID) == k_EAppStateUninstalled)
    {
        m_finished = true;
        printf("AppID %d is not installed!\n", m_appID);
        return;
    }

    // while actual log in is not strictly required it would be nice to set account name if possible
    // so client config store could guess our steamid and avoid creating crash dump on assert
    if (!m_userName.empty())
    {
        GClientContext()->ClientUser()->SetAccountNameForCachedCredentialLogin(m_userName.c_str(), false);
    }

    GClientContext()->ClientAppManager()->UninstallApp(m_appID, false);
}

void ClientUninstallAppCommand::OnAppEventStateChanged(AppEventStateChange_t* cbStateChanged)
{
    if (cbStateChanged->m_nAppID != m_appID || !m_started)
    {
        return;
    }

    else if (cbStateChanged->m_eAppError != k_EAppUpdateErrorNoError)
    {
        m_finished = true;
        printf("App uninstall error: %d\n", cbStateChanged->m_eAppError);
        return;
    }

    if (cbStateChanged->m_eNewState == k_EAppStateUninstalled)
    {
        m_finished = true;
        RunInstallScript(m_appID, true);
        std::cout << "App uninstalled!" << std::endl;
    }
}

// app launch command
ClientLaunchGameCommand::ClientLaunchGameCommand(AppId_t appID):
    m_appID(appID),
    m_overlayEnabledOriginal(1),
    m_stateChangedCb(this, &ClientLaunchGameCommand::OnAppEventStateChanged),
    m_launchResultCb(this, &ClientLaunchGameCommand::OnAppLaunchResult)
{
}

ClientLaunchGameCommand::~ClientLaunchGameCommand()
{
}

void ClientLaunchGameCommand::Start()
{
    m_started = true;

    EAppState state = GClientContext()->ClientAppManager()->GetAppInstallState(m_appID);
    if (!(state & k_EAppStateFullyInstalled))
    {
        m_finished = true;
        printf("AppID %d is not fully installed! ( EAppState: %d )\n", m_appID, state);
        return;
    }

    if (state & k_EAppStateUpdateRequired)
    {
        printf("AppID %d update required! Will attempt to start app anyway...\n", m_appID);
    }

    GClientContext()->ClientUser()->GetConfigInt(k_ERegistrySubTreeSystem, "EnableGameOverlay", &m_overlayEnabledOriginal);
    GClientContext()->ClientUser()->SetConfigInt(k_ERegistrySubTreeSystem, "EnableGameOverlay", 0);
    std::cout << "Steam in-game overlay disabled..." << std::endl;

    // should install app deps ... if it's implemented correctly 
    RunInstallScript(m_appID, false);

    // do cloud sync befor launching. This is probably implemented incorrectly here.
    if (GClientContext()->ClientRemoteStorage()->IsCloudEnabledForAccount())
    {
        if (GClientContext()->ClientRemoteStorage()->IsCloudEnabledForApp(m_appID))
        {
            GClientContext()->ClientRemoteStorage()->LoadLocalFileInfoCache(m_appID);
            GClientContext()->ClientRemoteStorage()->RunAutoCloudOnAppLaunch(m_appID);
            GClientContext()->ClientRemoteStorage()->SynchronizeApp(m_appID, true, false);
        }
    }

    GClientContext()->ClientAppManager()->LaunchApp(CGameID(m_appID), 0, 100, "");
}

void ClientLaunchGameCommand::ResotreGameOverlay()
{
    if (GClientContext()->ClientUser()->SetConfigInt(k_ERegistrySubTreeSystem, "EnableGameOverlay", m_overlayEnabledOriginal))
    {
        std::cout << "Steam in-game overlay restored..." << std::endl;
    }
}

void ClientLaunchGameCommand::OnAppEventStateChanged(AppEventStateChange_t* cbStateChanged)
{
    if (cbStateChanged->m_nAppID != m_appID || !m_started)
    {
        return;
    }

    if (cbStateChanged->m_eAppError != k_EAppUpdateErrorNoError)
    {
        ResotreGameOverlay();
        printf("Could not launch AppID %d ( Error: %d )\n", m_appID, cbStateChanged->m_eAppError);
        m_finished = true;
        return;
    }

    if (!(cbStateChanged->m_eNewState & k_EAppStateAppRunning))
    {
        ResotreGameOverlay();

        // do cloud sync on app exit. This is probably implemented incorrectly here.
        if (GClientContext()->ClientRemoteStorage()->IsCloudEnabledForAccount())
        {
            if (GClientContext()->ClientRemoteStorage()->IsCloudEnabledForApp(m_appID))
            {
                GClientContext()->ClientRemoteStorage()->RunAutoCloudOnAppExit(m_appID);
                GClientContext()->ClientRemoteStorage()->SynchronizeApp(m_appID, false, true);
            }
        }

        m_finished = true;
    }
}

void ClientLaunchGameCommand::OnAppLaunchResult(AppLaunchResult_t* cbLaunchResult)
{
    if (!m_started || cbLaunchResult->m_GameID != CGameID(m_appID))
    {
        return;
    }

    if (cbLaunchResult->m_eAppError != k_EAppUpdateErrorNoError)
    {
        printf("Could not start AppId %d ! ( AppError: %d Additional details: \"%s\" )\n", m_appID, cbLaunchResult->m_eAppError, cbLaunchResult->m_szErrorDetail);
    }
}
