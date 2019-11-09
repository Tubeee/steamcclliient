#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>
#include "stuff.h"
#include "clientcontext.h"

const char* settingsPath = "./settings.json";

AppId_t appIDToUse = 0;
SteamAction actionToPerform = SteamAction::Invalid;

std::vector<AppId_t> accountApps;

int main(int argc, char* argv[])
{
	if (argc > 1)
	{
		if (std::string(argv[1]).compare("run") == 0)
		{
			actionToPerform = SteamAction::Run;
		}
		else if (std::string(argv[1]).compare("install") == 0)
		{
			actionToPerform = SteamAction::Install;
		}
		else if (std::string(argv[1]).compare("uninstall") == 0)
		{
			actionToPerform = SteamAction::Uninstall;
		}
		else
		{
			actionToPerform = SteamAction::Invalid;
		}
	}

	if (actionToPerform == SteamAction::Invalid)
	{
		std::cout << std::endl;
		std::cout << "Usage: scc <command> <appid>" << std::endl;
		std::cout << "  Commands are:" << std::endl;
		std::cout << "    install   - to install app" << std::endl;
		std::cout << "    uninstall - to uninstall app" << std::endl;
		std::cout << "    run       - to start app" << std::endl;
		std::cout << std::endl;
		return 0;
	}

	if (actionToPerform == SteamAction::Install		||
		actionToPerform == SteamAction::Run			||
		actionToPerform == SteamAction::Uninstall
	)
	{
		if (argc > 2)
		{
			appIDToUse = std::stoul(argv[2]);
			if (appIDToUse == 0)
			{
				std::cout << "Wrong AppID" << std::endl;
				return 0;
			}
		}
		else
		{
			std::cout << "No AppID supplied" << std::endl;
			return 0;
		}
	}

	if (!GClientContext()->Init())
	{
		std::cout << "Could not initialize steamclient library!" << std::endl;
		return -1;
	}

	SCCSettings sccSettings = loadSettings(settingsPath);

	std::string username = sccSettings.user;
	std::string password;

	if (username.empty())
	{
		std::cout << "Enter username: ";
		std::cin >> username;
		if (username.empty())
		{
			return 0;
		}

		sccSettings.user = username;
	}
	
	saveSettings(sccSettings, settingsPath);

	EAppState appState = GClientContext()->ClientAppManager()->GetAppInstallState(appIDToUse);

	bool running = true;

	// looks like steam apps can be uninstalled without logging in to steam account
	if (actionToPerform != SteamAction::Uninstall)
	{
		if (!GClientContext()->ClientUser()->BHasCachedCredentials(username.c_str()))
		{
			std::cout << "Enter password: ";
			std::cin >> password;
			if (password.empty())
			{
				return 0;
			}

			GClientContext()->ClientUser()->SetLoginInformation(username.c_str(), password.c_str(), true);
			GClientContext()->ClientUser()->LogOn(GClientContext()->ClientUser()->GetSteamID());
		}
		else
		{
			GClientContext()->ClientUser()->SetAccountNameForCachedCredentialLogin(username.c_str(), false);
			GClientContext()->ClientUser()->LogOn(GClientContext()->ClientUser()->GetSteamID());
		}
	}
	else
	{
		if (appState != k_EAppStateInvalid &&
			appState != k_EAppStateUninstalled
		)
		{
			std::cout << "Uninstalling AppID " << appIDToUse << std::endl;
			GClientContext()->ClientAppManager()->UninstallApp(appIDToUse, false);
		}
		else
		{
			std::cout << "App is not installed" << std::endl;
			running = false;
		}
	}

	while (running)
	{
		CallbackMsg_t msg;
		while (GClientContext()->BGetCallback(&msg))
		{
			switch (msg.m_iCallback)
			{
				case SteamServerConnectFailure_t::k_iCallback:
				{
					SteamServerConnectFailure_t* cb = (SteamServerConnectFailure_t*)msg.m_pubParam;
					switch (cb->m_eResult)
					{
						case k_EResultAccountLogonDenied:
						{
							std::cout << "Steam guard code required: ";
							std::string code;
							std::cin >> code;

							GClientContext()->ClientUser()->Set2ndFactorAuthCode(code.c_str(), false);
							GClientContext()->ClientUser()->LogOn(GClientContext()->ClientUser()->GetSteamID());
						}
						break;
						case k_EResultAccountLoginDeniedNeedTwoFactor:
						{
							std::cout << "Steam 2FA code required: ";
							std::string code;
							std::cin >> code;

							GClientContext()->ClientUser()->SetTwoFactorCode(code.c_str());
							GClientContext()->ClientUser()->LogOn(GClientContext()->ClientUser()->GetSteamID());
						}
						break;
						case k_EResultInvalidPassword:
						{
							std::cout << "Could not log in as " << username << "! Wrong password or invalid session!" << std::endl;
							std::cout << "Re-enter password: ";
							std::cin >> password;
							if (password.empty())
							{
								running = false;
							}

							GClientContext()->ClientUser()->SetLoginInformation(username.c_str(), password.c_str(), true);
							GClientContext()->ClientUser()->LogOn(GClientContext()->ClientUser()->GetSteamID());
						}
						break;
						default:
						{
							std::cout << "Could not connect to steam (EResult: " << cb->m_eResult << ")" << std::endl;
							running = false;
						}
						break;
					}
				}
				break;
				case SteamServersConnected_t::k_iCallback:
				{
					std::cout << "Connected to steam!" << std::endl;

					uint32 numApps = GClientContext()->ClientUser()->GetSubscribedApps(nullptr, 0, true);
					AppId_t* apps = new AppId_t[numApps];
					GClientContext()->ClientUser()->GetSubscribedApps(apps, numApps, true);
					for (uint32 i = 0; i < numApps; ++i)
					{
						accountApps.push_back(apps[i]);
					}
					delete[] apps;

					switch (actionToPerform)
					{
						case SteamAction::Install:
						{
							if (appState != k_EAppStateFullyInstalled)
							{
								std::cout << "Downloading AppID " << appIDToUse << std::endl;
								GClientContext()->ClientAppManager()->InstallApp(appIDToUse, 0, false); // installing to first library folder
							}
							else
							{
								std::cout << "App is already installed" << std::endl;
								running = false;
							}
						}
						break;
						case SteamAction::Run:
						{
							if (appState == k_EAppStateFullyInstalled)
							{
								std::cout << "Launching AppID " << appIDToUse << std::endl;
								GClientContext()->ClientAppManager()->LaunchApp(CGameID(appIDToUse), 0, 100, ""); // ( ELaunchSource == 100 == new library details page (?))
							}
							else
							{
								std::cout << "App is not installed" << std::endl;
								running = false;
							}
						}
						break;
					}
				}
				break;
				case AppEventStateChange_t::k_iCallback:
				{
					// when game is fully installed or closed steam will fire this callback 
					// changing app state from whatever it was before to to FullyInstalled (4)
					AppEventStateChange_t* stateChangeCb = (AppEventStateChange_t*)msg.m_pubParam;
					if (stateChangeCb->m_nAppID == appIDToUse)
					{
						if (actionToPerform == SteamAction::Install)
						{
							if ((stateChangeCb->m_eNewState & k_EAppStateUpdateRunning) &&
								(stateChangeCb->m_eNewState & k_EAppUpdateStateValidating)
							)
							{
								showDownloadProgress(100, 100);
								std::cout << std::endl << "Validating..." << std::endl;
							}
						}

						if (stateChangeCb->m_eOldState != stateChangeCb->m_eNewState)
						{
							if ( stateChangeCb->m_eNewState == k_EAppStateFullyInstalled || 
								 stateChangeCb->m_eNewState == k_EAppStateUninstalled 
							)
							{
								std::cout << std::endl << "Done" << std::endl;
								running = false;
							}
						}
					}
				}
				break;
				case AppUpdateProgress_t::k_iCallback:
				{
					AppUpdateProgress_t* updateProgress = (AppUpdateProgress_t*)msg.m_pubParam;
					
					if (updateProgress->m_nAppID == appIDToUse)
					{
						AppUpdateInfo_s updateInfo;
						GClientContext()->ClientAppManager()->GetUpdateInfo(appIDToUse, &updateInfo);
						showDownloadProgress(updateInfo.m_unBytesToDownload, updateInfo.m_unBytesDownloaded);
					}
				}
				break;
			}
			GClientContext()->FreeLastCallback();
		}
#ifdef _WIN32
		Sleep(300);
#else
		usleep(100000);
#endif
	}

	return 0;
}