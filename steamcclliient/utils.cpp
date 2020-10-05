#include <iostream>
#include <windows.h>
#include <vector>
#include <direct.h>
#include <map>
#include <string>
#include "clientcontext.h"
#include "utils.h"

bool IsSteamRunning()
{
    int steamPID = 0;
    DWORD pidSize = sizeof(steamPID);
    LSTATUS err_no = RegGetValueA(HKEY_CURRENT_USER, "SOFTWARE\\Valve\\Steam\\ActiveProcess\\", "pid", RRF_RT_DWORD, NULL, &steamPID, &pidSize);
    if (!err_no && steamPID != 0)
    {
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, steamPID);
        if (hProcess == NULL)
        {
            return false;
        }

        DWORD exitCode = 0;
        if (GetExitCodeProcess(hProcess, &exitCode))
        {
            if (exitCode == STILL_ACTIVE)
            {
                return true;
            }
        }
    }

    return false;
}

void GetAppMissingDeps(AppId_t appID, std::vector<AppId_t>* deps)
{
    uint32 depCount = GClientContext()->ClientAppManager()->GetAppDependencies(appID, NULL, 0);
    if (depCount)
    {
        AppId_t* appDeps = new AppId_t[depCount];
        GClientContext()->ClientAppManager()->GetAppDependencies(appID, appDeps, depCount);
        for (uint32 i = 0; i < depCount; ++i)
        {
            if (GClientContext()->ClientAppManager()->GetAppInstallState(appDeps[i]) != k_EAppStateFullyInstalled)
            {
                GetAppMissingDeps(appDeps[i], deps);
                if (std::find(deps->cbegin(), deps->cend(), appDeps[i]) == deps->cend())
                {
                    deps->push_back(appDeps[i]);
                }
            }
        }
    }
}

int32 GetAppLaunchOption(AppId_t appIDToUse)
{
    uint32 appLaunchOpts[16];
    uint32 numOpts = GClientContext()->ClientApps()->GetAvailableLaunchOptions(appIDToUse, appLaunchOpts, sizeof(appLaunchOpts));
    int32 optToUse = appLaunchOpts[0];
    if (numOpts > 1)
    {
        std::map<int32, std::string> launchOptions;
        for (int i = 0; i < numOpts; ++i)
        {
            // FIXME: Should read config section instead and parse it as binary vdf
            char description[256] = { '\0' };
            char key[32] = { '\0' };
            sprintf(key, "config/launch/%d/description", appLaunchOpts[i]);
            if (GClientContext()->ClientApps()->GetAppData(appIDToUse, key, description, sizeof(description)))
            {
                launchOptions[appLaunchOpts[i]] = std::string(description);
            }
        }

        int32 userChoice = PromptChooseSingleInt("Chose launch option", launchOptions);
        if (userChoice != -1)
        {
            optToUse = userChoice;
        }
    }
    return optToUse;
}

void SelectDLC(AppId_t appID)
{
    int32 dlcCount = GClientContext()->ClientApps()->GetDLCCount(appID);
    std::map<int32, std::string> dlcSelectMap;

    AppId_t dlcId = 0;
    bool bAvail = false;
    char name[128] = { '\0' };
    for (int32 i = 0; i < dlcCount; ++i)
    {
        GClientContext()->ClientApps()->BGetDLCDataByIndex(appID, i, &dlcId, &bAvail, name, sizeof(name));
        if (bAvail)
        {
            GClientContext()->ClientAppManager()->SetDlcEnabled(appID, dlcId, false);
            dlcSelectMap[dlcId] = std::string(name);
        }
    }

    if (dlcSelectMap.size() == 0)
    {
        return;
    }

    dlcSelectMap[0] = "None";

    std::vector<int32> dlcSelected = PromptChooseMultiInt("Select DLC to be installed", dlcSelectMap);
    if (std::find(dlcSelected.cbegin(), dlcSelected.cend(), 0) != dlcSelected.cend())
    {
        return;
    }

    for (auto it = dlcSelected.cbegin(); it != dlcSelected.cend(); ++it)
    {
        GClientContext()->ClientAppManager()->SetDlcEnabled(appID, *it, true);
    }
}

