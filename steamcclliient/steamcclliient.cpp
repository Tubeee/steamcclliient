#include <string>
#include "clientcontext.h"
#include "steamcclliient.h"
#include "clientcommands.h"
#include "utils.h"

int main(int argc, char* argv[])
{
	if (IsSteamRunning())
	{
		std::cout << "Another client instance is already running!" << std::endl;
		return 0;
	}

	if(!OpenAPI_LoadLibrary())
	{
		std::cout << "Could not load steamclient library!" << std::endl;
		return 0;
	}

	if (!GClientContext()->Init())
	{
		std::cout << "Could not initialize client context!" << std::endl;
		return 0;
	}

	SetSteamProtocolHandler();

	// required for some games install scripts to work when using 
	// launchers like Playnite
	ChangeCurrentWorkDir(GetSteamInstallPath());

	ClientCommandManager cmdManager;
	std::string loginUser = GetSteamAutoLoginUser();

	std::vector<std::string> args;
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
		if (std::string(args[0]).compare("run") == 0 || std::string(args[0]).compare("launch") == 0 || std::string(args[0]).compare("rungameid") == 0)
		{
			AppId_t appIDToUse = 0;
			if (args.size() > 1)
			{
				appIDToUse = std::stoul(args[1]);
			}
			else
			{
				return 0;
			}

			cmdManager.QueCommand(new ClientLogOnCommand(loginUser, ""));
			cmdManager.QueCommand(new ClientLaunchGameCommand(appIDToUse));
		}
		else if (std::string(args[0]).compare("install") == 0)
		{
			AppId_t appIDToUse = 0;
			if (args.size() > 1)
			{
				appIDToUse = std::stoul(args[1]);
			}
			else
			{
				return 0;
			}

			std::vector<AppId_t> deps;
			GetAppMissingDeps(appIDToUse, &deps);
			if (deps.size())
			{
				for (auto it = deps.cbegin(); it != deps.cend(); ++it)
				{
					cmdManager.QueCommand(new ClientInstallAppCommand(*it));
				}
			}

			cmdManager.QueCommand(new ClientLogOnCommand(loginUser, ""));
			cmdManager.QueCommand(new ClientInstallAppCommand(appIDToUse));
		}
		else if (std::string(args[0]).compare("uninstall") == 0)
		{
			AppId_t appIDToUse = 0;
			if (args.size() > 1)
			{
				appIDToUse = std::stoul(args[1]);
			}
			else
			{
				return 0;
			}

			cmdManager.QueCommand(new ClientUninstallAppCommand(appIDToUse, loginUser));
		}
		else
		{
			std::cout << std::endl;
			std::cout << "Usage: steamcclliient <command> <appid>" << std::endl;
			std::cout << "  Commands are:"                         << std::endl;
			std::cout << "    install   - to install app"          << std::endl;
			std::cout << "    uninstall - to uninstall app"        << std::endl;
			std::cout << "    run       - to start app"            << std::endl;
			std::cout << std::endl;
			return 0;
		}
	}

	bool running = true;

	while (running)
	{ 
		GClientContext()->RunCallbacks();

		cmdManager.RunCommands();
		if (cmdManager.QueSize() == 0)
		{
			running = false;
		}

		Sleep(500);
	}

	std::cout << "Done" << std::endl;
	return 0;
}
