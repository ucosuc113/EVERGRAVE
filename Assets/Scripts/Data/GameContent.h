#pragma once

// EL CONTENIDO DEL JUEGO: de donde salen las tecnologias y las interacciones, y
// como se arma una partida.
//
// NO SE ESCRIBE AQUI. El contenido vive en la hoja Design/EVERGRAVE_Balance.xlsx
// (hojas Techs e Interactions) y Tools/export_catalog.py lo convierte en
// TechCatalog.generated.h. Asi el balance - costos, requisitos, efectos - se
// disena y se prueba en la hoja (hoja Simulation) y no a ojo en C++.
//
// Una interaccion nueva es una fila en Interactions; si hay que ganarsela, una
// tecnologia con un efecto UnlockInteraction que la nombre. GameState::
// ContentProblems avisa de un nombre mal escrito (lo comprueba
// Tests/core_tests.cpp), y el exportador lo comprueba antes de escribir.
//
// Que un elemento del mundo sea clicable NO esta aqui sino en SpaceWorld: aqui se
// dice que pasa al usarlo (cuanto, cada cuanto, si esta abierto), alla donde
// esta (la Tierra, el Sol). Asi el Sol no hace nada hasta que su interaccion se
// desbloquea, y eso lo decide el Core, no una casilla de la escena.

#include "TechCatalog.generated.h"
#include "../Core/GameState.h"

namespace evergrave {

// Una partida nueva con todo el contenido registrado.
inline GameState NewGame() { return GameState(RegisterGeneratedCatalog, RegisterGeneratedInteractions); }

} // namespace evergrave
