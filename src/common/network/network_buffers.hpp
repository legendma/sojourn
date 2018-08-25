#pragma once

#include "network_platform.hpp"
#include "network_types.hpp"

namespace Engine
{
    class BitStreamBase
    {
    public:
        BitStreamBase( bool is_owned );
        ~BitStreamBase();

        byte * GetBuffer() { return m_buffer; }
        static int BitsRequired( uint64_t value );
        static int BytesRequired( uint64_t value );
        virtual size_t GetSize() = 0;
        void Reset() { m_bit_head = 0; }

    protected:
        byte     *m_buffer;
        uint32_t  m_bit_head;
        uint32_t  m_bit_capacity;
        bool      m_owned;

        void ReallocateBuffer( const size_t size );
        void BindBuffer( byte* buffer, const size_t size );
    };

    class InputBitStream : public BitStreamBase
    {
        friend class BitStreamFactory;
    public:
        ~InputBitStream() {};

        size_t GetRemainingBitCount() { return m_bit_capacity - m_bit_head; }
        size_t GetRemainingByteCount() { return (GetRemainingBitCount() + 7) / 8; }
        byte * GetBufferAtCurrent();
        size_t GetSize() { return (7 + m_bit_capacity) / 8; }
        uint32_t SaveCurrentLocation() { return m_bit_head; }
        void SeekToLocation( uint32_t location ) { m_bit_head = location; }

        void Advance( uint32_t bit_cnt ) { m_bit_head += bit_cnt; }

        void WriteBits( void *out, uint32_t bit_cnt );
        void WriteBits( byte &out, uint32_t bit_cnt );
        template <typename T> void Write( T &data, uint32_t bit_cnt = sizeof( T ) * 8 )
        {
            static_assert(std::is_arithmetic<T>::value
                || std::is_enum<T>::value,
                "Generic Read only supports primitive data types");
            T read;
            WriteBits( &read, bit_cnt );
            data = ByteSwap( read );
        }

        void Write( NetworkKey &out );
        void Write( sockaddr &out );
             
        void Write( uint64_t &out, uint32_t bit_cnt = 64 ) { uint64_t temp = 0; WriteBits( &temp, bit_cnt ); out = ByteSwap( temp ); }
        void Write(  int64_t &out, uint32_t bit_cnt = 64 ) {  int64_t temp = 0; WriteBits( &temp, bit_cnt ); out = ByteSwap( temp ); }
                                                                           
        void Write( uint32_t &out, uint32_t bit_cnt = 32 ) { uint32_t temp = 0; WriteBits( &temp, bit_cnt ); out = ByteSwap( temp ); }
        void Write(      int &out, uint32_t bit_cnt = 32 ) {  int32_t temp = 0; WriteBits( &temp, bit_cnt ); out = ByteSwap( temp ); }
                                                                           
        void Write( uint16_t &out, uint32_t bit_cnt = 16 ) { uint16_t temp = 0; WriteBits( &temp, bit_cnt ); out = ByteSwap( temp ); }
        void Write(  int16_t &out, uint32_t bit_cnt = 16 ) {  int16_t temp = 0; WriteBits( &temp, bit_cnt ); out = ByteSwap( temp ); }
                                                                           
        void Write( bool &out )                            { bool temp = false; WriteBits( &temp, 1 );       out = ByteSwap( temp ); }
        void Write( uint8_t &out, uint32_t bit_cnt = 8 )   { WriteBits( &out, bit_cnt ); }
             
        void Write( float &out )                           { float temp;        WriteBits( &temp, 32 );      out = ByteSwap( temp ); }
        void Write( double &out )                          { double temp;       WriteBits( &temp, 64 );      out = ByteSwap( temp ); }

        void WriteBytes( void* out, size_t byte_cnt )      { WriteBits( out, byte_cnt * 8 ); }

    private:
        InputBitStream( byte *input, const size_t size, bool owned );
    }; typedef std::shared_ptr<InputBitStream> InputBitStreamPtr;

    class OutputBitStream : public BitStreamBase
    {
        friend class BitStreamFactory;
    public:
        ~OutputBitStream() {};

        size_t GetCurrentBitCount() { return m_bit_head; }
        size_t GetCurrentByteCount() { return (GetCurrentBitCount() + 7) / 8; }
        size_t GetSize() { return GetCurrentByteCount(); }
        size_t Collapse();

        void WriteBits( void *out, uint32_t bit_cnt );
        void WriteBits( byte &out, uint32_t bit_cnt );

        template< typename T >
        void Write( T &data, uint32_t bit_cnt = sizeof( T ) * 8 )
        {
            static_assert(std::is_arithmetic<T>::value
                || std::is_enum<T>::value,
                "Generic Write only supports primitive data types");

            T write = ByteSwap( data );
            WriteBits( &write, bit_cnt );
        }

