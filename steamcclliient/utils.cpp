#include <iostream>
#include <windows.h>
#include <vector>
#include <direct.h>
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

bool RunInstallScript(AppId_t appID, bool bUninstall)
{
	std::cout << "Running app install script..." << std::endl;

	bool res = false;
	char* appLang = new char[128];
	GClientContext()->ClientAppManager()->GetAppConfigValue(appID, "language", appLang, 128);
	res = GClientContext()->ClientUser()->RunInstallScript(appID, appLang, bUninstall);
	delete[] appLang;

	while (GClientContext()->ClientUser()->IsInstallScriptRunning())
	{
		Sleep(300);
	}

	return res;
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

void ChangeCurrentWorkDir(std::string newDir)
{
	if (!newDir.empty())
	{
		_chdir(newDir.c_str());
	}
}

void ShowDownloadProgress(uint64_t bytesDownloaded, uint64_t bytesToDownload)
{
	float donloadProgress = (float)bytesDownloaded / (float)bytesToDownload;
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

int32 PromptLaunchOptions(AppId_t appID, uint32* opts, int32 optsSize)
{
	std::cout << "Chose launch option:" << std::endl;
	for (int i = 0; i < optsSize; ++i)
	{
		// FIXME: Should read config section instead and parse it as binary vdf
		char description[256] = { '\0' };
		char key[32] = { '\0' };
		sprintf(key, "config/launch/%d/description", opts[i]);
		if (GClientContext()->ClientApps()->GetAppData(appID, key, description, sizeof(description)))
		{
			printf("%d. %s\n", i, description);
		}
	}
	int32 launchOpt = 0;
	std::cout << "Your choice: ";
	std::cin >> launchOpt;
	if (!std::cin)
	{
		return -1;
	}
	return launchOpt;
}