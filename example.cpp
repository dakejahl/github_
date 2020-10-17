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

using ChangeLog = std::vector<VersionEntry>;

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	((std::string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;
}

std::vector<std::string> split(const std::string& str, const std::string& delim)
{
	std::vector<std::string> tokens;
	size_t prev = 0, pos = 0;
	do
	{
		pos = str.find(delim, prev);
		if (pos == std::string::npos) pos = str.length();
		std::string token = str.substr(prev, pos-prev);
		if (!token.empty()) tokens.push_back(token);
		prev = pos + delim.length();
	}
	while (pos < str.length() && prev < str.length());
	return tokens;
}

static std::string match(const std::string& pattern, const std::string& text, int from, int& end)
{
	end = -1;

	auto delimiters = split(pattern, "*");

	if (delimiters.size() != 2)
	{
		// Q_ASSERT_X(delimiters.size() != 2, __FUNCTION__, "Invalid pattern");
		std::cout << "error: delimiters.size() != 2" << std::endl;
		return {};
	}

	const int leftDelimiterStart = text.find(delimiters[0], from);
	if (leftDelimiterStart < 0)
		return {};

	const int rightDelimiterStart = text.find(delimiters[1], leftDelimiterStart + delimiters[0].length());
	if (rightDelimiterStart < 0)
		return {};

	const int resultLength = rightDelimiterStart - leftDelimiterStart - delimiters[0].length();
	if (resultLength <= 0)
		return {};

	end = rightDelimiterStart + delimiters[1].length();
	return text.substr(leftDelimiterStart + delimiters[0].length(), resultLength);
}

int main(void)
{
	CURL *curl;
	CURLcode res;
	std::string readBuffer;

	curl = curl_easy_init();

	if (!curl) {
		std::cout << "Failed to init curl" << std::endl;
		return 0;
	}

	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, true);
	curl_easy_setopt(curl, CURLOPT_URL, "https://github.com/PX4/Firmware/releases");
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
	res = curl_easy_perform(curl);
	curl_easy_cleanup(curl);

	if (res != CURLcode::CURLE_OK) {
		std::cout << "Error while downloading text, curl error code: "
				 << curl_easy_strerror(res) << std::endl;
		return 0;
	}

	if (readBuffer.size() <= 0) {
		std::cout << "Failed to retrieve data from URL: size: " << readBuffer.size() << std::endl;
		return 0;
	}

	ChangeLog changelog;
	const auto changelogPattern = std::string("<div class=\"markdown-body\">\n*</div>");
	const auto versionPattern = std::string("/releases/tag/*\">");
	const auto releaseUrlPattern = std::string("<a href=\"*\"");
	const auto assetCountPattern = std::string("<span title=\"*\"");
	const auto assetPattern = std::string("<a href=\"*\"");

	auto releases = split(readBuffer, "release-header");

	// Skipping the 0 item because anything before the first "release-header" is not a release
	for (int releaseIndex = 1, numItems = releases.size(); releaseIndex < numItems; ++releaseIndex) {

		const std::string& releaseText = releases[releaseIndex];
		int offset = 0;
		std::string updateVersion = match(versionPattern, releaseText, offset, offset);
		const std::string versionChanges = match(changelogPattern, releaseText, offset, offset);

		int assetTextPos = releaseText.find("Assets");
		std::string AssetText = releaseText.substr(assetTextPos, releaseText.size() - assetTextPos);

		int assetOffset = 0;
		int numAssets = std::stoi(match(assetCountPattern, AssetText, assetOffset, assetOffset)
								,nullptr
								,0);

		std::vector<std::string> assets;
		for (int i = 0; i < numAssets; i++) {
			std::string asset = match(assetPattern, AssetText, assetOffset, assetOffset);
			assets.push_back("https://github.com" + asset);
		}

		changelog.push_back({ updateVersion, versionChanges, assets });
	}

	// Print out all our Versions and their Assets
	for (auto& item : changelog) {
		std::cout << "Version: "<< item.versionString << std::endl;
		std::cout << "Assets: " << item.assetUrls.size() << std::endl;

		for (auto& a : item.assetUrls) {
			std::cout << "--> "<< a << std::endl;
		}
		std::cout << " " << std::endl;
	}

	return 0;
}
