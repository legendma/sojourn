#include "pch.hpp"
#include "engine_resources.hpp"

static const std::wstring s_assets_location = Engine::Resources::RegGetString( HKEY_LOCAL_MACHINE, Engine::BASE_REGISTRY_PATH, L"Installation Directory" ).append( L"\\assets\\" );

std::wstring Engine::Resources::RegGetString( HKEY hKey, const std::wstring& subKey, const std::wstring& value )
{
    /* discover if the install location registry key exists (and if the game was installed properly), as well as the key value length */
    DWORD string_size{};
    auto status = ::RegGetValue( hKey,
                               subKey.c_str(),
                               value.c_str(),
                               RRF_RT_REG_SZ,
                               nullptr,
                               nullptr,
                               &string_size );
    if( status != ERROR_SUCCESS )
    {
        throw std::runtime_error( "Installation is invalid" );
    }

    /* now get the key value */
    std::wstring return_value;
    return_value.resize( string_size / sizeof( wchar_t ) );
    status = ::RegGetValue( hKey,
                            subKey.c_str(),
                            value.c_str(),
                            RRF_RT_REG_SZ,
                            nullptr,
                            &return_value[0],
                            &string_size );
    if( status != ERROR_SUCCESS )
    {
        throw std::runtime_error( "Installation is invalid!" );
    }

    /* remove the extra NULL character in the string */
    return_value.resize( string_size / sizeof( wchar_t ) - 1 );

    return( return_value );
}

std::shared_ptr<Engine::ResourceFolder> Engine::Resources::OpenFolder( Engine::ResourceFolderType folder_id )
{
    auto folder = std::shared_ptr<ResourceFolder>( new ResourceFolder() );
    switch( folder_id )
    {
        case RESOURCE_FOLDER_SCRIPTS:
            folder->Open( L"scripts\\" );
            break;

        case RESOURCE_FOLDER_SHADERS:
            folder->Open( L"shaders\\" );
            break;

        default:
            /* unknown folder ID */
            throw std::runtime_error( "Unknown resource folder type!" );
            break;
    }

    return( folder );
}

// ResourceFolder Class
void Engine::ResourceFolder::Open( const std::wstring &location )
{
    auto assets = std::wstring( s_assets_location ).append( location );
    if( !PathExists( assets ) )
    {
        throw std::runtime_error( "Could not find assets folder!" );
    }

    m_path = assets;
}

bool Engine::ResourceFolder::PathExists( const std::wstring &location )
{
    DWORD attributes = GetFileAttributes( location.c_str() );
    if( attributes == INVALID_FILE_ATTRIBUTES
     || ~attributes & FILE_ATTRIBUTE_DIRECTORY )
        return false;  // either path doesn't exist, or a file with the same name exists at that location

    return true;
}

// Function that reads from a binary file asynchronously.
Concurrency::task<std::vector<byte>> Engine::ResourceFolder::ReadDataAsync( const std::wstring &filename )
{
    //using namespace Windows::Storage;
    using namespace Concurrency;

    auto file_and_path = std::make_shared<std::wstring>( m_path )->append( filename );
    return this->GetFileAsync( file_and_path ).then( []( std::shared_ptr<Engine::ResourceFile> file ) -> std::vector<byte>
    {
        file->OpenForRead();
        std::vector<byte> returnBuffer;
        returnBuffer.resize( static_cast<size_t>( file->Length() ) );
        file->ReadBytes( returnBuffer.data(), returnBuffer.size() );
        return returnBuffer;
    } );
}

// Function that reads from a binary file asynchronously.
Concurrency::task<std::shared_ptr<Engine::ResourceFile>> Engine::ResourceFolder::GetFileAsync( const std::wstring &filename_w_path )
{
    //using namespace Windows::Storage;
    using namespace Concurrency;

    return create_task( [filename_w_path]()
    {
        auto file = std::shared_ptr<Engine::ResourceFile>( new Engine::ResourceFile( filename_w_path ) );
        return( file );
    } );
}

Engine::ResourceFile::ResourceFile( const std::wstring &filename_w_path ) : m_filename( filename_w_path )
{
}

void Engine::ResourceFile::OpenForRead()
{
    m_mode = std::ios::in | std::ios::binary;
    m_read.open( m_filename, m_mode );
    if( !m_read.is_open() )
    {
        throw std::runtime_error( "Could not open the asset!" );
    }

}

void Engine::ResourceFile::ReadBytes( byte *output, size_t size )
{
    if( !m_read.is_open() )
    {
        throw std::runtime_error( "Tried to read from an invalid file!" );
    }

    m_read.read( reinterpret_cast<char*>(output), size );
}

std::streamsize Engine::ResourceFile::Length()
{
    std::streamsize size = 0;
    if( m_read.is_open() )
    {
        m_read.ignore( std::numeric_limits<std::streamsize>::max() );
        size = m_read.gcount();
        m_read.clear();
        m_read.seekg( 0, std::ios_base::beg );
    }
        
    return( size );
}