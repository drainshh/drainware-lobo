/*
    use the m_ prefix for members of structs and classes.
    state the reason why you have include a file.

    macros UPPERCASE regar.ds.

    to-do example -
        to-do ~ blah blah blah

    note example -
        note ~ blah blah blah

    xref example
        xref @ "module.dll" "bytes"

    declarations
        enum example
            enum e_fart
            {
                fart_one,
                fart_two
                fart_max
            };

        struct example
            struct fart_t
            {
                int m_member{};
            };

            inline fart_t g_fart = {};

        namespace example
            namespace n_fart
            {

            };

        class example
            class c_fart
            {
            public:
                int get_fart()
                {
                    return this->m_fart;
                }
            private:
                int m_fart{};
            };
*/

/* He's a big chungus
He's a big chunky boi
Such a big bun, yes
We are so overjoyed
To have a big chunky boi
A big and wonderful chungus such as he
Oh such as he
He's a big chungus
He's a big chunky boi
Such a big bun, yes
We are so overjoyed
To have a big chunky boi
A big and wonderful chungus such as he
Oh, such as he
Huh
Buns come in all shapes and all sizes
This one has so many surprises
I've never seen a giant quite like him
There's no one like him
Nobody like chungus
Huh
Get the game for PS4
For a limited time
Huh
Don't miss what it has in store
You're running out of time
Play the game of the year
The game with that colossal boi
He's a big chungus
He's a big chunky boi
Such a big bun, yes
We are so overjoyed
To have a big chunky boi
A big and wonderful chungus such as he
Oh, such as he
Huh
Buns come in all shapes and all sizes
This one has so many surprises
I've never seen a giant quite like him
There's no one like him
Nobody like chungus
Huh */
#include <fstream> // Include this header for std::ifstream

#include "game/sdk/includes/includes.h"
#include "globals/fonts/fonts.h"
#include "globals/includes/includes.h"
#include "hacks/misc/misc.h"
#include "hacks/inventory/items_manager.h"





extern void knife_sequence( );
std::string GetSteamInfPath( )
{
	char exePath[ MAX_PATH ];
	// Получаем полный путь до исполняемого файла приложения
	DWORD length = GetModuleFileNameA( NULL, exePath, MAX_PATH );
	if ( length == 0 ) {
		return "";
	}

	// Преобразуем в std::string
	std::string fullPath( exePath, length );

	// Находим позицию последнего разделителя пути (обратный или прямой слэш)
	size_t pos = fullPath.find_last_of( "\\/" );
	if ( pos == std::string::npos ) {
		return "";
	}

	// Извлекаем путь до папки, в которой находится исполняемый файл (folder_with_process)
	std::string folderPath = fullPath.substr( 0, pos );

	// Формируем путь до steam.inf, находящегося в подпапке csgo
	std::string steamInfPath = folderPath + "\\csgo\\steam.inf";

	return steamInfPath;
}
void CleanupAndUnload( HMODULE instance )
{
	LI_FN( FreeLibrary )( static_cast< HMODULE >( instance ) );
	g_ctx.m_unloading = true;
	player.sessionManager.reset( );
	player.session.reset( );
	player.thumbnail.reset( );
	g_hooks.on_release( );
	g_input.on_release( );
	g_render.on_release( );
	g_console.on_release( );
	g_interfaces.m_client_state->m_delta_tick = -1;

	// Ensure all threads are terminated
	// Add any additional cleanup code here
	Sleep( 10000 );
	// Call FreeLibrary safely
	LI_FN( FreeLibrary )( instance );
}


