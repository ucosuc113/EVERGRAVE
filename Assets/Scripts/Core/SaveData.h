#pragma once

// GUARDAR Y CARGAR: el estado a texto y de vuelta. Sin disco y sin motor.
//
// Solo lo que NO se puede derivar: el saldo, lo ganado en total, las ids de las
// investigadas, el tiempo y las usos de interacciones. Los modificadores no se guardan
// (GameState::Recompute los saca del conjunto), asi un cambio de balance en el
// catalogo no deja guardados con numeros viejos.
//
// El formato es de lineas `clave=valor`, a proposito legible: para un
// prototipo, poder abrir el guardado y ver que paso vale mas que ahorrar bytes.
// `version` va primero; un guardado de otra version se rechaza entero en vez de
// leerse a medias.
//
// Cargar es TODO O NADA: se lee y valida el texto completo antes de tocar el
// estado. Un archivo cortado a la mitad no puede dejar la partida con el saldo
// nuevo y las tecnologias viejas.

#include "GameState.h"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace evergrave {

inline constexpr int kSaveVersion = 1;

inline std::string SerializeSave(const GameState& g) {
    std::string out;
    char buf[96];
    std::snprintf(buf, sizeof buf, "version=%d\n", kSaveVersion);
    out += buf;
    for (std::size_t i = 0; i < kResourceCount; ++i) {
        const ResourceId id = static_cast<ResourceId>(i);
        std::snprintf(buf, sizeof buf, "%s=%.6f\n", InfoOf(id).key, g.wallet().Get(id));
        out += buf;
        std::snprintf(buf, sizeof buf, "%s.lifetime=%.6f\n", InfoOf(id).key, g.wallet().Lifetime(id));
        out += buf;
    }
    std::snprintf(buf, sizeof buf, "elapsed=%.3f\n", g.Elapsed());
    out += buf;
    std::snprintf(buf, sizeof buf, "uses=%llu\n", static_cast<unsigned long long>(g.UseCount()));
    out += buf;
    for (const TechId& id : g.tree().ResearchedIds()) out += "tech=" + id + "\n";
    // Una linea por tipo que se tenga: `unit=<id>:<cuantas>`. Un guardado viejo no las
    // trae y carga igual (sin instalaciones); uno nuevo en un lector viejo se las salta.
    for (const InstallationDef& d : g.installations().All()) {
        const int n = g.Owned(d.id);
        if (n > 0) out += "unit=" + d.id + ":" + std::to_string(n) + "\n";
    }
    return out;
}

enum class LoadResult {
    Ok,
    BadFormat,    // no se entiende una linea, o falta la version
    WrongVersion, // otra version del formato
};

struct LoadReport {
    LoadResult result = LoadResult::Ok;
    int droppedTechs = 0; // investigadas del guardado que ya no existen o no son coherentes
    int droppedUnits = 0; // tipos de instalacion del guardado que ya no existen
};

namespace save_detail {

inline bool ParseDouble(const std::string& s, double& out) {
    if (s.empty()) return false;
    char* end = nullptr;
    errno = 0;
    const double v = std::strtod(s.c_str(), &end);
    if (errno != 0 || end == s.c_str() || *end != '\0') return false;
    if (!(v == v)) return false; // NaN
    out = v;
    return true;
}

} // namespace save_detail

inline LoadReport DeserializeSave(GameState& g, std::string_view text) {
    LoadReport rep;
    double amount[kResourceCount] = {};
    double lifetime[kResourceCount] = {};
    bool haveVersion = false;
    double elapsed = 0.0, uses = 0.0;
    std::vector<TechId> techs;
    std::unordered_map<InstallationId, int> units;

    std::size_t pos = 0;
    while (pos < text.size()) {
        std::size_t eol = text.find('\n', pos);
        if (eol == std::string_view::npos) eol = text.size();
        std::string line(text.substr(pos, eol - pos));
        pos = eol + 1;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;

        const std::size_t eq = line.find('=');
        if (eq == std::string::npos) { rep.result = LoadResult::BadFormat; return rep; }
        const std::string key = line.substr(0, eq), val = line.substr(eq + 1);

        double n = 0.0;
        if (key == "version") {
            if (!save_detail::ParseDouble(val, n)) { rep.result = LoadResult::BadFormat; return rep; }
            if (static_cast<int>(n) != kSaveVersion) { rep.result = LoadResult::WrongVersion; return rep; }
            haveVersion = true;
        } else if (key == "tech") {
            if (val.empty()) { rep.result = LoadResult::BadFormat; return rep; }
            techs.push_back(val);
        } else if (key == "unit") {
            // `id:cuantas`. Una linea mal formada rechaza el guardado entero, como el resto.
            const std::size_t colon = val.rfind(':');
            if (colon == std::string::npos || colon == 0) { rep.result = LoadResult::BadFormat; return rep; }
            if (!save_detail::ParseDouble(val.substr(colon + 1), n) || n < 0.0 || n > 1e9) { rep.result = LoadResult::BadFormat; return rep; }
            units[val.substr(0, colon)] = static_cast<int>(n);
        } else if (key == "elapsed" || key == "uses") {
            if (!save_detail::ParseDouble(val, n)) { rep.result = LoadResult::BadFormat; return rep; }
            (key == "elapsed" ? elapsed : uses) = n;
        } else {
            // clave de un recurso, con o sin ".lifetime"; una clave que no se
            // conoce se ignora (un guardado de un catalogo con mas recursos).
            for (std::size_t i = 0; i < kResourceCount; ++i) {
                const std::string base = InfoOf(static_cast<ResourceId>(i)).key;
                if (key == base || key == base + ".lifetime") {
                    if (!save_detail::ParseDouble(val, n) || n < 0.0) { rep.result = LoadResult::BadFormat; return rep; }
                    (key == base ? amount : lifetime)[i] = n;
                }
            }
        }
    }
    if (!haveVersion) { rep.result = LoadResult::BadFormat; return rep; }

    // Todo valido: ahora si, se toca el estado.
    for (std::size_t i = 0; i < kResourceCount; ++i)
        g.MutableWallet().Restore(static_cast<ResourceId>(i), amount[i], lifetime[i]);
    rep.droppedTechs = g.MutableTree().RestoreResearched(techs);
    rep.droppedUnits = g.RestoreInstallations(units);
    g.RestoreProgress(elapsed, uses > 0.0 ? static_cast<std::uint64_t>(uses) : 0);
    return rep;
}

} // namespace evergrave
