#pragma once
#include "Data/IDataProvider.hpp"

class PacketSnifferProvider : public IDataProvider
{
public:
    PacketSnifferProvider();
    ~PacketSnifferProvider() override;

    void fetch() override;
    void stop() override;
};
