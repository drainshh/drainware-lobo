#include "fonts2.h"

#include "../dependencies/fonts/anime.h"
#include "../dependencies/fonts/lobotomy.h"

#include "../dependencies/fonts/apple-font/SF-Pro-Display-Semibold.h"
#include "../dependencies/fonts/apple-font/SF-Pro-Display-Bold.h"
#include "../dependencies/fonts/apple-font/SF-Pro-Display-Regular.h"
#include "../dependencies/imgui/helpers/fonts.h"
			   
std::string get_fonts_folder() {
	char windowsPath[MAX_PATH];

	if (GetWindowsDirectoryA(windowsPath, MAX_PATH)) 
		return std::string(windowsPath) + "\\Fonts";
}

void fonts_for_gui::instalisation( ImGuiIO& io )
{
#ifdef _DEBUG
	regular = io.Fonts->AddFontFromMemoryCompressedTTF( verdana_compressed_data, verdana_compressed_size, 13.f );
	bolder  = io.Fonts->AddFontFromMemoryCompressedTTF( verdana_bold_compressed_data, verdana_bold_compressed_size, 13.f );
	title   = io.Fonts->AddFontFromMemoryCompressedTTF( verdana_bold_compressed_data, verdana_bold_compressed_size, 20.f );
	icons   = io.Fonts->AddFontFromMemoryCompressedTTF( icon_font_compressed_data, icon_font_compressed_size, 13.f );
	logo    = io.Fonts->AddFontFromMemoryCompressedTTF( verdana_bold_compressed_data, verdana_bold_compressed_size, 34.f );
	regular_11 = io.Fonts->AddFontFromMemoryCompressedTTF( verdana_compressed_data, verdana_compressed_size, 13.f );
	bolder_11  = io.Fonts->AddFontFromMemoryCompressedTTF( verdana_bold_compressed_data, verdana_bold_compressed_size, 13.f );
	#else
	ImGuiStyle& style = ImGui::GetStyle( );

	io.IniFilename = nullptr;
	io.LogFilename = nullptr;

	auto title_icon_config               = ImFontConfig( );
	title_icon_config.FontData           = ANIME;
	title_icon_config.FontDataSize       = sizeof( ANIME );
	title_icon_config.SizePixels         = 8.0f;
	title_icon_config.RasterizerMultiply = 1.0f;
	title_icon_config.FontBuilderFlags   = ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_Bitmap;

	auto tab_icons_config               = ImFontConfig( );
	tab_icons_config.FontData           = lobotomy;
	tab_icons_config.FontDataSize       = sizeof( lobotomy );
	tab_icons_config.SizePixels         = 14.0f;
	tab_icons_config.RasterizerMultiply = 1.0f;
	tab_icons_config.FontBuilderFlags   = ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_Bitmap;

	auto regular_config               = ImFontConfig( );
	regular_config.FontData           = SFProDisplaySemibold;
	regular_config.FontDataSize       = sizeof( SFProDisplaySemibold );
	regular_config.SizePixels         = 13.0f;
	regular_config.PixelSnapH         = true;
	regular_config.OversampleH        = 5;
	regular_config.OversampleV        = 1;
	regular_config.RasterizerMultiply = 1.0f;
	regular_config.FontBuilderFlags   = ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_Bitmap;

	auto bold_config             = ImFontConfig( );
	bold_config.FontData         = SFProDisplayBold;
	bold_config.FontDataSize     = sizeof( SFProDisplayBold );
	bold_config.SizePixels       = 13.0f;
	bold_config.PixelSnapH       = true;
	bold_config.OversampleH      = 5;
	bold_config.OversampleV      = 1;
	bold_config.FontBuilderFlags = ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_Bitmap;


	auto regular_config_11               = ImFontConfig( );
	regular_config_11.FontData           = SFProDisplayRegular;
	regular_config_11.FontDataSize       = sizeof( SFProDisplayRegular );
	regular_config_11.SizePixels         = 11.0f;
	regular_config_11.PixelSnapH         = true;
	regular_config_11.OversampleH        = 5;
	regular_config_11.OversampleV        = 1;
	regular_config_11.RasterizerMultiply = 1.0f;
	regular_config_11.FontBuilderFlags   = ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_Bitmap |
	                                     ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_Monochrome |
	                                     ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_MonoHinting;

	auto bold_config_11            = ImFontConfig( );
	bold_config_11.FontData         = SFProDisplayBold;
	bold_config_11.FontDataSize     = sizeof( SFProDisplayBold );
	bold_config_11.SizePixels       = 11.0f;
	bold_config_11.PixelSnapH       = true;
	bold_config_11.OversampleH      = 5;
	bold_config_11.OversampleV      = 1;
	bold_config_11.FontBuilderFlags = ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_Bitmap |
	                                  ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_Monochrome |
	                                  ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_MonoHinting;

	auto logo_conf          = ImFontConfig( );
	logo_conf.FontData         = SFProDisplayBold;
	logo_conf.FontDataSize     = sizeof( SFProDisplayBold );
	logo_conf.SizePixels       = 42.0f;
	logo_conf.PixelSnapH       = true;
	logo_conf.OversampleH      = 5;
	logo_conf.OversampleV      = 1;
	logo_conf.FontBuilderFlags = ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_Bitmap;

	regular = io.Fonts->AddFontFromMemoryTTF( regular_config.FontData, regular_config.FontDataSize, regular_config.SizePixels, &regular_config,
	                                          io.Fonts->GetGlyphRangesCyrillic( ) );
	bolder  = io.Fonts->AddFontFromMemoryTTF( bold_config.FontData, bold_config.FontDataSize, bold_config.SizePixels, &bold_config,
	                                          io.Fonts->GetGlyphRangesCyrillic( ) );
	
	regular_11 = regular;
	
	bolder_11 = bolder;
	title   = io.Fonts->AddFontFromMemoryTTF( title_icon_config.FontData, title_icon_config.FontDataSize, title_icon_config.SizePixels,
	                                          &title_icon_config, io.Fonts->GetGlyphRangesCyrillic( ) );
	icons = io.Fonts->AddFontFromMemoryTTF( tab_icons_config.FontData, tab_icons_config.FontDataSize, tab_icons_config.SizePixels, &tab_icons_config,
	                                        io.Fonts->GetGlyphRangesCyrillic( ) );

	logo = io.Fonts->AddFontFromMemoryTTF( logo_conf.FontData, logo_conf.FontDataSize, logo_conf.SizePixels, &logo_conf,
	                                       io.Fonts->GetGlyphRangesCyrillic( ) );
	
	#endif

}