int32 SelectInstallFolder()
{
    int32 installFolderSelected = 0;
    int32 numFolders = GClientContext()->ClientAppManager()->GetNumInstallBaseFolders();
    if (numFolders > 1)
    {
        std::map<int32, std::string> folders;
        char folderName[256] = { '\0' };
        for (int32 i = 0; i < numFolders; ++i)
        {
            GClientContext()->ClientAppManager()->GetInstallBaseFolder(i, folderName, sizeof(folderName));
            folders[i] = std::string(folderName);
        }

        installFolderSelected = PromptChooseSingleInt("Select steam library folder to use", folders);
        if (installFolderSelected == -1)
        {
            installFolderSelected = 0;
        }
    }

    return installFolderSelected;
}

std::string GetSteamAutoLoginUser()
{
    char nameBuf[256];
    DWORD nameSize = sizeof(nameBuf);
    LSTATUS err_no = RegGetValueA(HKEY_CURRENT_USER, "SOFTWARE\\Valve\\Steam\\", "AutoLoginUser", RRF_RT_REG_SZ, NULL, nameBuf, &nameSize);
    if (!err_no)
    {
        return std::string(nameBuf);
    }
    return std::string();
}

bool SetSteamProtocolHandler()
{
    std::string selfPath = GetSelfPath();
    if (selfPath.empty())
    {
        return false;
    }

    HKEY hKey;
    LSTATUS err_no = RegOpenKeyExA(HKEY_CURRENT_USER, "SOFTWARE\\Classes\\steam\\Shell\\Open\\Command", 0, KEY_SET_VALUE, &hKey);
    if (!err_no)
    {
        char openComm[512] = { '\0' };
        sprintf(openComm, "\"\%s\" \"%%1\"", selfPath.c_str());
        err_no = RegSetValueExA(hKey, NULL, NULL, RRF_RT_REG_SZ, LPBYTE(openComm), DWORD(sizeof(openComm)));
        if (!err_no)
        {
            return true;
        }
    }

    return false;
}

bool SetSteamAutoLoginUser(std::string user)
{
    HKEY hKey;
    LSTATUS err_no = RegOpenKeyExA(HKEY_CURRENT_USER, "SOFTWARE\\Valve\\Steam\\", 0, KEY_SET_VALUE, &hKey);
    if (!err_no)
    {
        err_no = RegSetValueExA(hKey, "AutoLoginUser", NULL, RRF_RT_REG_SZ, LPBYTE(user.c_str()), DWORD(user.size()+1));
        if (!err_no)
        {
            return true;
        }
    }

    return false;
}

typedef NTSTATUS (WINAPI *pRtlGetVersion)(PRTL_OSVERSIONINFOW);
EOSType GetOsType()
{
    HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
    if (hNtdll != NULL) 
    {
        pRtlGetVersion rtlGetVersion = (pRtlGetVersion)GetProcAddress(hNtdll, "RtlGetVersion");
        if (rtlGetVersion != NULL)
        {
            OSVERSIONINFOW osVersion;
            rtlGetVersion(&osVersion);

            // just 4 major desktop versions
            if (osVersion.dwMajorVersion == 6 && osVersion.dwMinorVersion == 1)
            {
                return k_EOSTypeWin7;
            }
            else if (osVersion.dwMajorVersion == 6 && osVersion.dwMinorVersion == 2)
            {
                return k_EOSTypeWin8;
            }
            else if (osVersion.dwMajorVersion == 6 && osVersion.dwMinorVersion == 3)
            {
                return k_EOSTypeWin81;
            }
            else if (osVersion.dwMajorVersion == 10 && osVersion.dwMinorVersion == 0)
            {
                return k_EOSTypeWin10;
            }
        }
    }

    // ruturn sane win version as fallback
    return k_EOSTypeWin7;
}

std::string GetSteamInstallPath()
{
    std::string ownPath = GetSelfPath();
    if (!ownPath.empty())
    {
        size_t endPos = ownPath.find_last_of("\\");
        if (endPos != std::string::npos)
        {
            return ownPath.substr(0, endPos + 1);
        }
    }
    return std::string();
}

