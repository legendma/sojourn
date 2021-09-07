#pragma once


/* Sparse Set class */
template <typename EntityType>
class SparseSet
{
public:
    void Add( EntityType k )
    {
        DebugAssert( Contains( k ) );
        auto &page = GrowPage( ToPageIndex( k ) );
        page[ ToPageSubindex( k ) ] = EntityType( dense.size() );
        dense.push_back( k );
    }

    virtual void Clear()
    {
        dense.clear();
        sparse.clear();
    }

    bool Contains( EntityType k ) const
    {
        auto page_index = ToPageIndex( k );

        return page_index < sparse.size()
            && sparse[ page_index ] != nullptr
            && sparse[ page_index ][ ToPageSubindex( k ) ] != EntityType::null_value;
    }

    size_t Get( EntityType k ) const
    {
        auto &page = sparse.at( ToPageIndex( k ) );
        return page.at( ToPageSubindex( k ) );
    }

    void Remove( EntityType k )
    {
        DebugAssert( Contains( k ) );
        auto &remove_page = GrowPage( ToPageIndex( k ) );
        auto &remove_dense_index = remove_page[ ToPageSubindex( k ) ];

        auto swap_value = dense.back();
        auto &swap_page = GrowPage( ToPageIndex( swap_value ) );

        dense[ remove_dense_index ] = swap_value;
        swap_page[ ToPageSubindex( swap_value ) ] = remove_dense_index;
        remove_dense_index = EntityType::null_value;
        dense.pop_back();
    }

    size_t Size()
    {
        return dense.size();
    }

private:
    static constexpr auto indices_per_page = SOJOURN_PAGE_SIZE / sizeof( EntityType );
    using Page = std::unique_ptr<EntityType[]>;

    std::vector<EntityType> dense;
    std::vector<Page> sparse;

private:
    size_t ToPageIndex( EntityType k ) const
    {
        return size_t( k ) / indices_per_page;
    }

    size_t ToPageSubindex( EntityType k ) const
    {
        return k & ( indices_per_page - 1 );
    }

    Page & GrowPage( std::size_t page_index )
    {
        if( !( sparse.size() > page_index ) )
        {
            sparse.resize( page_index + 1 );
        }

        if( sparse[ page_index ] == nullptr )
        {
            sparse[ page_index ].reset( new std::array<EntityType, indices_per_page> );
            for( auto &value : sparse[ page_index ] )
            {
                value = EntityType::null_value;
            }
        }

        return sparse[ page_index ];
    }
};