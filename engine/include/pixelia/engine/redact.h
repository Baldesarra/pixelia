#pragma once
// Geheimnisse aus Diagnose-Ausgaben tilgen (0.2.2): Jedes Vorkommen eines Geheimnisses (z. B. WLAN-Passwort) wird
// durch '*' gleicher Länge ersetzt, bevor Text das Gerät verlässt (Log, Befehlsausgaben, Diagnosepaket).
// Funktioniert gestreamt über Blockgrenzen hinweg: SecretFilter hält die letzten (längstes Geheimnis − 1) Zeichen
// zurück, bis der nächste Block da ist. Zusätzlich zur Rohform wird die JSON-Schreibweise gesucht (\" \\ \uXXXX).
// Plattformunabhängig, im Simulator per --test redact geprüft.

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

namespace pixelia {

class SecretFilter {
public:
    // Ziel der gefilterten Ausgabe; false = Abbruch (z. B. Verbindung weg)
    using Sink = std::function<bool(const char* data, size_t len)>;

    static constexpr size_t kMinSecret = 4;  // kürzere Geheimnisse würden zu viel Text verdecken

    explicit SecretFilter(Sink sink, size_t block = 4096);
    ~SecretFilter();  // überschreibt die Geheimnisse und zurückgehaltenen Text im Speicher
    SecretFilter(const SecretFilter&) = delete;
    SecretFilter& operator=(const SecretFilter&) = delete;

    void add_secret(const char* secret);
    bool write(const char* data, size_t len);
    bool write(const std::string& s) { return write(s.data(), s.size()); }
    bool finish();  // Rest ausgeben

    size_t secrets() const { return secrets_; }  // Zahl der geprüften Geheimnisse (ohne Schreibvarianten)
    size_t hits() const { return hits_; }        // ersetzte Vorkommen

    // Einmal auf einen ganzen Puffer anwenden (kein Streaming); gibt die Zahl der Ersetzungen zurück.
    // patterns müssen mit sort_longest_first() sortiert sein.
    static size_t redact(char* buf, size_t len, const std::vector<std::string>& patterns);
    // Schreibvarianten eines Geheimnisses: roh und, falls verschieden, JSON-escaped
    static void variants(const char* secret, std::vector<std::string>* out);
    static void sort_longest_first(std::vector<std::string>* patterns);

private:
    bool flush(bool all);

    Sink sink_;
    size_t block_;
    std::vector<std::string> patterns_;
    size_t keep_ = 0;  // längstes Muster − 1
    size_t secrets_ = 0;
    size_t hits_ = 0;
    std::string pending_;
    bool ok_ = true;
};

}  // namespace pixelia
