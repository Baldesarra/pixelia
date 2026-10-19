#pragma once
// Gemeinsame Hilfen der Web-Module: web.cpp (Seite, Status, WLAN, Einstellungen, Update) und web_diag.cpp
// (Fernwartung: Log, Befehle, Leistung, Reset-Historie, Absturzbericht, Diagnosepaket).

#include <cstddef>
#include <string>

#include "esp_http_server.h"

namespace app {
class Json;
}

namespace app::web::detail {

httpd_handle_t server();
// ws_open: nach dem WebSocket-Handschlag (ESP-IDF 6.1 ruft dabei den Handler selbst nicht mehr auf)
void add(const char* uri, httpd_method_t method, esp_err_t (*handler)(httpd_req_t*), bool ws = false,
         esp_err_t (*ws_open)(httpd_req_t*) = nullptr);

esp_err_t send_json(httpd_req_t* req, const std::string& body, const char* status = "200 OK");
esp_err_t reply(httpd_req_t* req, bool ok, const char* msg, const char* status = nullptr);
bool read_body(httpd_req_t* req, std::string* out, size_t max);
// Wert zu key aus „a=1&b=2“ (Formulardaten oder Query), URL-dekodiert
bool form_value(const std::string& body, const char* key, std::string* out);
bool query_value(httpd_req_t* req, const char* key, std::string* out);

std::string status_json();

// Zugriffsschutz (Projektplan 4.12, auth.h): ohne PIN ist alles erlaubt. Mit PIN braucht es ein gültiges
// Sitzungs-Token im Kopf „X-Pixelia-Token“ oder den PIN im Kopf „X-Pixelia-Pin“ bzw. im Formularfeld „pin“.
bool authorized(httpd_req_t* req, const std::string* body = nullptr);
// Antwort 401 (bzw. 429 während der Sperre) mit {ok:false, auth:true, msg, lockS, left}
esp_err_t deny(httpd_req_t* req);

// WebSocket-Sitzungen von /ws/log (alle anderen WebSocket-Sitzungen bekommen den Status)
bool is_log_client(int fd);

// Endpunkte der Fernwartung anmelden (web_diag.cpp)
void register_diag();

// Info, Hardware, verbundene Geräte (web_info.cpp): Endpunkte anmelden; Nachricht eines Browsers über /ws
void register_info();
void client_message(httpd_req_t* req, const std::string& msg);
void client_names_json(Json& j);

}  // namespace app::web::detail
