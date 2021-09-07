#include "pch.hpp"

#include "network_buffers.hpp"

Engine::BitStreamBase::BitStreamBase( bool is_owned ) :
    m_buffer( nullptr ),
    m_bit_head( 0 ),
    m_bit_capacity( 0 ),
    m_owned( is_owned )
{
}

Engine::BitStreamBase::~BitStreamBase()
{
    if( m_owned )
    {
        std::free( m_buffer );
    }
}

void Engine::BitStreamBase::ReallocateBuffer( const size_t size )
{
    assert( m_owned );
    if( !m_buffer )
    {
        BindBuffer( static_cast<byte*>(std::malloc( size )), size );
    }
    else
    {
        BindBuffer( static_cast<byte*>(std::realloc( m_buffer, size )), size );
    }
}

void Engine::BitStreamBase::BindBuffer( byte* buffer, const size_t size )
{
    m_buffer = buffer;
    m_bit_capacity = 8 * size;
}

int Engine::BitStreamBase::BitsRequired( uint64_t value )
{
    if( value == 0 )
    {
        return 1;
    }

    int required_bits = 0;
    while( value )
    {
        value >>= 1;
        required_bits++;
    }

    return required_bits;
}

int Engine::BitStreamBase::BytesRequired( uint64_t value )
{
    int required_bits = BitsRequired( value );
    return (required_bits + 7) / 8;
}

Engine::InputBitStream::InputBitStream( byte *input, const size_t size, bool owned ) :
    Engine::BitStreamBase( owned )
{
    if( !owned )
    {
        BindBuffer( input, size );
        return;
    }

    ReallocateBuffer( size );
    std::memcpy( m_buffer, input, size );
}

void Engine::InputBitStream::WriteBits( byte &out, size_t bit_cnt )
{
    size_t byte_offset = m_bit_head / 8;
    size_t bit_offset = m_bit_head % 8;

    out = m_buffer[byte_offset] >> bit_offset;

    auto free_bits_this_byte = 8 - bit_offset;
    if( free_bits_this_byte < bit_cnt )
    {
        /* go to the next byte */
        out |= m_buffer[byte_offset + 1] << free_bits_this_byte;
    }

    out &= ~(0xff << bit_cnt);
    m_bit_head += bit_cnt;
}

void Engine::InputBitStream::Write( NetworkKey & out )
{
    for( size_t i = 0; i < out.size(); i++ )
    {
        Write( out[i] );
    }
}

void Engine::InputBitStream::Write( sockaddr &out )
{
    Write( out.sa_family );
    WriteBytes( &out.sa_data, sizeof( out.sa_data ) );
}

byte * Engine::InputBitStream::GetBufferAtCurrent()
{
    size_t byte_offset = m_bit_head / 8;
    assert( m_bit_head % 8 == 0 );
    return m_buffer + byte_offset;
}

void Engine::InputBitStream::WriteBits( void *out, size_t bit_cnt )
{
    auto destination = reinterpret_cast<byte*>(out);

    /* read the whole bytes */
    while( bit_cnt > 8 )
    {
        WriteBits( *destination, 8 );
        ++destination;
        bit_cnt -= 8;
    }

    /* read any left over bits */
    if( bit_cnt > 0 )
    {
        WriteBits( *destination, bit_cnt );
    }
}

Engine::OutputBitStream::OutputBitStream( byte *input, const size_t size, bool owned ) :
    BitStreamBase( owned )
{
    if( !owned )
    {
        assert( input );
        BindBuffer( input, size );
        return;
    }

    ReallocateBuffer( size );
    if( input )
    {
        std::memcpy( m_buffer, input, size );
    }
}

void Engine::OutputBitStream::WriteBits( byte &out, size_t bit_cnt )
{
    auto next_bit_head = m_bit_head + bit_cnt;

    if( next_bit_head > m_bit_capacity )
    {
        ReallocateBuffer( std::max( m_bit_capacity * 2, next_bit_head ) );
    }

    size_t byte_offset = m_bit_head / 8;
    size_t bit_offset = m_bit_head % 8;

    auto mask = ~(0xff << bit_offset);
    m_buffer[byte_offset] = (m_buffer[byte_offset] & mask) | (out << bit_offset);

    auto free_bits_this_byte = 8 - bit_offset;
    if( free_bits_this_byte < bit_cnt )
    {
        /* go to the next byte */
        m_buffer[byte_offset + 1] = out >> free_bits_this_byte;
    }

    m_bit_head = next_bit_head;
}

void Engine::OutputBitStream::Write( NetworkKey &out )
{
    for( size_t i = 0; i < out.size(); i++ )
    {
        Write( out[i] );
    }
}

void Engine::OutputBitStream::Write( NetworkAuthentication &out )
{
    for( size_t i = 0; i < out.size(); i++ )
    {
        Write( out[i] );
    }
}

void Engine::OutputBitStream::Write( sockaddr &out )
{
    Write( out.sa_family );
    WriteBytes( out.sa_data, sizeof( out.sa_data ) );
}

size_t Engine::OutputBitStream::Collapse()
{
    assert( m_owned );
    size_t size = GetSize();
    ReallocateBuffer( size );
    return(size);
}

void Engine::OutputBitStream::WriteBits( void *out, size_t bit_cnt )
{
    auto source = reinterpret_cast<byte*>(out);

    /* write the whole bytes */
    while( bit_cnt > 8 )
    {
        WriteBits( *source, 8 );
        ++source;
        bit_cnt -= 8;
    }

    /* write any left over bits */
    if( bit_cnt > 0 )
    {
        WriteBits( *source, bit_cnt );
    }
}