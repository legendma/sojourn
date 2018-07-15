#pragma once

namespace Engine
{
    class Resources;
    class ResourceFolder;

    class ResourceFile
    {
        friend class ResourceFolder;
    public:
        ~ResourceFile() {};

    private:
        ResourceFile( const std::wstring &filename_w_path );
        void OpenForRead();
        void ReadBytes( byte * output, size_t size );
        std::streamsize Length();

        std::wstring m_filename;
        std::ios_base::openmode m_mode;
        std::ofstream m_write;
        std::ifstream m_read;
    };

    typedef enum
    {
        RESOURCE_FOLDER_SCRIPTS,
        RESOURCE_FOLDER_TEXTURES,
        RESOURCE_FOLDER_SHADERS
    } ResourceFolderType;

    class ResourceFolder
    {
    friend class Resources;
    public:
        ~ResourceFolder(){};

        Concurrency::task<std::vector<byte>> ReadDataAsync( const std::wstring &filename );


    private:
        ResourceFolder() {};
        void Open( const std::wstring &location );
        static Concurrency::task<std::shared_ptr<Engine::ResourceFile>> GetFileAsync( const std::wstring &filename_w_path );
        static bool PathExists( const std::wstring &location );

        std::wstring m_path;
    };

    class Resources
    {
    public:
        static std::wstring RegGetString( HKEY hKey, const std::wstring &subKey, const std::wstring &value );
        static std::shared_ptr<Engine::ResourceFolder> OpenFolder( Engine::ResourceFolderType folder );

    private:
        Resources()  = delete;
        ~Resources() = delete;

    };
};

