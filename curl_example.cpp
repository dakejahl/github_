#include <iostream>
#include <string>
#include <functional>
#include <vector>
#include <memory>
#include <map>

#include <curl/curl.h>
#include <json/json.h>

struct VersionEntry {
	std::string versionString;
	std::string versionChanges;
	std::string versionUpdateUrl;
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

	// curl_easy_setopt(curl, CURLOPT_URL, "https://github.com/wattsinnovations/PRISM/releases");
	// curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	// curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
	std::string url = "https://api.github.com/repos/PX4/Firmware/releases";
	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
	curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "application/vnd.github.v3+json");
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "PRISM_OS");
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

	res = curl_easy_perform(curl);
	curl_easy_cleanup(curl);

	if (res != CURLcode::CURLE_OK) {
		std::cout << "Error while downloading text, curl error code: "
				 << curl_easy_strerror(res) << std::endl;
		return 0;
	}

	// std::cout << readBuffer << std::endl;
	if (readBuffer.size() <= 0) {
		std::cout << "Failed to retrieve data from URL" << std::endl;
		return 0;
	}

	// std::cout << readBuffer << std::endl;

	/// JsonCpp test
    Json::CharReaderBuilder builder {};
    auto reader = std::unique_ptr<Json::CharReader>(builder.newCharReader());
	Json::Value root;
	std::string errors {};

    const auto is_parsed = reader->parse( readBuffer.c_str(),
                                          readBuffer.c_str() + readBuffer.size(),
                                          &root,
                                          &errors);

    std::map<int, std::string> releaseIndexNameMap {};

	if (is_parsed) {
		std::cout << "Successfully parsed JSON data" << std::endl;
		std::cout << "\nJSON data received:" << std::endl;
		std::cout << "\nFound " << root.size() << " releases" << std::endl;

		for (int index = 0; index < root.size(); ++index) {  // Iterates over the sequence elements.
			Json::Value release = root[index];
			std::string name =  release["name"].asString();
			releaseIndexNameMap.insert(make_pair(index, name));
		}

		// for (auto it = releaseIndexNameMap.begin(); it != releaseIndexNameMap.end(); ++it)
		for (auto it : releaseIndexNameMap)
			std::cout << it.first << "  :  " << it.second << std::endl;
	}

	return 0;
	//////////////// end HACK

	ChangeLog changelog;
	const auto changelogPattern = std::string("<div class=\"markdown-body\">\n*</div>");
	const auto versionPattern = std::string("/releases/tag/*\">");
	const auto releaseUrlPattern = std::string("<a href=\"*\"");

	auto releases = split(readBuffer, "release-header");

	std::cout << "Found releases: " << releases.size() << std::endl;

	// Skipping the 0 item because anything before the first "release-header" is not a release
	for (int releaseIndex = 1, numItems = releases.size(); releaseIndex < numItems; ++releaseIndex) {
		const std::string& releaseText = releases[releaseIndex];

		int offset = 0;
		std::string updateVersion = match(versionPattern, releaseText, offset, offset);
		std::cout << "Update version: " << updateVersion << std::endl;
		if (updateVersion.at(0) == '.' && updateVersion.at(1) == 'v') {
			updateVersion.erase(0, 2);
		} else if (updateVersion.at(0) == 'v') {
			updateVersion.erase(0, 1);
		}

		const std::string updateChanges = match(changelogPattern, releaseText, offset, offset);

		std::string url;
		offset = 0; // Gotta start scanning from the beginning again, since the release URL could be before the release description
		while (offset != -1)
		{
			const std::string newUrl = match(releaseUrlPattern, releaseText, offset, offset);
			url = newUrl;
		}

		if (url.size() != 0) {
			changelog.push_back({ updateVersion, updateChanges, "https://github.com" + url });
		}
	}

	return 0;
}
