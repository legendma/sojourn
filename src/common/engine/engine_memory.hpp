#pragma once

namespace Engine
{
    interface IMemoryAllocator
    {
        virtual void * Allocate( size_t size, const wchar_t *user_name ) = 0;
        virtual void Free( void *allocation ) = 0;
    }; typedef std::shared_ptr<IMemoryAllocator> MemoryAllocatorPtr;

    class MemoryStackAllocator : public IMemoryAllocator
    {
    public:
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

    class MemoryPoolAllocator : public IMemoryAllocator
    {
    public:
        MemoryPoolAllocator( byte *pool, size_t pool_size, size_t object_size );

        void * Allocate( size_t size, const wchar_t *user_name = nullptr );
        void Free( void *allocation );

        inline size_t GetCurrentUsed() { return m_allocations.size() * m_object_size; }
        inline size_t GetCapacity() { return m_pool_size; }

    private:
        std::vector<void*> m_allocations;
        std::vector<void*> m_frees;
        size_t m_object_size;
        byte *m_head;
        byte *m_tail;
        size_t m_pool_size;
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
        MemoryAllocatorPtr m_allocator;
        std::vector<std::pair<const wchar_t*, void *>> m_allocations;
        std::list<void*> m_frees;
    };

    template <typename T, size_t chunk_cnt>
    class MemoryChunkAllocator
    {
        typedef struct
        {
            MemoryAllocatorPtr allocator;
        } Chunk;

    public:
        MemoryChunkAllocator( MemoryAllocatorPtr allocator, const wchar_t *user_name = nullptr ) :
            m_allocator( allocator )
        {
            size_t object_size = sizeof( T );
            size_t pool_size = chunk_cnt * object_size;
            auto pool = reinterpret_cast<byte*>( m_allocator->Allocate( pool_size, user_name ) );

            Chunk first_chunk;
            first_chunk.allocator = MemoryAllocatorPtr( new MemoryPoolAllocator( pool, pool_size, object_size ) );
            m_chunks.push_back( first_chunk );
        }

    private:
        MemoryAllocatorPtr m_allocator;
        std::vector<Chunk> m_chunks;
    };
}