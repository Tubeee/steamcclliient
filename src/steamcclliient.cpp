#include <iostream>
#include <fstream>
#include "stuff.h"
#include "clientcontext.h"

std::string settingsPath = "./settings.json";

int main(int argc, char* argv[])
{
	std::vector<AppId_t> accountApps;
	std::vector<std::string> args;

	AppId_t appIDToUse = 0;
	SteamAction actionToPerform = SteamAction::Invalid;

	std::string selfPath = getSelfPath();
	if (!selfPath.empty())
	{
		settingsPath = selfPath + "settings.json";
	}

	if (argc > 1)
	{
		if (std::string(argv[1]).compare(0, 8, "steam://") == 0)
		{
			std::string steamProtocolLink = std::string(std::string(argv[1]));
			size_t delimOffsetOld = 8;
			size_t delimOffset = steamProtocolLink.find_first_of('/', delimOffsetOld);
			while (delimOffset != std::string::npos)
			{
				args.push_back(steamProtocolLink.substr(delimOffsetOld, delimOffset - delimOffsetOld));
				delimOffsetOld = delimOffset + 1;
				delimOffset = steamProtocolLink.find_first_of('/', delimOffsetOld);
			}
			if (delimOffsetOld < steamProtocolLink.size())
			{
				args.push_back(steamProtocolLink.substr(delimOffsetOld, steamProtocolLink.size() - delimOffsetOld));
			}
		}
		else
		{
			for (int i = 1; i < argc; ++i)
			{
				args.push_back(std::string(argv[i]));
			}
		}
	}

	if (args.size() > 0)
	{
		if (std::string(args[0]).compare("run") == 0 || std::string(args[0]).compare("launch") == 0)
		{
			actionToPerform = SteamAction::Run;
		}
		else if (std::string(args[0]).compare("install") == 0)
		{
			actionToPerform = SteamAction::Install;
		}
		else if (std::string(args[0]).compare("uninstall") == 0)
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
		if (args.size() > 1)
		{
			appIDToUse = std::stoul(args[1]);
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

	SCCSettings sccSettings = loadSettings(settingsPath.c_str());

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
	
	saveSettings(sccSettings, settingsPath.c_str());

	EAppState appState = GClientContext()->ClientAppManager()->GetAppInstallState(appIDToUse);

	bool running = true;

	// looks like steam apps can be uninstalled without logging in to steam account
	// also no need to log in if app is not installed
	if (actionToPerform != SteamAction::Uninstall)
	{
		if (actionToPerform == SteamAction::Run && appState != k_EAppStateFullyInstalled)
		{
			std::cout << "App is not installed" << std::endl;
			return 0;
		}

		if (actionToPerform == SteamAction::Install && appState == k_EAppStateFullyInstalled)
		{
			std::cout << "App is already installed" << std::endl;
			return 0;
		}

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
			return 0;
		}
	}

	int32 overlayEnabled = 0;

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
							std::cout << "Downloading AppID " << appIDToUse << std::endl;
							GClientContext()->ClientAppManager()->InstallApp(appIDToUse, 0, false); // installing to first library folder
						}
						break;
						case SteamAction::Run:
						{
							std::cout << "Launching AppID " << appIDToUse << std::endl;
							
							
							if (GClientContext()->ClientUser()->GetConfigInt(k_ERegistrySubTreeSystem, "EnableGameOverlay", &overlayEnabled))
							{
								if (overlayEnabled)
								{
									std::cout << "Disabling steam overlay" << std::endl;
									GClientContext()->ClientUser()->SetConfigInt(k_ERegistrySubTreeSystem, "EnableGameOverlay", 0);
								}
							}
							
							GClientContext()->ClientRemoteStorage()->LoadLocalFileInfoCache(appIDToUse);
							GClientContext()->ClientAppManager()->LaunchApp(CGameID(appIDToUse), 0, 100, ""); // ( ELaunchSource == 100 == new library details page (?))
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
								if (actionToPerform == SteamAction::Run)
								{
									if (overlayEnabled)
									{
										std::cout << "Restoring steam overlay enabled state" << std::endl;
										GClientContext()->ClientUser()->SetConfigInt(k_ERegistrySubTreeSystem, "EnableGameOverlay", 1);
									}
								}

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