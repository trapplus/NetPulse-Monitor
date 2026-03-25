#pragma once
#include "Data/IDataProvider.hpp"
#include <atomic>
#include <chrono>
#include <mutex>
#include <string>

class ExternalAPIProvider : public IDataProvider
{
public:
    ExternalAPIProvider();
    ~ExternalAPIProvider() override;

    void fetch() override;
    void stop() override;

    std::string getIP() const;
    std::string getProvider() const;
    std::string getCity() const;
    std::string getCountry() const;
    bool isDataFresh() const;

private:
    mutable std::mutex                    m_mutex;
    std::string                           m_ip;
    std::string                           m_provider;
    std::string                           m_city;
    std::string                           m_country;
    bool                                  m_dataFresh { false };
    bool                                  m_hasSuccessfulFetch { false };
    std::chrono::steady_clock::time_point m_lastSuccessfulFetch;
    std::atomic<bool>                     m_stopped { false };
};
