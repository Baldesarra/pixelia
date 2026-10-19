// Eingebettete Weboberfläche (web/dist), erzeugt beim Build von tools/web_embed.py (web_assets.cpp im Build-Ordner)
#pragma once

#include <cstddef>
#include <cstdint>

namespace app::web::assets {

struct Asset {
    const char* path;  // z. B. "/index.html", "/app.js"
    const char* mime;
    const uint8_t* data;
    size_t size;
    bool gzip;         // Daten sind gzip-komprimiert (Content-Encoding: gzip)
    const char* etag;  // Inhalts-Hash in Anführungszeichen
};

extern const Asset kAssets[];
extern const size_t kCount;
extern const size_t kBytes;  // Summe der eingebetteten Bytes

}  // namespace app::web::assets
