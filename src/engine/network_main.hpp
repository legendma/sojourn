#pragma once

namespace Engine
{
    class NetworkMain
    {
    public:
        NetworkMain();
        ~NetworkMain();

    private:
        WSADATA m_wsa_data;
    };

    typedef std::shared_ptr<NetworkMain> NetworkMainPtr;
}

