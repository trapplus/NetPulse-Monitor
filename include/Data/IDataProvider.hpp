#pragma once

// base interface for all data providers
// fetch() does the actual work - runs in the data thread, never in main
// stop() signals the provider to abort any blocking operation so the thread can join
class IDataProvider
{
public:
    virtual ~IDataProvider() = default;
    virtual void fetch() = 0;
    virtual void stop()  = 0;
};