        void Write( NetworkKey &out );
        void Write( NetworkAuthentication &out );
        void Write( sockaddr &out );

        void Write( uint64_t out, uint32_t bit_cnt = 64 ) { auto temp = ByteSwap( out ); WriteBits( &temp, bit_cnt ); }
        void Write(  int64_t out, uint32_t bit_cnt = 64 ) { auto temp = ByteSwap( out ); WriteBits( &temp, bit_cnt ); }

        void Write( uint32_t out, uint32_t bit_cnt = 32 ) { auto temp = ByteSwap( out ); WriteBits( &temp, bit_cnt ); }
        void Write(      int out, uint32_t bit_cnt = 32 ) { auto temp = ByteSwap( out ); WriteBits( &temp, bit_cnt ); }

        void Write( uint16_t out, uint32_t bit_cnt = 16 ) { auto temp = ByteSwap( out ); WriteBits( &temp, bit_cnt ); }
        void Write(  int16_t out, uint32_t bit_cnt = 16 ) { auto temp = ByteSwap( out ); WriteBits( &temp, bit_cnt ); }

        void Write( bool out )                            { auto temp = ByteSwap( out ); WriteBits( &temp, 1 ); }
        void Write( uint8_t out, uint32_t bit_cnt = 8 )   { WriteBits( &out, bit_cnt ); }

        void Write( float out )                           { auto temp = ByteSwap( out ); WriteBits( &temp, 32 ); }
        void Write( double out )                          { auto temp = ByteSwap( out ); WriteBits( &temp, 64 ); }

        void WriteBytes( void* out, size_t byte_cnt )     { WriteBits( out, byte_cnt * 8 ); }

    private:
        OutputBitStream( byte *input, const size_t size, bool owned );
    }; typedef std::shared_ptr<OutputBitStream> OutputBitStreamPtr;

    class MeasureBitStream : public BitStreamBase
    {
        friend class BitStreamFactory;
    public:
        ~MeasureBitStream() {};

        size_t GetCurrentBitCount() { return m_bit_head; }
        size_t GetCurrentByteCount() { return (GetCurrentBitCount() + 7) / 8; }
        size_t GetSize() { return GetCurrentByteCount(); }

        void WriteBits( void *out, uint32_t bit_cnt ) { m_bit_head += bit_cnt; }
        void WriteBits( byte &out, uint32_t bit_cnt ) { m_bit_head += bit_cnt; }

        template< typename T >
        void Write( T &data, uint32_t bit_cnt = sizeof( T ) * 8 )
        {
            static_assert(std::is_arithmetic<T>::value
                || std::is_enum<T>::value,
                "Generic Measure only supports primitive data types");

            WriteBits( &data, bit_cnt );
        }

        void Write( NetworkKey &out )            { m_bit_head += out.size() * 8; }
        void Write( NetworkAuthentication &out ) { m_bit_head += out.size() * 8; }
        void Write( sockaddr &out )              { Write( out.sa_family ); WriteBytes( out.sa_data, sizeof( out.sa_data ) ); }

        void Write( uint64_t out, uint32_t bit_cnt = 64 ) { m_bit_head += bit_cnt; }
        void Write(  int64_t out, uint32_t bit_cnt = 64 ) { m_bit_head += bit_cnt; }

        void Write( uint32_t out, uint32_t bit_cnt = 32 ) { m_bit_head += bit_cnt; }
        void Write(      int out, uint32_t bit_cnt = 32 ) { m_bit_head += bit_cnt; }

        void Write( uint16_t out, uint32_t bit_cnt = 16 ) { m_bit_head += bit_cnt; }
        void Write(  int16_t out, uint32_t bit_cnt = 16 ) { m_bit_head += bit_cnt; }

        void Write( bool out )                            { m_bit_head += 1; }
        void Write( uint8_t out, uint32_t bit_cnt = 8 )   { m_bit_head += bit_cnt; }

        void Write( float out )                           { m_bit_head += 32; }
        void Write( double out )                          { m_bit_head += 64; }

        void WriteBytes( void* out, size_t byte_cnt )     { WriteBits( out, byte_cnt * 8 ); }

    private:
        MeasureBitStream() : BitStreamBase( false ) {};
    }; typedef std::shared_ptr<MeasureBitStream> MeasureBitStreamPtr;

    class BitStreamFactory
    {
    public:
        static OutputBitStreamPtr CreateOutputBitStream( byte *input = nullptr, size_t size = 0, bool owned = true )
        {
            return OutputBitStreamPtr( new OutputBitStream( input, size, owned ) );
        }

        static InputBitStreamPtr CreateInputBitStream( byte *input, const size_t size, bool owned = true )
        {
            return InputBitStreamPtr( new InputBitStream( input, size, owned ) );
        }

        static MeasureBitStreamPtr CreateMeasureBitStream()
        {
            return MeasureBitStreamPtr( new MeasureBitStream() );
        }
    };
}

