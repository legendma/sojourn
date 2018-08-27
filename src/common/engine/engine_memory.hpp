#pragma once

namespace Engine
{
    interface IMemoryAllocator
    {
        virtual void * Allocate( size_t size, const wchar_t *user_name ) = 0;
        virtual void Free( void *allocation ) = 0;
    };

    class MemoryStackAllocator : public IMemoryAllocator
    {
    public:
        MemoryStackAllocator() : m_head( nullptr ), m_pool( nullptr ), m_pool_size( 0 ) {};
        MemoryStackAllocator( byte *pool, size_t pool_size );

        void * Allocate( size_t size, const wchar_t *user_name = nullptr );
        void Free( void *allocation );
        
        inline size_t GetCurrentUsed() { return m_head - m_pool; }
        inline size_t GetCapacity() { return m_pool_size; }

    private:
        std::vector<byte*> m_stack;
        size_t m_pool_size;
        byte *m_pool;
        byte *m_head;
    };

    class MemorySystem : public IMemoryAllocator
    {
    public:
        MemorySystem( size_t capacity );
        ~MemorySystem();

        void * Allocate( size_t size, const wchar_t *user_name = nullptr );
        void Free( void *allocation );

    private:
        byte *m_system_memory;
        MemoryStackAllocator m_allocator;
        std::vector<std::pair<const wchar_t*, void *>> m_allocations;
        std::list<void*> m_frees;
    };
}