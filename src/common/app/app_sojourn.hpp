namespace Application
{
    template<typename T>
    class Application
    {
    public:
        template< typename ...ARGS >
        int Run( ARGS... args )
        {
            auto app = std::make_unique<T>( std::forward<ARGS>( args )... );
            if( !app->Start() )
                return 0;

            auto ret = app->Run();
            app->Shutdown();

            return ret;
        }      
    };
}