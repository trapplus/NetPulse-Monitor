#pragma once
#include "Data/IDataProvider.hpp"

class NetworkDeviceProvider : public IDataProvider
{
public:
    NetworkDeviceProvider();
    ~NetworkDeviceProvider() override;

    void fetch() override;
    void stop() override;
};
