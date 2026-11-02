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

// Un saldo: un decimal mientras es chico, entero despues, K/M cuando crece.
// Nunca "-0.0" ni "1e+06".
inline std::string FormatAmount(double v) {
    if (!(v > 0.0)) v = 0.0;
    char b[40];
    if (v >= 1e6) std::snprintf(b, sizeof b, "%.2fM", v / 1e6);
    else if (v >= 1e4) std::snprintf(b, sizeof b, "%.1fK", v / 1e3);
    else if (v >= 100.0) std::snprintf(b, sizeof b, "%.0f", std::floor(v));
    else std::snprintf(b, sizeof b, "%.1f", std::floor(v * 10.0) / 10.0);
    return b;
}

// Un costo o un umbral: es una cifra exacta que el jugador compara con su saldo,
// asi que no se redondea hacia abajo ni se abrevia.
inline std::string FormatExact(double v) {
    char b[40];
    std::snprintf(b, sizeof b, "%.0f", std::ceil(v - 1e-9));
    return b;
}

inline std::string FormatRate(double perSecond) {
    char b[48];
    std::snprintf(b, sizeof b, "%.1f/s", perSecond);
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
        case EffectKind::YieldMultiplier: std::snprintf(b, sizeof b, "All output +%.0f%%", (e.value - 1.0) * 100.0); break;
    }
    return b;
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
