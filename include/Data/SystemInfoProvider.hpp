#pragma once
#include "Data/IDataProvider.hpp"

class SystemInfoProvider : public IDataProvider
{
public:
    SystemInfoProvider();
    ~SystemInfoProvider() override;

    void fetch() override;
    void stop() override;
};
