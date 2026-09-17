#pragma once

#include <modules/economy/economy.h>
#include <core/config.hpp>
#include <algorithm>
#include <atomic>
#include <cctype>
#include <mutex>
#include <shared_mutex>
#include <filesystem>
#include <fstream>
#include <string>
#include <ShlObj.h>

namespace economy {

    class cloud_configs
    {
    public:
        void refresh( )
        {
            if ( m_busy.exchange( true ) )
                return;

            config_list_result result{};
            result.success = true;

            const auto dir = configs_directory( );
            if ( dir.empty( ) )
            {
                result.success = false;
                result.error_message = "Could not open config folder.";
            }
            else
            {
                std::error_code ec{};
                std::filesystem::create_directories( dir, ec );

                for ( const auto& entry : std::filesystem::directory_iterator( dir, ec ) )
                {
                    if ( ec || !entry.is_regular_file( ) || entry.path( ).extension( ) != L".json" )
                        continue;

                    config_entry cfg{};
                    cfg.name = wide_to_utf8( entry.path( ).stem( ).wstring( ) );
                    cfg.share_code = cfg.name;
                    cfg.owned = true;
                    cfg.can_rename = true;
                    result.configs.push_back( cfg );
                }

                std::sort( result.configs.begin( ), result.configs.end( ), []( const auto& a, const auto& b ) { return a.name < b.name; } );
            }

            {
                std::unique_lock lock( m_mtx );
                m_list = std::move( result );
                m_status = m_list.success ? "Configs refreshed." : m_list.error_message;
            }

            m_busy.store( false );
        }

        void save( const std::string& name, const std::string& share_code = {} )
        {
            const auto target = sanitize_name( share_code.empty( ) ? name : share_code );
            if ( target.empty( ) || m_busy.exchange( true ) )
                return;

            bool ok = false;
            const auto dir = configs_directory( );
            if ( !dir.empty( ) )
            {
                std::error_code ec{};
                std::filesystem::create_directories( dir, ec );
                std::ofstream file( dir / ( utf8_to_wide( target ) + L".json" ), std::ios::binary | std::ios::trunc );
                if ( file )
                {
                    file << config::to_json( ).dump( 2 );
                    ok = true;
                }
            }

            {
                std::unique_lock lock( m_mtx );
                m_last_share_code = target;
                m_status = ok ? "Config saved." : "Config save failed.";
            }

            m_busy.store( false );
            refresh( );
        }

        void load( const std::string& share_code )
        {
            const auto target = sanitize_name( share_code );
            if ( target.empty( ) || m_busy.exchange( true ) )
                return;

            bool ok = false;
            const auto dir = configs_directory( );
            if ( !dir.empty( ) )
            {
                std::ifstream file( dir / ( utf8_to_wide( target ) + L".json" ), std::ios::binary );
                if ( file )
                {
                    try
                    {
                        nlohmann::json root{};
                        file >> root;
                        ok = config::from_json( root );
                    }
                    catch ( ... )
                    {
                        ok = false;
                    }
                }
            }

            {
                std::unique_lock lock( m_mtx );
                m_status = ok ? "Config loaded." : "Config load failed.";
            }

            m_busy.store( false );
        }

        void remove( const std::string& share_code )
        {
            const auto target = sanitize_name( share_code );
            if ( target.empty( ) || m_busy.exchange( true ) )
                return;

            bool ok = false;
            const auto dir = configs_directory( );
            if ( !dir.empty( ) )
            {
                std::error_code ec{};
                ok = std::filesystem::remove( dir / ( utf8_to_wide( target ) + L".json" ), ec );
            }

            {
                std::unique_lock lock( m_mtx );
                m_status = ok ? "Config deleted." : "Config delete failed.";
            }

            m_busy.store( false );
            refresh( );
        }

        bool consume_pending_load( ) { return false; }
        bool busy( ) const { return m_busy.load( ); }

        config_list_result list( ) const
        {
            std::shared_lock lock( m_mtx );
            return m_list;
        }

        std::string status( ) const
        {
            std::shared_lock lock( m_mtx );
            return m_status;
        }

        std::string last_share_code( ) const
        {
            std::shared_lock lock( m_mtx );
            return m_last_share_code;
        }

    private:
        [[nodiscard]] static std::filesystem::path configs_directory( )
        {
            wchar_t app_data[ MAX_PATH ]{};
            if ( FAILED( SHGetFolderPathW( nullptr, CSIDL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, app_data ) ) )
                return {};

            return std::wstring( app_data ) + L"\\cs2_internal\\configs";
        }

        [[nodiscard]] static std::string sanitize_name( std::string_view name )
        {
            std::string out{};
            out.reserve( name.size( ) );

            for ( const auto c : name )
            {
                if ( std::isalnum( static_cast<unsigned char>( c ) ) || c == '_' || c == '-' || c == ' ' )
                    out.push_back( static_cast<char>( c ) );
            }

            while ( !out.empty( ) && out.back( ) == ' ' )
                out.pop_back( );

            return out;
        }

        [[nodiscard]] static std::wstring utf8_to_wide( std::string_view value )
        {
            if ( value.empty( ) )
                return {};

            const auto size = MultiByteToWideChar( CP_UTF8, 0, value.data( ), static_cast<int>( value.size( ) ), nullptr, 0 );
            std::wstring out( static_cast<std::size_t>( size ), L'\0' );
            MultiByteToWideChar( CP_UTF8, 0, value.data( ), static_cast<int>( value.size( ) ), out.data( ), size );
            return out;
        }

        [[nodiscard]] static std::string wide_to_utf8( std::wstring_view value )
        {
            if ( value.empty( ) )
                return {};

            const auto size = WideCharToMultiByte( CP_UTF8, 0, value.data( ), static_cast<int>( value.size( ) ), nullptr, 0, nullptr, nullptr );
            std::string out( static_cast<std::size_t>( size ), '\0' );
            WideCharToMultiByte( CP_UTF8, 0, value.data( ), static_cast<int>( value.size( ) ), out.data( ), size, nullptr, nullptr );
            return out;
        }

        mutable std::shared_mutex m_mtx;
        std::atomic<bool> m_busy{ false };
        config_list_result m_list{};
        std::string m_status{ "Ready." };
        std::string m_last_share_code;
    };

    inline cloud_configs g_cloud;

}