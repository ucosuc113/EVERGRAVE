#pragma once

// USAR UNA INTERACCION Y DECIRLO: el paso que comparten el mundo (clic en la
// Tierra o el Sol), el HUD (botones) y el teclado.
//
// La decision es del Core (`GameState::Use`); esto solo traduce su respuesta a
// lo que se ve: el numero que sube cuando funciono, y el aviso cuando no. Esta
// aparte para que los tres sitios no digan cosas distintas ante el mismo
// rechazo - un Sol bloqueado tiene que decir lo mismo se clique donde se clique.

#include "Session.h"
#include "../View/Format.h"

#include <cstdio>
#include <string>

namespace evergrave {

// `x, y`: donde nace el numero (pixeles de juego). Devuelve lo que dijo el Core.
inline UseResult UseWithFeedback(const InteractionId& id, float x, float y) {
    GameState& game = Session();
    double gained = 0.0;
    const UseResult r = game.Use(id, &gained);
    const InteractionDef* def = game.interactions().Find(id);
    const std::string name = def ? def->name : id;
    switch (r) {
        case UseResult::Ok:
            Ui().floaters.push_back({"+" + FormatAmount(gained), x, y, 0.0f});
            break;
        case UseResult::Locked: {
            const TechDef* t = game.tree().Find(game.UnlockedBy(id));
            Toast(t ? "Locked: research " + t->name + " to unlock " + name : "Locked: " + name);
            break;
        }
        case UseResult::Cooling: {
            char b[64];
            std::snprintf(b, sizeof b, " is recharging (%.1fs)", game.CooldownLeft(id));
            Toast(name + b, 1.2f);
            break;
        }
        case UseResult::Unknown: break;
    }
    return r;
}

} // namespace evergrave
