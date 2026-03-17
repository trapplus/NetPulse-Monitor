#pragma once

class IDataProvider
{
public:
    virtual ~IDataProvider() = default;
    virtual void fetch() = 0;   // вызывается из Data-потока по таймеру
    virtual void stop() = 0;
};
