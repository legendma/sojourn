#pragma once

namespace Game
{
    typedef enum
    {
        EVENT_TYPE_CNT
    } GameEventType;

    class IGameEvent
    {
    public:
        virtual ~IGameEvent() {};
        virtual GameEventType GetType() = 0;
    };

    template <typename T, GameEventType _EventType>
    class GameEvent : public IGameEvent
    {
    public:
        GameEvent() {};

        virtual GameEventType GetType() { return EVENT_TYPE; }

        static const GameEventType EVENT_TYPE = _EventType;
    };
}