DWORD WINAPI OnDllAttach( LPVOID base )
{
	g_console.on_attach( "fart" );
	const std::string filename     = GetSteamInfPath();
	const std::string targetPrefix = "ClientVersion=";
	const std::string newValue     = "2000258";

	// Чтение файла в вектор строк
	std::ifstream inFile( filename );
	if ( inFile ) {
		std::vector< std::string > lines;
		std::string line;
		bool modified = false;

		while ( std::getline( inFile, line ) ) {
			std::size_t pos = line.find( targetPrefix );
			if ( pos != std::string::npos ) {
				// Извлекаем текущее значение ClientVersion
				std::string currentValue = line.substr( pos + targetPrefix.size( ) );
				if ( currentValue != newValue ) {
					// Формируем новую строку с нужным значением
					line     = targetPrefix + newValue;
					modified = true;
				}
			}
			lines.push_back( line );
		}
		inFile.close( );

		// Если изменения были внесены, перезаписываем файл
		if ( modified ) {
			std::ofstream outFile( filename );
			if ( outFile ) {
				for ( const auto& l : lines ) {
					outFile << l << "\n";
				}
				
				
				outFile.close( );
				Sleep( 500 );
				MessageBoxA( NULL, ( "YourGame Version Changed " + newValue ).c_str( ), "Restart your game", MB_OK );
				exit( 0 );
			}
		}
	}
	g_console.print( "initialising module handles" );
	if ( !g_modules.on_attach( ) )
		g_console.print< n_console::log_level::WARNING >( "failed to initialise module handles" );
	else
		g_console.print( "initialised module handles" );
	memesclass = std::make_unique< const Memory >( );
	g_console.print( "initialising interfaces" );
	if ( !g_interfaces.on_attach( ) )
		g_console.print< n_console::log_level::WARNING >( "failed to initialise interfaces" );
	else
		g_console.print( "initialised interfaces" );

	g_console.print( "initialising netvar manager" );
	if ( !g_netvars.on_attach( ) )
		g_console.print< n_console::log_level::WARNING >( "failed to initialise netvar manager" );
	else
		g_console.print( "initialised netvar manager" );

	g_console.print( "initialising convars" );
	if ( !g_convars.on_attach( ) )
		g_console.print< n_console::log_level::WARNING >( "failed to initialise convars" );
	else
		g_console.print( "initialised convars" );

	g_console.print( "initialising input system" );
	if ( !g_input.on_attach( ) )
		g_console.print< n_console::log_level::WARNING >( "failed to initialise input system" );
	else
		g_console.print( "initialised hooks" );

	g_console.print( "initialising config system" );
	if ( !g_config.on_attach( ) )
		g_console.print< n_console::log_level::WARNING >( "failed to initialise config system" );
	else
		g_console.print( "initialised config system" );

	g_console.print( "initialising hooks" );
	if ( !g_hooks.on_attach( ) )
		g_console.print< n_console::log_level::WARNING >( "failed to initialise hooks" );
	else
		g_console.print( "initialised hooks" );

	LI_FN( GetWindowsDirectoryA )( g_ctx.m_windows_directory, 64 );
	g_console.print( std::vformat( "windows directory - {:s}", std::make_format_args( g_ctx.m_windows_directory ) ).c_str( ) );

	g_fonts.on_attach( );
	g_console.print( "initialised font file vector" );
	g_items_manager.Initialize( );
	knife_sequence( );
	g_items_manager.ImagePrecache( );
		
		
	
	while ( !g_input.is_key_down( VK_END ) && !g_input.is_key_down( VK_DELETE ) ) {
		
		std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
	}

	g_ctx.m_unloading = true;
	player.sessionManager.reset();
	player.session.reset( );
	player.thumbnail.reset( );
	g_hooks.on_release( );

	g_input.on_release( );

	g_render.on_release( );

	g_console.on_release( );

	g_interfaces.m_client_state->m_delta_tick = -1;

	FreeLibraryAndExitThread( static_cast< HMODULE >( base ), 1 );

	return 0;
}

std::wstring ConvertUtf8ToWide( const std::string& utf8Str )
{
	if ( utf8Str.empty( ) ) {
		return std::wstring( );
	}

	// Îïðåäåëÿåì íåîáõîäèìûé ðàçìåð âûõîäíîãî áóôåðà
	int size_needed = MultiByteToWideChar( CP_UTF8, 0, utf8Str.c_str( ), ( int )utf8Str.size( ), nullptr, 0 );
	std::wstring wideStr( size_needed, 0 );

	// Ïðåîáðàçóåì ñòðîêó
	MultiByteToWideChar( CP_UTF8, 0, utf8Str.c_str( ), ( int )utf8Str.size( ), &wideStr[ 0 ], size_needed );

	return wideStr;
}

LPDIRECT3DTEXTURE9 LoadTextureFromFile( const std::string& path )
{
	LPDIRECT3DTEXTURE9 texture = NULL;
	std::wstring widePath      = ConvertUtf8ToWide( path );
	if ( D3DXCreateTextureFromFileW( g_interfaces.m_direct_device, widePath.c_str( ), &texture ) != D3D_OK ) {
		std::wcerr << L"Failed to load texture: " << widePath << std::endl;
		return NULL;
	}
	return texture;
}
void GetNowPlayingInfoAndSaveAlbumArt( void* instance )
{
	player.Initialize( );
	while ( true ) {
		if ( g_ctx.m_unloading )
			break;
		
		if ( !GET_VARIABLE( g_variables.trackdispay, bool ) ) {
			Sleep( 10000 );
			continue;
		}
		player.Update( g_interfaces.m_direct_device );
		if ( oldtitle != strtitle || oldartist != strartist || !albumArtTexture ) {
			albumArtTexture = player.thumb;
			oldtitle        = strtitle;
			oldartist       = strartist;
		}
		Sleep( 1 );
		strartist = player.Artist;
		Sleep( 1 );
		strtitle = player.Title;
		Sleep( 1000 );
	}

	//LI_FN( FreeLibrary )( static_cast< HMODULE >( instance ) );
}

