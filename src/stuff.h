#ifndef STUFF_H
#define STUFF_H
#pragma once

#include <string>

struct SCCSettings
{
	std::string user;
};

enum class SteamAction
{
	Invalid,
	Run,
	Install,
	Update,
	Uninstall
};


bool fileExists(const char* path);

void to_json(nlohmann::json& j, const SCCSettings& s);

void from_json(const nlohmann::json& j, SCCSettings& s);

SCCSettings loadSettings(const char* path);

void saveSettings(SCCSettings& sets, const char* path);

void showDownloadProgress(uint64_t bytesToDownload, uint64_t bytesDownloaded);

#endif // STUFF_H
