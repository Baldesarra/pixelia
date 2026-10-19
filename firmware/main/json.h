#pragma once
// Minimaler JSON-Schreiber (cJSON fehlt in ESP-IDF 6.1, der Bedarf ist klein): Kommas und Escaping automatisch.
// Benutzt von der JSON-API (web.cpp, web_diag.cpp) und dem Diagnosepaket (diag.cpp).
// Die Methoden stehen bewusst in json.cpp: inline kopiert der Compiler (-O2) die String-Arbeit in jede Aufrufstelle,
// das kostete 0.2.2 in der Entwicklung über 100 KB Code.

#include <cstddef>
#include <string>

namespace app {

class Json {
public:
    Json& obj(const char* k = nullptr);
    Json& arr(const char* k = nullptr);
    Json& end_obj();
    Json& end_arr();
    Json& str(const char* k, const char* v);
    // Text mit bekannter Länge
    Json& str_n(const char* k, const char* v, size_t n);
    Json& num(const char* k, double v, int decimals = 0);
    Json& boolean(const char* k, bool v);
    Json& null(const char* k);
    // Fertiges JSON (z. B. ein Unterobjekt aus einer anderen Funktion) unverändert einsetzen
    Json& raw(const char* k, const std::string& json);
    std::string& text() { return s_; }

    // JSON-Escaping eines Textes an out anhängen (auch für gestreamte Ausgaben ohne Json-Objekt)
    static void escape(std::string& out, const char* v, size_t n);

private:
    Json& open(const char* k, char c);
    Json& close(char c);
    void key(const char* k);
    void quote(const char* v, size_t n);

    std::string s_;
    bool first_[12] = {};
    int depth_ = -1;
};

}  // namespace app
