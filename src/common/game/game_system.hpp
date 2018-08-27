#pragma once

interface ISystem
{
public:
    virtual void PreUpdate( float delta_time ) = 0;
    virtual void Update( float delta_time ) = 0;
    virtual void PostUpdate( float delta_time ) = 0;
};