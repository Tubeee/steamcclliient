#include <iostream>
#include <fstream>
#include <stdlib.h>

#ifdef _WIN32
	#include <Windows.h>
#endif

#include "stuff.h"


bool fileExists(const char* path)
{
	std::ifstream iFile(path);
	return iFile.good();
}

void to_json(nlohmann::json& j, const SCCSettings& s) {
	j = nlohmann::json{ {"user", s.user} };
}

void from_json(const nlohmann::json& j, SCCSettings& s) {
	j.at("user").get_to(s.user);
}

SCCSettings loadSettings(const char* path)
{
	SCCSettings sets;
	if (fileExists(path))
	{
		std::ifstream jSets(path);
		nlohmann::json j;
		jSets >> j;
		sets = j;
	}
	return sets;
}

void saveSettings(SCCSettings& sets, const char* path)
{
	std::ofstream jSets(path);
	nlohmann::json j = sets;
	jSets << j;
}


void showDownloadProgress(uint64_t bytesToDownload, uint64_t bytesDownloaded)
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

std::string getSelfPath()
{
#ifdef _WIN32
	char* buf = new char[260];
	GetModuleFileName(NULL, buf, 260);
	std::string out(buf);
	size_t lastDel = out.find_last_of('\\');
	delete[] buf;
	return out.substr(0, lastDel+1);
#else
	return "";
#endif // _WIN
}