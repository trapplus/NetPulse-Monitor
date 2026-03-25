#include "Data/ExternalAPIProvider.hpp"
#include "App/Config.hpp"
#include "Utils/Logger.hpp"
#include <curl/curl.h>
#include <optional>
#include <string_view>
#include <utility>

namespace {

constexpr long kHttpOk = 200;
constexpr const char* kUserAgent = "NetPulse-Monitor/1.0";

struct ParsedExternalInfo {
    std::string ip;
    std::string provider;
    std::string city;
    std::string country;
};

size_t writeCallback(char* ptr, size_t size, size_t nmemb, void* userdata)
{
    if (!userdata || !ptr) return 0;

    const size_t bytes = size * nmemb;
    auto* buffer = static_cast<std::string*>(userdata);
    buffer->append(ptr, bytes);
    return bytes;
}

std::optional<std::string> extractJsonStringValue(const std::string& body, std::string_view key)
{
    const std::string quotedKey = std::string("\"") + std::string(key) + "\"";
    const std::size_t keyPos = body.find(quotedKey);
    if (keyPos == std::string::npos) return std::nullopt;

    const std::size_t colonPos = body.find(':', keyPos + quotedKey.size());
    if (colonPos == std::string::npos) return std::nullopt;

    const std::size_t firstQuote = body.find('"', colonPos + 1);
    if (firstQuote == std::string::npos) return std::nullopt;

    std::size_t cursor = firstQuote + 1;
    while (cursor < body.size()) {
        const std::size_t quotePos = body.find('"', cursor);
        if (quotePos == std::string::npos) return std::nullopt;
        if (quotePos == firstQuote + 1) return std::string{};

        // escaped quotes are part of value, so we skip and keep scanning
        if (body[quotePos - 1] == '\\') {
            cursor = quotePos + 1;
            continue;
        }

        return body.substr(firstQuote + 1, quotePos - firstQuote - 1);
    }

    return std::nullopt;
}

bool parseExternalInfo(const std::string& body, ParsedExternalInfo& out)
{
    std::optional<std::string> ip = extractJsonStringValue(body, "ip");
    if (!ip || ip->empty()) return false;

    std::optional<std::string> provider = extractJsonStringValue(body, "org");
    if (!provider || provider->empty())
        provider = extractJsonStringValue(body, "isp");

    std::optional<std::string> city = extractJsonStringValue(body, "city");

    std::optional<std::string> country = extractJsonStringValue(body, "country_name");
    if (!country || country->empty())
        country = extractJsonStringValue(body, "country");

    out.ip = std::move(*ip);
    out.provider = provider ? std::move(*provider) : std::string("unknown");
    out.city = city ? std::move(*city) : std::string("unknown");
    out.country = country ? std::move(*country) : std::string("unknown");

    return true;
}

bool performGetRequest(const char* url, std::string& responseBody)
{
    CURL* curl = curl_easy_init();
    if (!curl) return false;

    responseBody.clear();

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, kUserAgent);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);

    const CURLcode result = curl_easy_perform(curl);

    long httpCode = 0;
    if (result == CURLE_OK)
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

    curl_easy_cleanup(curl);
    return (result == CURLE_OK && httpCode == kHttpOk);
}

} // namespace

ExternalAPIProvider::ExternalAPIProvider() = default;
ExternalAPIProvider::~ExternalAPIProvider() = default;

void ExternalAPIProvider::fetch()
{
    if (m_stopped) return;

    {
        std::lock_guard lock(m_mutex);

        // no need to hammer public APIs every tick - cached data is fine until refresh window expires
        if (m_hasSuccessfulFetch) {
            const auto now = std::chrono::steady_clock::now();
            const auto elapsed = std::chrono::duration_cast<std::chrono::duration<float>>(now - m_lastSuccessfulFetch);
            if (elapsed.count() < Config::API_REFRESH_SEC)
                return;
        }
    }

    std::string responseBody;
    ParsedExternalInfo parsed;

    // ipapi is primary because it gives all fields we need in one free request
    bool requested = performGetRequest("https://ipapi.co/json/", responseBody);
    bool parsedOk = requested ? parseExternalInfo(responseBody, parsed) : false;

    if (!requested || !parsedOk) {
        // 2ip is fallback for regions where international API endpoints are flaky or rate-limited
        requested = performGetRequest("https://2ip.io/json/", responseBody);
        parsedOk = requested ? parseExternalInfo(responseBody, parsed) : false;
    }

    if (!requested) {
        Log::warn("ExternalAPIProvider: request failed, keeping cached external IP data");
        return;
    }

    if (!parsedOk) {
        Log::warn("ExternalAPIProvider: failed to parse API response, keeping cached external IP data");
        return;
    }

    {
        std::lock_guard lock(m_mutex);
        m_ip = std::move(parsed.ip);
        m_provider = std::move(parsed.provider);
        m_city = std::move(parsed.city);
        m_country = std::move(parsed.country);
        m_dataFresh = true;
        m_hasSuccessfulFetch = true;
        m_lastSuccessfulFetch = std::chrono::steady_clock::now();
    }
}

void ExternalAPIProvider::stop()
{
    m_stopped = true;
}

std::string ExternalAPIProvider::getIP() const
{
    std::lock_guard lock(m_mutex);
    return m_ip;
}

std::string ExternalAPIProvider::getProvider() const
{
    std::lock_guard lock(m_mutex);
    return m_provider;
}

std::string ExternalAPIProvider::getCity() const
{
    std::lock_guard lock(m_mutex);
    return m_city;
}

std::string ExternalAPIProvider::getCountry() const
{
    std::lock_guard lock(m_mutex);
    return m_country;
}

bool ExternalAPIProvider::isDataFresh() const
{
    std::lock_guard lock(m_mutex);
    return m_dataFresh;
}
