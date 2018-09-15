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

    template <typename T, size_t objects_per_chunk>
    class MemoryChunkAllocator
    {
        typedef struct
        {
            MemoryAllocatorPtr allocator;
            std::list<T*> objects;
        } Chunk;

    public:
        MemoryChunkAllocator( MemoryAllocatorPtr allocator, const wchar_t *user_name = nullptr ) :
            m_allocator( allocator ),
            m_user_name( user_name )
        {
            CreateNewChunk();
        }

        void * Allocate()
        {
            Chunk *chunk = nullptr;
            for( auto &search : m_chunks )
            {
                if( search.objects.size() < objects_per_chunk )
                {
                    chunk = &search;
                    break;
                }
            }

            if( !chunk )
            {
                chunk = CreateNewChunk();
            }

            auto new_object = chunk->allocator->Allocate( sizeof( T ), m_user_name );

            return new_object;
        }

        void Free( void *address )
        {

        }

    private:
        MemoryAllocatorPtr m_allocator;
        std::vector<Chunk> m_chunks;
        const wchar_t *m_user_name;

        Chunk * CreateNewChunk()
        {
            size_t object_size = sizeof( T );
            size_t chunk_size = objects_per_chunk * object_size;
            auto pool = reinterpret_cast<byte*>(m_allocator->Allocate( chunk_size, m_user_name ));

            m_chunks.emplace_back();
            auto *new_chunk = &m_chunks.back();
            new_chunk->allocator = MemoryAllocatorPtr( new MemoryPoolAllocator( pool, chunk_size, object_size ) );

            return new_chunk;
        }
    };
}