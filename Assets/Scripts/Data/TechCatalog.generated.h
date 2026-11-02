// GENERADO por Tools/export_catalog.py desde Design/EVERGRAVE_Balance.xlsx. NO EDITAR A MANO:
// el siguiente export lo pisa. Se cambia en la hoja (Techs / Interactions) y se vuelve a exportar.
#pragma once

#include "../Core/GameState.h"

namespace evergrave {

inline void RegisterGeneratedCatalog(TechTree& tree) {
    using K = EffectKind;
    const ResourceId R = ResourceId::Research;

    tree.Add({"instrumentation",
              "Lab Instrumentation",
              "Calibrated meters and sensors. Every manual measurement yields more.",
              {{R, 10}},
              {},
              {},
              {{K::ClickPowerAdd, 1}}});

    tree.Add({"steam_generators",
              "Steam Generators",
              "A small turbine bank that produces research points on its own.",
              {{R, 25}},
              {"instrumentation"},
              {},
              {{K::PassiveAdd, 0.5}}});

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
              {{K::YieldMultiplier, 1.1}}});

    tree.Add({"power_grid",
              "Power Grid",
              "Distribution lines tie the generators together and cut losses.",
              {{R, 120}},
              {"steam_generators"},
              {{R, 60}},
              {{K::PassiveAdd, 3}}});

    tree.Add({"stimulated_emission",
              "Stimulated Emission",
              "Excited atoms can be made to emit coherent light. The first step to a laser.",
              {{R, 80}},
              {"photon_theory"},
              {},
              {{K::ClickPowerAdd, 3}}});

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
              {{K::PassiveAdd, 2}, {K::UnlockInteraction, 0, "laser_pulse"}}});

    tree.Add({"gas_lasers",
              "Gas Lasers",
              "Continuous beams from a gas discharge. Needs stored energy and a laser source.",
              {{R, 400}},
              {"ruby_laser", "capacitor_banks"},
              {{R, 500}},
              {{K::PassiveAdd, 10}, {K::YieldMultiplier, 1.1}, {K::ClickPowerAdd, 40, "laser_pulse"}}});

    tree.Add({"laser_arrays",
              "Industrial Laser Arrays",
              "Many beams working in step. Output scales to industrial levels.",
              {{R, 900}},
              {"gas_lasers"},
              {{R, 1500}},
              {{K::YieldMultiplier, 1.5}}});
}

inline void RegisterGeneratedInteractions(InteractionSet& set) {
    set.Add({"gather", "Gather Data", "Click the Earth to collect research points.",
             InteractionTarget::Earth, 1, 0, true});
    set.Add({"solar_collector", "Solar Collection", "Click the Sun to collect solar energy. Slow to recharge, but worth more than a click on the Earth.",
             InteractionTarget::Sun, 8, 5, false});
    set.Add({"laser_pulse", "Laser Pulse", "Fire a high-power laser pulse from the HUD. Long recharge, large yield.",
             InteractionTarget::Button, 60, 12, false});
}

// Cuantas tecnologias e interacciones salieron de la hoja: una prueba compara contra esto.
inline constexpr int kGeneratedTechCount = 10;
inline constexpr int kGeneratedInteractionCount = 3;

} // namespace evergrave
