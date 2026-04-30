#include "fonts.h"
#include "../globals.h"
#include <filesystem>

void n_fonts::impl_t::on_attach( )
{
	try {
		for ( const std::filesystem::directory_entry& entry : std::filesystem::recursive_directory_iterator(
				  std::vformat( "{}\\Fonts\\", std::make_format_args( g_ctx.m_windows_directory ) ).c_str( ) ) ) {
			try {
				if ( entry.is_regular_file( ) ) {
					if ( entry.path( ).has_filename( ) && entry.path( ).extension( ) == ".ttf" ) {
						this->m_font_file_names.push_back( reinterpret_cast< const char* >( entry.path( ).stem( ).u8string( ).c_str( ) ) );
					}
				}
			} catch ( const std::exception& e ) {
				// Логируем ошибку и продолжаем
				
			}
		}
	} catch ( const std::exception& e ) {
		// Логируем ошибку при итерации по каталогу
		
	}
}