bool StartSteamService()
{
    // set "SteamPID" for Steam Client Service, without this value set client service won't start!
    HKEY hKey;
    LSTATUS err_no = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Valve\\Steam", 0, KEY_SET_VALUE, &hKey);
    if (!err_no)
    {
        DWORD ownPID = GetCurrentProcessId();
        LSTATUS stat = RegSetValueExA(hKey, "SteamPID", NULL, REG_DWORD, (LPBYTE)&ownPID, sizeof(ownPID));
    }
    else
    {
        std::cout << "Could not set SteamPID registry value!" << std::endl;
        return false;
    }

    // start service
    //
    // TODO: Reimplement service startup routine to check for service status first
    //       Just assume best case scenario for now: service is installed and not running
    SC_HANDLE hscMgr = OpenSCManager(NULL, NULL, 0x80000000);
    if (!hscMgr)
    {
        std::cout << "OpenSCManagerA failed! " << std::endl;
        DWORD error = GetLastError();
        return false;
    }

    SC_HANDLE hscService = OpenService(hscMgr, "Steam Client Service", 0x80000030);
    if (!hscService)
    {
        std::cout << "OpenServiceA failed!" << std::endl;
        return false;
    }

    if (!StartService(hscService, 0, NULL))
    {
        std::cout << "Could not start steam client service!" << std::endl;
        DWORD error = GetLastError();
        return false;
    }

    return true;
}

void ChangeCurrentWorkDir(std::string newDir)
{
    if (!newDir.empty())
    {
        _chdir(newDir.c_str());
    }
}

void ShowProgress(uint64_t current, uint64_t total)
{
    float donloadProgress = (float)current / (float)total;
    int progressWidth = 50;
    int progress = progressWidth * donloadProgress;

    std::cout << "[";
    for (int i = 0; i < progressWidth; ++i)
    {
        if (i < progress)
        {
            std::cout << "=";
        }
        else if (i == progress)
        {
            std::cout << ">";
        }
        else
        {
            std::cout << " ";
        }
    }
    std::cout << "] " << int(donloadProgress * 100.0) << "%\r";
}

std::string GetSelfPath()
{
    char buf[260] = { '\0' };
    GetModuleFileName(NULL, buf, sizeof(buf));
    return std::string(buf);
}

void PrintOpts(std::string title, std::string prompt, std::map<int32, std::string> &variants)
{
    std::cout << title << std::endl;
    for (auto it = variants.cbegin(); it != variants.cend(); ++it)
    {
        printf("  %d. %s\n", (*it).first, (*it).second.c_str());
    }
    std::cout << prompt;
}

std::vector<int32> PromptChooseMultiInt(std::string title, std::map<int32, std::string> &variants)
{
    PrintOpts(title, "Your choice (separated by ,): ", variants);

    std::vector<int> userChoice;
    std::string choice;
    std::cin >> choice;
    if (std::cin)
    {
        size_t delimPos = 0, delimOld = 0;
        while (delimPos != std::string::npos)
        {
            delimPos = choice.find_first_of(',', delimOld);
            int32 singleChoice = std::stoi(choice.substr(delimOld, delimPos - delimOld));
            delimOld = delimPos + 1;

            if (variants.find(singleChoice) != variants.cend())
            {
                userChoice.push_back(singleChoice);
            }
        }
    }
    
    std::cout << std::endl;

    return userChoice;
}

int32 PromptChooseSingleInt(std::string title, std::map<int32, std::string> &variants)
{
    PrintOpts(title, "Your choice: ", variants);

    int32 userChoice = -1;
    std::cin >> userChoice;
    if (!std::cin || variants.find(userChoice) == variants.cend())
    {
        return -1;
    }

    std::cout << std::endl;

    return userChoice;
}

bool PromptYN(std::string prompt)
{
    std::cout << prompt << " [y/n]: ";
    char choice = 0;
    std::cin >> choice;
    if (!std::cin || (choice != 'Y' && choice != 'y'))
    {
        return false;
    }

    std::cout << std::endl;

    return true;
}