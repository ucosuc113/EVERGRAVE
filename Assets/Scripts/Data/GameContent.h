#pragma once

// EL CONTENIDO DEL JUEGO: que interacciones hay y como se arma una partida.
//
// Las tecnologias estan en TechCatalog.h; las interacciones, aqui. Una
// interaccion nueva es una fila; si hay que ganarsela, una tecnologia con un
// efecto UnlockInteraction que la nombre. GameState::ContentProblems avisa de
// un nombre mal escrito (lo comprueba Tests/core_tests.cpp).
//
// Que un elemento del mundo sea clicable NO esta aqui sino en LabWorld: aqui se
// dice que pasa al usarlo (cuanto, cada cuanto, si esta abierto), alla donde
// esta (la Tierra, el Sol). Asi el Sol no hace nada hasta que su interaccion se
// desbloquea, y eso lo decide el Core, no una casilla de la escena.

#include "TechCatalog.h"
#include "../Core/GameState.h"

namespace evergrave {

inline void RegisterInteractions(InteractionSet& set) {
    set.Add({kGatherId, "Gather Data", "Click the Earth to collect research points.", InteractionTarget::Earth, 1.0, 0.0, true});
    set.Add({"solar_collector", "Solar Collection", "Click the Sun to collect solar energy. Slow to recharge, but worth more than a click on the Earth.",
             InteractionTarget::Sun, 8.0, 5.0, false});
    set.Add({"laser_pulse", "Laser Pulse", "Fire a high-power laser pulse from the HUD. Long recharge, large yield.",
             InteractionTarget::Button, 60.0, 12.0, false});
}

// Una partida nueva con todo el contenido registrado.
inline GameState NewGame() { return GameState(RegisterEarlyEnergyCatalog, RegisterInteractions); }

} // namespace evergrave
