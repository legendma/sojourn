#include "pch.hpp"

#include "engine_memory.hpp"
#include "engine_utilities.hpp"

Engine::MemoryStackAllocator::MemoryStackAllocator( byte *pool, size_t pool_size ) :
    m_pool( pool ),
    m_pool_size( pool_size ),
    m_head( pool )
{
}

void * Engine::MemoryStackAllocator::Allocate( size_t size, const wchar_t *user_name )
{
    if( m_head + size > m_pool + m_pool_size )
    {
        auto user = std::wstring( L"UNKNOWN" );
        if( user_name == nullptr )
        {
            user = std::wstring( user_name );
        }

        Engine::Log( Engine::LOG_LEVEL_ERROR, L"MemoryStackAllocator::Allocate was out of memory, request from %s", user.c_str() );

        return nullptr;
    }

    m_stack.push_back( m_head );
    auto new_alloc = m_head;
    m_head += size;

    return new_alloc;
}

void Engine::MemoryStackAllocator::Free( void *allocation )
{
    assert( allocation == m_stack.back() );
    m_head = m_stack.back();
    m_stack.pop_back();
}

Engine::MemoryPoolAllocator::MemoryPoolAllocator( byte *pool, size_t pool_size, size_t object_size ) :
    m_head( pool ),
    m_pool_size( pool_size ),
    m_object_size( object_size )
{
    auto max_num_objects = m_pool_size / object_size;
    m_tail = m_head + max_num_objects * object_size;
    m_allocations.reserve( max_num_objects );
    m_frees.reserve( max_num_objects );
}

void * Engine::MemoryPoolAllocator::Allocate( size_t size, const wchar_t *user_name )
{
    void *address = nullptr;
    assert( size == m_object_size );
    if( !m_frees.empty() )
    {
        address = m_frees.back();
        m_frees.pop_back();
    }
    else
    {
        if( m_head + m_object_size > m_tail )
        {
            auto user = std::wstring( L"UNKNOWN" );
            if( user_name == nullptr )
            {
                user = std::wstring( user_name );
            }

            Engine::Log( Engine::LOG_LEVEL_ERROR, L"MemoryPoolAllocator::Allocate was out of memory, request from %s", user.c_str() );

            return nullptr;
        }

        address = m_head;
        m_head += m_object_size;
    }

    m_allocations.push_back( address );

    return address;
}

void Engine::MemoryPoolAllocator::Free( void *allocation )
{
    void *found = nullptr;
    for( auto it = m_allocations.begin(); it != m_allocations.end(); it++ )
    {
        if( *it == allocation )
        {
            found = allocation;
            m_allocations.erase( it );
            break;
        }
    }

    assert( found );
    m_frees.push_back( allocation );
}

Engine::MemorySystem::MemorySystem( size_t capacity )
{
    m_system_memory = reinterpret_cast<byte*>( std::malloc( capacity ) );
    if( !m_system_memory )
    {
        throw new std::runtime_error( "MemorySystem could not allocate pool from system memory!" );
    }

    m_allocator = MemoryAllocatorPtr( new Engine::MemoryStackAllocator( m_system_memory, capacity ) );
}

Engine::MemorySystem::~MemorySystem()
{
    std::free( m_system_memory );
}

void * Engine::MemorySystem::Allocate( size_t size, const wchar_t *user_name )
{
    auto user = std::wstring( L"UNKNOWN" );
    if( user_name != nullptr )
    {
        user = std::wstring( user_name );
    }

    Engine::Log( Engine::LOG_LEVEL_INFO, L"MemorySystem::Allocate %s allocated %d bytes.", user.c_str(), size );
    auto new_allocation = m_allocator->Allocate( size, user_name );
    m_allocations.push_back( std::make_pair( user.c_str(), new_allocation ) );

    return new_allocation;
}

void Engine::MemorySystem::Free( void *allocation )
{
    if( allocation != m_allocations.back().second )
    {
        m_frees.push_back( allocation );
        return;
    }

    void *pointer_to_free = m_allocations.back().second;
    m_allocations.pop_back();
    do
    {
        m_allocator->Free( pointer_to_free );
        pointer_to_free = nullptr;

        for( auto it = m_frees.begin(); it != m_frees.end(); it++ )
        {
            if( *it == m_allocations.back().second )
            {
                pointer_to_free = m_allocations.back().second;
                m_allocations.pop_back();
                m_frees.erase( it );
                break;
            }
        }

    } while( pointer_to_free );
}