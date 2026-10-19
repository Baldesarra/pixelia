#pragma once
// Gemeinsame Hilfen der Web-Module: web.cpp (Seite, Status, WLAN, Einstellungen, Update) und web_diag.cpp
// (Fernwartung: Log, Befehle, Leistung, Reset-Historie, Absturzbericht, Diagnosepaket).

#include <cstddef>
#include <string>

#include "esp_http_server.h"

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

// Zugriffsschutz für Befehle und Löschen (Projektplan 4.12): Der optionale PIN kommt mit 0.3.x. Ist er gesetzt,
// müssen Aufrufer ihn im Kopf „X-Pixelia-Pin“ oder im Formularfeld „pin“ mitschicken, sonst 401.
// Bis dahin ist kein PIN gesetzt und alles erlaubt (nur im Heimnetz erreichbar).
bool authorized(httpd_req_t* req, const std::string* body = nullptr);

// WebSocket-Sitzungen von /ws/log (alle anderen WebSocket-Sitzungen bekommen den Status)
bool is_log_client(int fd);

// Endpunkte der Fernwartung anmelden (web_diag.cpp)
void register_diag();

}  // namespace app::web::detail
