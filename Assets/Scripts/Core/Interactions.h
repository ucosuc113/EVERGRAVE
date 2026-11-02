#pragma once

// LAS INTERACCIONES: lo que el jugador puede HACER, y que no todo esta
// disponible desde el principio.
//
// Una interaccion es una fila de datos (Data/GameContent.h): un nombre, a que
// elemento del mundo se aplica, cuanto da y cada cuanto. Que este desbloqueada
// NO es un campo que alguien enciende: lo decide GameState mirando las
// tecnologias investigadas (un `Effect` UnlockInteraction), o `unlockedAtStart`.
// Asi una interaccion nueva es una fila aqui y, si hay que ganarsela, una linea
// de efecto en una tecnologia; ni el arbol ni el mundo ni el HUD cambian: el
// mundo pregunta "que interaccion le toca a la Tierra?" y el HUD dibuja un
// boton por cada interaccion de tipo Button que este desbloqueada.
//
// Es la mitad "clicker" del juego en datos. La otra mitad - produccion
// automatica - ya cabe en el mismo molde: una interaccion con `automatic` que
// GameState dispare solo; hoy no hay ninguna a proposito.

#include <string>
#include <vector>

namespace evergrave {

using InteractionId = std::string;

// A que elemento se aplica. Earth y Sun son del mundo (se les hace clic encima);
// Button es una accion que vive en el HUD y solo aparece desbloqueada.
enum class InteractionTarget : int { Earth, Sun, Button };

struct InteractionDef {
    InteractionId id;
    std::string name;
    std::string description;
    InteractionTarget target = InteractionTarget::Earth;
    double baseYield = 1.0;    // puntos por uso, antes de efectos
    double cooldown = 0.0;     // segundos entre usos (0 = sin espera)
    bool unlockedAtStart = false;
};

class InteractionSet {
public:
    bool Add(InteractionDef def) {
        if (def.id.empty() || Find(def.id)) return false;
        m_defs.push_back(std::move(def));
        return true;
    }
    const std::vector<InteractionDef>& All() const { return m_defs; }
    const InteractionDef* Find(const InteractionId& id) const {
        for (const InteractionDef& d : m_defs)
            if (d.id == id) return &d;
        return nullptr;
    }
    // La primera interaccion DESBLOQUEADA (segun `unlocked`) que se aplica a ese
    // elemento, o la primera a secas si ninguna lo esta. Un elemento puede tener
    // varias con el tiempo; el clic usa la mejor que el jugador tenga.
    template <typename UnlockedFn>
    const InteractionDef* ForTarget(InteractionTarget t, UnlockedFn&& unlocked) const {
        const InteractionDef* firstAny = nullptr;
        const InteractionDef* lastUnlocked = nullptr;
        for (const InteractionDef& d : m_defs) {
            if (d.target != t) continue;
            if (!firstAny) firstAny = &d;
            if (unlocked(d.id)) lastUnlocked = &d;
        }
        return lastUnlocked ? lastUnlocked : firstAny;
    }

private:
    std::vector<InteractionDef> m_defs;
};

enum class UseResult {
    Ok,
    Unknown,
    Locked,   // todavia no se desbloqueo
    Cooling,  // desbloqueada pero en espera
};

} // namespace evergrave
