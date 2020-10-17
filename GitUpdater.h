#pragma once

#include <iostream>
#include <string>
#include <curl/curl.h>

#include <functional>
#include <vector>


struct VersionEntry {
	std::string versionString;
	std::string versionChanges;
	std::vector<std::string> assetUrls;
};

using VersionList = std::vector<VersionEntry>;

class GitUpdater
{
public:
	GitUpdater(std::string url = "https://github.com/PX4/Firmware/releases");
	~GitUpdater();

	bool check_for_updates();
	VersionList&  fetch_versions_list();


private:


private:
	CURL* _curl;
	std::string _readBuffer;

	std::string _url;

	VersionList _versions;
};