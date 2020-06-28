#ifndef CLIENTCOMMANDS_H
#define CLIENTCOMMANDS_H

#include "Steamworks.h"
#include <queue>

class ClientCommandBase
{
public:
    ClientCommandBase() {};
    virtual ~ClientCommandBase() {};

    virtual void Start() {};
    virtual void Update() {};

    bool BStarted();
    bool BFinished();

protected:
    bool m_started = false;
    bool m_finished = false;
};


// tiny command scheduler/runner
class ClientCommandManager
{
public:
    ClientCommandManager();
    ~ClientCommandManager();

    void QueCommand(ClientCommandBase* command);
    void RunCommands();

    uint32 QueSize();

private:
    std::queue<ClientCommandBase*> m_commandQue;
};


// some basic commands

class ClientLogOnCommand : public ClientCommandBase
{
public:
    ClientLogOnCommand();
    ClientLogOnCommand(std::string user, std::string password);
    ~ClientLogOnCommand();

    void Start() override;

private:

    std::string m_userName;
    std::string m_password;

    STEAM_CALLBACK(ClientLogOnCommand, OnLoggedOn, SteamServersConnected_t, m_loggedOnCb);
    STEAM_CALLBACK(ClientLogOnCommand, OnLogOnFailed, SteamServerConnectFailure_t, m_logonFailedCb);
};

class ClientLaunchGameCommand: public ClientCommandBase
{
public:
    ClientLaunchGameCommand(AppId_t appID);
    ~ClientLaunchGameCommand();

    void Start();

private:
    void ResotreGameOverlay();

    AppId_t m_appID;

    STEAM_CALLBACK(ClientLaunchGameCommand, OnAppEventStateChanged, AppEventStateChange_t, m_stateChangedCb);
    STEAM_CALLBACK(ClientLaunchGameCommand, OnAppLaunchResult, AppLaunchResult_t, m_launchResultCb);
};

class ClientInstallAppCommand : public ClientCommandBase
{
public:
    ClientInstallAppCommand(AppId_t appID);
    ~ClientInstallAppCommand();

    void Start();

private:
    AppId_t m_appID;
    std::vector<AppId_t> m_appDeps;

    STEAM_CALLBACK(ClientInstallAppCommand, OnAppEventStateChanged, AppEventStateChange_t, m_stateChangedCb);
    STEAM_CALLBACK(ClientInstallAppCommand, OnAppUpdateProgress, AppUpdateProgress_t, m_appUpdateProgressCb);
    STEAM_CALLBACK(ClientInstallAppCommand, OnDisconnected, SteamServersDisconnected_t, m_steamDisconnectedCb);

    void OnRequestFreeLicenseResult(RequestFreeLicenseResponse_t* pCallResult, bool IOFail);
    CCallResult<ClientInstallAppCommand, RequestFreeLicenseResponse_t> m_requestFreeLicenseResult;
};

class ClientUninstallAppCommand : public ClientCommandBase
{
public: 
    ClientUninstallAppCommand(AppId_t appID);
    ClientUninstallAppCommand(AppId_t appID, std::string user);
    ~ClientUninstallAppCommand();

    void Start();

private:
    AppId_t m_appID;
    std::string m_userName;

    STEAM_CALLBACK(ClientUninstallAppCommand, OnAppEventStateChanged, AppEventStateChange_t, m_stateChangedCb);
};

#endif // CLIENTCOMMANDS_H

