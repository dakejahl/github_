#include <iostream>
#include <string>
#include <functional>
#include <vector>
#include <memory>
#include <map>

#include <curl/curl.h>
#include <json/json.h>

#include <boost/filesystem.hpp>

// struct VersionEntry {
// 	std::string versionString;
// 	std::string versionChanges;
// 	std::string versionUpdateUrl;
// };

using Release_t = std::map<int, std::string>;

Release_t _releaseIndexNameMap {};


static size_t CurlWriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
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

Release_t get_releases(const Json::Value& jsonData)
{
	// For the list of releases, get index, tag name, and asset_url
	for (int index = 0; index < jsonData.size(); ++index) {
		Json::Value release = jsonData[index];
		std::string tag = release["tag_name"].asString();
		_releaseIndexNameMap.insert(make_pair(index, tag));
	}

	return _releaseIndexNameMap;
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

	std::string url = "https://api.github.com/repos/PX4/Firmware/releases";
	curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
	curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "application/vnd.github.v3+json");
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "PRISM_OS");
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

	res = curl_easy_perform(curl);
	curl_easy_cleanup(curl);

	if (res != CURLcode::CURLE_OK) {
		std::cout << "Error while downloading text, curl error code: "
				 << curl_easy_strerror(res) << std::endl;
		return 0;
	}

	if (readBuffer.size() <= 0) {
		std::cout << "Failed to retrieve data from URL" << std::endl;
		return 0;
	}

	/// JsonCpp test
    Json::CharReaderBuilder builder {};
    auto reader = std::unique_ptr<Json::CharReader>(builder.newCharReader());
	Json::Value root;
	std::string errors {};

    const auto is_parsed = reader->parse( readBuffer.c_str(),
                                          readBuffer.c_str() + readBuffer.size(),
                                          &root,
                                          &errors);


	if (!is_parsed) {
		std::cout << "Failed to parse JSON data" << std::endl;
		return 0;
	}

	std::cout << "Successfully parsed JSON data" << std::endl;
	std::cout << "\nJSON data received:" << std::endl;
	std::cout << "\nFound " << root.size() << " releases" << std::endl;

	// Extracts releases from JSON data
	auto releases = get_releases(root);

	const char* releaseDir = "releases/";
	for (auto it : releases) {
		// std::cout << it.first << "  :  " << it.second << std::endl;

		int index = it.first;
		std::string tag = it.second;

		if(boost::filesystem::create_directory(releaseDir + tag)) {
			// Found a new release, add all the assets to it
			std::cout << "Found new release! --> " << tag << std::endl;

		} else {
			std::cout << "Already tracking release " << tag << std::endl;
		}
	}

	return 0;
}
