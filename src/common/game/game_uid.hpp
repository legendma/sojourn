#pragma once

namespace Game
{
    template<typename T, size_t MAX_SIMULTANEOUS>
    union GameUID
    {
        using value_type = T;
        static const int UNIQUE_BITS = sizeof(T) * 8 - MAX_SIMULTANEOUS;

        struct
        {
            T m_index : MAX_SIMULTANEOUS;
            T m_version : UNIQUE_BITS;
        };

        GameUID() {};
        GameUID( T index, T version ) :
            m_index( index ),
            m_version( version ) {}

    private:
        T m_uid;
    };

    template<typename T, typename uid_type, size_t grow_size = 1024>
    class GameUIDTable
    {
    private:
        struct TableEntry
        {
            T *object;
            typename uid_type::value_type version;
        };

        std::vector<TableEntry> m_entries;

    public:
        GameUIDTable()
        {
            Grow();
        }

        uid_type GetNewUID( T *object )
        {
            assert( object );
            size_t i;
            for( i = 0; i < m_entries.size(); i++ )
            {
                if( m_entries[i].object == nullptr )
                {
                    break;
                }
            }

            if( i == m_entries.size() )
            {
                Grow();
            }

            m_entries[i].object = object;
            m_entries[i].version++;

            return uid_type( i, m_entries[i].version );
        }

        T* GetObjectByUID( uid_type uid )
        {
            assert( m_entries[ uid.m_index ].version == uid.m_version );
            return m_entries[ uid.m_index ].object;
        }

        void ReleaseUID( uid_type uid )
        {
            assert( m_entries[ uid.m_index ].version == uid.m_version );

        }

    private:
        void Grow()
        {
            auto old_size = m_entries.size();
            auto new_size = m_entries.capacity() + grow_size;
            m_entries.resize( new_size );
            
            for( auto i = old_size; i < new_size; i++ )
            {
                m_entries[i].object = nullptr;
                m_entries[i].version = 0;
            }
        }

    
    };

}
