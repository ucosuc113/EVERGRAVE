#pragma once

// EL CATALOGO DE TECNOLOGIAS: solo datos.
//
// Para agregar una tecnologia se escribe un `tree.Add({...})` aqui. No se toca
// el arbol, ni GameState, ni la interfaz: el panel dibuja lo que haya en
// `tree.All()`, ordenado por profundidad de requisitos.
//
// Reglas del registro (TechTree::Add las hace cumplir):
//  - el id es unico y estable (se guarda en disco: renombrarlo rompe guardados);
//  - un requisito tiene que estar registrado ANTES, asi que se escribe en orden;
//  - el texto es ASCII (la fuente del motor no tiene acentos).
//
// Este primer tramo es energia temprana y laseres: solo lo que el prototipo
// necesita para probar el arbol. Las eras siguientes (instalaciones
// industriales, escala estelar...) entran como mas filas, con requisitos hacia
// estas. No hay narrativa aqui a proposito: nombres y descripciones son
// tecnicos.
//
// Los costos estan puestos para que el primer tramo se juegue en pocos
// minutos: ~1 punto por clic al inicio, y cada tecnologia de produccion cambia
// el ritmo de la siguiente.

#include "../Core/Tech.h"

namespace evergrave {

inline void RegisterEarlyEnergyCatalog(TechTree& tree) {
    using K = EffectKind;
    const ResourceId R = ResourceId::Research;

    tree.Add({"instrumentation",
              "Lab Instrumentation",
              "Calibrated meters and sensors. Every manual measurement yields more.",
              {{R, 10}},
              {},
              {},
              {{K::ClickPowerAdd, 1.0}}});

    tree.Add({"steam_generators",
              "Steam Generators",
              "A small turbine bank that produces research points on its own.",
              {{R, 25}},
              {"instrumentation"},
              {},
              {{K::PassiveAdd, 0.5}}});

    // Abre el clic sobre el Sol: una interaccion nueva, no solo un numero mayor.
    tree.Add({"solar_collection",
              "Solar Collectors",
              "Instruments aimed at the Sun. Unlocks clicking the Sun to collect solar energy.",
              {{R, 30}},
              {"instrumentation"},
              {},
              {{K::UnlockInteraction, 0, "solar_collector"}}});

    tree.Add({"photon_theory",
              "Photon Theory",
              "A quantum description of light. Better models make every result count for more.",
              {{R, 40}},
              {"instrumentation"},
              {},
              {{K::YieldMultiplier, 1.10}}});

    tree.Add({"power_grid",
              "Power Grid",
              "Distribution lines tie the generators together and cut losses.",
              {{R, 120}},
              {"steam_generators"},
              {{R, 60}},
              {{K::PassiveAdd, 3.0}}});

    tree.Add({"stimulated_emission",
              "Stimulated Emission",
              "Excited atoms can be made to emit coherent light. The first step to a laser.",
              {{R, 80}},
              {"photon_theory"},
              {},
              {{K::ClickPowerAdd, 3.0}}});

    tree.Add({"capacitor_banks",
              "Capacitor Banks",
              "Store energy and release it in a single pulse. Raises all output.",
              {{R, 200}},
              {"power_grid"},
              {},
              {{K::YieldMultiplier, 1.25}, {K::ClickPowerAdd, 8, "solar_collector"}}});

    tree.Add({"ruby_laser",
              "Ruby Laser",
              "The first working laser: a flash-pumped ruby rod.",
              {{R, 150}},
              {"stimulated_emission"},
              {{R, 100}},
              {{K::PassiveAdd, 2.0}, {K::UnlockInteraction, 0, "laser_pulse"}}});

    tree.Add({"gas_lasers",
              "Gas Lasers",
              "Continuous beams from a gas discharge. Needs stored energy and a laser source.",
              {{R, 400}},
              {"ruby_laser", "capacitor_banks"},
              {{R, 500}},
              {{K::PassiveAdd, 10.0}, {K::YieldMultiplier, 1.10}, {K::ClickPowerAdd, 40, "laser_pulse"}}});

    tree.Add({"laser_arrays",
              "Industrial Laser Arrays",
              "Many beams working in step. Output scales to industrial levels.",
              {{R, 900}},
              {"gas_lasers"},
              {{R, 1500}},
              {{K::YieldMultiplier, 1.50}}});
}

} // namespace evergrave