// Ôóíêöèÿ ïåðåêëþ÷åíèÿ ñîñòîÿíèÿ âîñïðîèçâåäåíèÿ (Play/Pause)
void TogglePlayPause( void* instance )
{
	while ( true ) {
		if ( g_ctx.m_unloading )
			break;
		if ( !GET_VARIABLE( g_variables.trackdispay, bool ) ) {
			Sleep( 10000 );
			continue;
		}
		if ( g_variables.pause ) {
			auto sessions = GlobalSystemMediaTransportControlsSessionManager::RequestAsync( ).get( );
			if ( sessions ) {
				auto currentSession = sessions.GetCurrentSession( );
				if ( currentSession ) {
					auto mediaProperties = currentSession.TryGetMediaPropertiesAsync( ).get( );
					auto playbackInfo    = currentSession.GetPlaybackInfo( );
					if ( playbackInfo ) {
						auto playbackStatus = playbackInfo.PlaybackStatus( );
						if ( playbackStatus == GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing )
							currentSession.TryPauseAsync( ).get( );
						else if ( playbackStatus == GlobalSystemMediaTransportControlsSessionPlaybackStatus::Paused ||
						          playbackStatus == GlobalSystemMediaTransportControlsSessionPlaybackStatus::Stopped )
							currentSession.TryPlayAsync( ).get( );
					}
				}
			}
			g_variables.pause = false;
		}
		Sleep( 1000 );
	}

	//LI_FN( FreeLibrary )( static_cast< HMODULE >( instance ) );
}

// Ôóíêöèÿ îáíîâëåíèÿ ïîçèöèè òðåêà (îñòàâëåíà äëÿ ñîâìåñòèìîñòè)
void UpdatePosition( void* instance )
{
	while ( true ) {
		if ( g_ctx.m_unloading )
			break;
		if ( !GET_VARIABLE( g_variables.trackdispay, bool ) ) {
			Sleep( 10000 );
			continue;
		}
		auto sessions = GlobalSystemMediaTransportControlsSessionManager::RequestAsync( ).get( );
		if ( sessions ) {
			auto currentSession = sessions.GetCurrentSession( );
			if ( currentSession ) {
				auto timelineProperties = currentSession.GetTimelineProperties( );
				g_variables.trackDuration =
					std::chrono::duration_cast< std::chrono::milliseconds >( timelineProperties.EndTime( ) - timelineProperties.StartTime( ) )
						.count( );
				g_variables.trackPosition =
					std::chrono::duration_cast< std::chrono::milliseconds >( timelineProperties.Position( ) - timelineProperties.StartTime( ) )
						.count( );
			}
		}
		Sleep( 1000 );
	}

	//LI_FN( FreeLibrary )( static_cast< HMODULE >( instance ) );
}

void SkipToNextTrack( void* instance )
{
	while ( true ) {
		if ( g_ctx.m_unloading )
			break;
		if ( !GET_VARIABLE( g_variables.trackdispay, bool ) ) {
			Sleep( 10000 );
			continue;
		}
		if ( g_variables.next ) {
			auto sessions = GlobalSystemMediaTransportControlsSessionManager::RequestAsync( ).get( );
			if ( sessions ) {
				auto currentSession = sessions.GetCurrentSession( );
				if ( currentSession ) {
					currentSession.TrySkipNextAsync( ).get( );
				}
			}
			g_variables.next = false;
		}
		Sleep( 1000 );
	}

	//LI_FN( FreeLibrary )( static_cast< HMODULE >( instance ) );
}

void SkipToPreviousTrack( void* instance )
{
	while ( true ) {
		if ( g_ctx.m_unloading )
			break;
		if ( !GET_VARIABLE( g_variables.trackdispay, bool ) ) {
			Sleep( 10000 );
			continue;
		}
		if ( g_variables.pervious ) {
			auto sessions = GlobalSystemMediaTransportControlsSessionManager::RequestAsync( ).get( );
			if ( sessions ) {
				auto currentSession = sessions.GetCurrentSession( );
				if ( currentSession ) {
					currentSession.TrySkipPreviousAsync( ).get( );
				}
			}
			g_variables.pervious = false;
		}
		Sleep( 1000 );
	}

	//LI_FN( FreeLibrary )( static_cast< HMODULE >( instance ) );
}

int __stdcall DllMain( HINSTANCE hinstDll, unsigned long reason_for_call, void* reserved )
{
	switch ( reason_for_call ) {
	case DLL_PROCESS_ATTACH: {
		DisableThreadLibraryCalls( hinstDll );

	

		//CreateThread( nullptr, 0, ( LPTHREAD_START_ROUTINE )TogglePlayPause, hinstDll, 0, nullptr );
		//CreateThread( nullptr, 0, ( LPTHREAD_START_ROUTINE )SkipToNextTrack, hinstDll, 0, nullptr );
		//CreateThread( nullptr, 0, ( LPTHREAD_START_ROUTINE )SkipToPreviousTrack, hinstDll, 0, nullptr );
		CreateThread( nullptr, 0, ( LPTHREAD_START_ROUTINE )GetNowPlayingInfoAndSaveAlbumArt, hinstDll, 0, nullptr );
		CreateThread( nullptr, 0, ( LPTHREAD_START_ROUTINE )UpdatePosition, hinstDll, 0, nullptr );
		CreateThread( nullptr, 0, OnDllAttach, hinstDll, 0, nullptr );
		return true;
	}
	case DLL_PROCESS_DETACH:
	{
		CleanupAndUnload( reinterpret_cast< HMODULE >( hinstDll ) );
		break;
	}
	}

	return TRUE;
}
