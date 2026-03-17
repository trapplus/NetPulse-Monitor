#pragma once
#include "Data/IDataProvider.hpp"

class ExternalAPIProvider : public IDataProvider
{
public:
    ExternalAPIProvider();
    ~ExternalAPIProvider() override;

    void fetch() override;
    void stop() override;
};
