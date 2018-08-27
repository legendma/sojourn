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

Engine::MemorySystem::MemorySystem( size_t capacity )
{
    m_system_memory = reinterpret_cast<byte*>( std::malloc( capacity ) );
    if( !m_system_memory )
    {
        throw new std::runtime_error( "MemorySystem could not allocate pool from system memory!" );
    }

    m_allocator = Engine::MemoryStackAllocator( m_system_memory, capacity );
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
    auto new_allocation = m_allocator.Allocate( size, user_name );
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
        m_allocator.Free( pointer_to_free );
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
