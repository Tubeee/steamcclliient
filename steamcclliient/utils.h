#ifndef UTILS_H
#define UTILS_H

#include "Steamworks.h"

void ShowDownloadProgress(uint64_t bytesToDownload, uint64_t bytesDownloaded);
bool IsSteamRunning();
void GetAppMissingDeps(AppId_t appID, std::vector<AppId_t>* deps);
bool RunInstallScript(AppId_t appID, bool bUninstall);
std::string GetSelfPath();
bool SetSteamProtocolHandler();
std::string GetSteamAutoLoginUser();
bool SetSteamAutoLoginUser(std::string user);

#endif // UTILS_H