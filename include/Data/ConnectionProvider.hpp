#pragma once
#include "Data/IDataProvider.hpp"

class ConnectionProvider : public IDataProvider
{
public:
    ConnectionProvider();
    ~ConnectionProvider() override;

    void fetch() override;
    void stop() override;
};
