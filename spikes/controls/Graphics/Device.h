#pragma once

class Device
{

};

class DeviceFactory
{
    template <typename ...ARGS>
    std::unique_ptr<Device> Create( ARGS... args )
    {
        return std::make_unique<Device>( std::forward<ARGS>( args )... );
    }
};

