#pragma once
#include <cstddef>

namespace n_embedded_assets
{
    struct watermark_asset_t {
        const char* label;
        const unsigned char* data;
        std::size_t size;
    };

    extern const unsigned char g_clarity_watermark_png[];
    extern const std::size_t g_clarity_watermark_png_size;
    extern const unsigned char g_fake_drain_watermark_png[];
    extern const std::size_t g_fake_drain_watermark_png_size;

    extern const watermark_asset_t g_extra_watermark_assets[];
    extern const std::size_t g_extra_watermark_assets_count;
}
