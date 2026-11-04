#pragma once

// TEXTO PARA LA INTERFAZ: numeros, efectos y condiciones como se leen.
//
// Va aparte del panel para que los numeros que se muestran salgan de UNA cuenta
// (la que prueba Tests/core_tests.cpp) y no de tres printf distintos. Todo es
// ASCII: la fuente del motor no tiene acentos ni simbolos.

#include "../Core/GameState.h"

#include <cmath>
#include <cstdio>
#include <string>

namespace evergrave {

namespace format_detail {

// Las abreviaturas del juego: el progreso llega a 10^28, y "1000000000000.00M" no se lee.
// Cada una vale mil veces la anterior.
struct Suffix { double scale; const char* name; };
inline const Suffix kSuffixes[] = {{1e6, "M"}, {1e9, "B"}, {1e12, "T"}, {1e15, "Qa"}, {1e18, "Qi"},
                                   {1e21, "Sx"}, {1e24, "Sp"}, {1e27, "Oc"}, {1e30, "No"}, {1e33, "Dc"}};

// `v` con dos decimales y su abreviatura. `roundUp` decide la direccion: un SALDO se
// redondea hacia abajo (nunca se muestra mas de lo que hay) y un COSTO hacia arriba
// (nunca se muestra menos de lo que pide): asi "alcanza" y "no alcanza" no se
// contradicen con lo que dicen los numeros.
inline std::string Abbreviate(double v, bool roundUp) {
    const int n = static_cast<int>(sizeof kSuffixes / sizeof kSuffixes[0]);
    int i = 0;
    while (i + 1 < n && v >= kSuffixes[i + 1].scale) ++i;
    auto scaled = [&](int k) {
        const double x = v / kSuffixes[k].scale * 100.0;
        return (roundUp ? std::ceil(x - 1e-9) : std::floor(x + 1e-9)) / 100.0;
    };
    double x = scaled(i);
    // 999.999K redondeado hacia arriba es 1000.00K: se lee mejor como 1.00M.
    if (x >= 1000.0 && i + 1 < n) x = scaled(++i);
    char b[48];
    if (v >= kSuffixes[n - 1].scale * 1000.0) std::snprintf(b, sizeof b, "%.2e", v);
    else std::snprintf(b, sizeof b, "%.2f%s", x, kSuffixes[i].name);
    return b;
}

} // namespace format_detail

// Un saldo: un decimal mientras es chico, entero despues, K/M/B/T... cuando crece.
// Nunca "-0.0" ni "1e+06".
inline std::string FormatAmount(double v) {
    if (!(v > 0.0)) v = 0.0;
    char b[40];
    if (v >= 1e6) return format_detail::Abbreviate(v, false);
    if (v >= 1e4) std::snprintf(b, sizeof b, "%.1fK", v / 1e3);
    else if (v >= 100.0) std::snprintf(b, sizeof b, "%.0f", std::floor(v));
    else std::snprintf(b, sizeof b, "%.1f", std::floor(v * 10.0) / 10.0);
    return b;
}

// Un costo o un umbral: es una cifra exacta que el jugador compara con su saldo,
// asi que no se redondea hacia abajo (hasta un millon es entera; despues se abrevia
// y se redondea hacia arriba).
inline std::string FormatExact(double v) {
    if (v >= 1e6) return format_detail::Abbreviate(v, true);
    char b[40];
    std::snprintf(b, sizeof b, "%.0f", std::ceil(v - 1e-9));
    return b;
}

// Una tasa por segundo. Una instalacion chica da 0.02/s y "0.0/s" mentiria: bajo 0.1 se
// muestran tres decimales; hasta 1000, uno; despues se abrevia como un saldo.
inline std::string FormatRate(double perSecond) {
    if (!(perSecond > 0.0)) perSecond = 0.0;
    char b[48];
    if (perSecond >= 1000.0) return FormatAmount(perSecond) + "/s";
    if (perSecond > 0.0 && perSecond < 0.1) std::snprintf(b, sizeof b, "%.3f/s", perSecond);
    else std::snprintf(b, sizeof b, "%.1f/s", perSecond);
    return b;
}

// `interactions` (opcional) pone el NOMBRE de la interaccion a la que apunta el
// efecto; sin el sale la id.
inline std::string DescribeEffect(const Effect& e, const InteractionSet* interactions = nullptr) {
    char b[120];
    const InteractionDef* target = interactions && !e.target.empty() ? interactions->Find(e.target) : nullptr;
    const std::string targetName = target ? target->name : e.target;
    switch (e.kind) {
        case EffectKind::UnlockInteraction: std::snprintf(b, sizeof b, "Unlocks: %s", targetName.c_str()); break;
        case EffectKind::ClickPowerAdd:
            if (e.target.empty()) std::snprintf(b, sizeof b, "+%g per gather", e.value);
            else std::snprintf(b, sizeof b, "+%g per use of %s", e.value, targetName.c_str());
            break;
        case EffectKind::PassiveAdd: std::snprintf(b, sizeof b, "+%g per second", e.value); break;
        case EffectKind::UnlockBody: {
            BodyId body;
            if (e.target == kMapKey) std::snprintf(b, sizeof b, "Unlocks: the system map");
            else if (BodyFromKey(e.target, body)) std::snprintf(b, sizeof b, "Unlocks: the %s", InfoOf(body).name);
            else std::snprintf(b, sizeof b, "Unlocks: %s", e.target.c_str());
            break;
        }
        case EffectKind::YieldMultiplier: std::snprintf(b, sizeof b, "All output +%.0f%%", (e.value - 1.0) * 100.0); break;
    }
    return b;
}

// Que tecnologia abre un cuerpo (o el mapa, con la clave "map"): la que lleva un efecto UnlockBody
// que lo nombra. Vacio si ninguna lo abre todavia (el Dyson).
inline std::string UnlockerName(const TechTree& tree, const std::string& bodyKey) {
    for (const TechDef& t : tree.All())
        for (const Effect& e : t.effects)
            if (e.kind == EffectKind::UnlockBody && e.target == bodyKey) return t.name;
    return {};
}

inline std::string DescribeCondition(const UnlockCondition& c) {
    return "Earn " + FormatExact(c.lifetimeAtLeast) + " total " + InfoOf(c.resource).name;
}

inline std::string DescribeCost(const std::vector<ResourceAmount>& cost) {
    std::string s;
    for (const ResourceAmount& a : cost) {
        if (!s.empty()) s += ", ";
        s += FormatExact(a.amount) + " " + InfoOf(a.id).name;
    }
    return s.empty() ? "Free" : s;
}

inline const char* DescribeState(TechState s) {
    switch (s) {
        case TechState::Locked: return "LOCKED";
        case TechState::Available: return "AVAILABLE";
        case TechState::Researched: return "RESEARCHED";
    }
    return "";
}

} // namespace evergrave
