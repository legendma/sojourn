#pragma once

namespace Engine
{
    class Networking
    {
        friend class NetworkingFactory;
    public:
        ~Networking();

    private:
        WSADATA m_wsa_data;

        Networking();
        void Initialize();
    };

    typedef std::shared_ptr<Networking> NetworkingPtr;

    class NetworkingFactory
    {
    public:
        static NetworkingPtr CreateNetworking();
    };
}

