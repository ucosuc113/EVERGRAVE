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
              "Calibrated meters and sensors. Turns out measuring things properly helps.",
              {{R, 10}},
              {},
              {},
              {{K::ClickPowerAdd, 1}},
              "Instruments", 1, 1});

    tree.Add({"data_logging",
              "Data Logging",
              "A notebook that fills itself in. The interns are suspicious.",
              {{R, 36}},
              {"instrumentation"},
              {},
              {{K::ClickPowerAdd, 1}},
              "Instruments", 1, 2});

    tree.Add({"steam_generators",
              "Steam Generators",
              "Boil water, spin a turbine. Humanity's oldest trick, still undefeated.",
              {{R, 36}},
              {"instrumentation"},
              {},
              {{K::PassiveAdd, 0.5}},
              "Power", 1, 2});

    tree.Add({"solar_collection",
              "Solar Collectors",
              "Point some glass at the Sun. It doesn't mind. Unlocks clicking the Sun.",
              {{R, 36}},
              {"instrumentation"},
              {},
              {{K::UnlockInteraction, 0, "solar_collector"}, {K::UnlockBody, 0, "map"}},
              "Solar", 1, 2});

    tree.Add({"copper_wiring",
              "Copper Wiring",
              "Thin, shiny and surprisingly good at moving electrons around.",
              {{R, 36}},
              {"instrumentation"},
              {},
              {{K::YieldMultiplier, 1.05}},
              "Materials", 1, 2});

    tree.Add({"automated_sampling",
              "Automated Sampling",
              "Little robots that take notes while you click. Dreams do come true.",
              {{R, 129.60000000000002}},
              {"data_logging"},
              {},
              {{K::PassiveAdd, 1}},
              "Instruments", 1, 3});

    tree.Add({"wind_turbines",
              "Wind Turbines",
              "Giant fans that work backwards. Somehow this is a good idea.",
              {{R, 129.60000000000002}},
              {"steam_generators"},
              {},
              {{K::PassiveAdd, 1.5}},
              "Power", 1, 3});

    tree.Add({"power_grid",
              "Power Grid",
              "Wires between everything. Now a blackout is a team sport.",
              {{R, 129.60000000000002}},
              {"steam_generators"},
              {{R, 60}},
              {{K::PassiveAdd, 3}},
              "Power", 1, 3});

    tree.Add({"solar_panels",
              "Solar Panels",
              "Glass on the ground turns sunlight into points. The Sun sends no invoice.",
              {{R, 129.60000000000002}},
              {"solar_collection"},
              {},
              {{K::PassiveAdd, 1}},
              "Solar", 1, 3});

    tree.Add({"steel_framework",
              "Steel Framework",
              "Beams strong enough to hold up bigger machines and bigger egos.",
              {{R, 129.60000000000002}},
              {"copper_wiring"},
              {},
              {{K::PassiveAdd, 1}},
              "Materials", 1, 3});

    tree.Add({"solar_tracking",
              "Solar Tracking",
              "Panels that follow the Sun around all day. Sunflowers, but expensive.",
              {{R, 466.56000000000006}},
              {"solar_panels"},
              {},
              {{K::YieldMultiplier, 1.1}},
              "Solar", 1, 4});

    tree.Add({"sounding_rockets",
              "Sounding Rockets",
              "Cheap rockets that go up, come down, and sometimes carry something useful.",
              {{R, 466.56000000000006}},
              {"steel_framework"},
              {},
              {{K::YieldMultiplier, 1.05}},
              "Orbit", 1, 4});

    tree.Add({"capacitor_banks",
              "Capacitor Banks",
              "Store energy, release it all at once. A very polite lightning bolt.",
              {{R, 466.56000000000006}},
              {"power_grid"},
              {},
              {{K::YieldMultiplier, 1.25}, {K::ClickPowerAdd, 8, "solar_collector"}},
              "Power", 1, 4});

    tree.Add({"insulation",
              "Insulation",
              "Less heat leaks out. The power bill sends flowers.",
              {{R, 466.56000000000006}},
              {"steel_framework"},
              {},
              {{K::YieldMultiplier, 1.05}},
              "Materials", 1, 4});

    tree.Add({"heat_exchangers",
              "Heat Exchangers",
              "Catch the waste heat and put it back to work. Recycling, but for fire.",
              {{R, 1679.616}},
              {"power_grid", "steel_framework"},
              {},
              {{K::YieldMultiplier, 1.1}},
              "Power", 1, 5});

    tree.Add({"thermal_storage",
              "Thermal Storage",
              "Molten salt that holds on to heat. Unlike you, it never lets go.",
              {{R, 6046.6176000000005}},
              {"capacitor_banks"},
              {{R, 400}},
              {{K::PassiveAdd, 5}},
              "Power", 1, 6});

    tree.Add({"hybrid_plant",
              "Hybrid Plant",
              "Wind, sun and a boiler walk into a power station. Nobody leaves complaining.",
              {{R, 21767.823360000002}},
              {"thermal_storage", "wind_turbines", "solar_tracking"},
              {},
              {{K::PassiveAdd, 12}},
              "Power", 1, 7});

    tree.Add({"grid_balancing",
              "Grid Balancing",
              "Smooths the peaks and fills the valleys. Nobody notices. Everybody benefits.",
              {{R, 78364.16409600001}},
              {"hybrid_plant", "heat_exchangers"},
              {},
              {{K::YieldMultiplier, 1.25}},
              "Power", 1, 8});

    tree.Add({"photon_theory",
              "Photon Theory",
              "Light turns out to be both a wave and a particle. Physicists refuse to pick a side.",
              {{R, 52000}},
              {"grid_balancing"},
              {},
              {{K::YieldMultiplier, 1.1}},
              "Optics", 2, 1});

    tree.Add({"stimulated_emission",
              "Stimulated Emission",
              "Convince atoms to emit light in perfect step. Peer pressure, but make it physics.",
              {{R, 93600}},
              {"photon_theory"},
              {},
              {{K::ClickPowerAdd, 5}},
              "Optics", 2, 2});

    tree.Add({"spectrometry",
              "Spectrometry",
              "Split light into colors and read the fine print. Every element signs its work.",
              {{R, 93600}},
              {"photon_theory"},
              {},
              {{K::PassiveAdd, 15}},
              "Instruments", 2, 2});

    tree.Add({"synthetic_crystals",
              "Synthetic Crystals",
              "Grow your own gems in a furnace. Jewelers hate this one trick.",
              {{R, 93600}},
              {"photon_theory", "steel_framework"},
              {},
              {{K::YieldMultiplier, 1.05}},
              "Materials", 2, 2});

    tree.Add({"ruby_laser",
              "Ruby Laser",
              "A flash lamp, a ruby rod and a very bright afternoon. The first laser, and it works. Unlocks the Laser Pulse.",
              {{R, 168480}},
              {"stimulated_emission"},
              {},
              {{K::PassiveAdd, 30}, {K::UnlockInteraction, 0, "laser_pulse"}},
              "Lasers", 2, 3});

    tree.Add({"optical_cavity",
              "Optical Cavity",
              "Two mirrors facing each other, trapping light in a very bright argument.",
              {{R, 168480}},
              {"stimulated_emission"},
              {},
              {{K::YieldMultiplier, 1.1}},
              "Optics", 2, 3});

    tree.Add({"flashlamp_arrays",
              "Flashlamp Arrays",
              "Camera flashes, but angrier and wired to the capacitor banks.",
              {{R, 168480}},
              {"capacitor_banks", "stimulated_emission"},
              {},
              {{K::PassiveAdd, 50}},
              "Power", 2, 3});

    tree.Add({"optical_coatings",
              "Optical Coatings",
              "A layer thinner than a hair that stops light from escaping. Fancy sunscreen for mirrors.",
              {{R, 168480}},
              {"synthetic_crystals"},
              {},
              {{K::YieldMultiplier, 1.05}},
              "Materials", 2, 3});

    tree.Add({"gas_lasers",
              "Gas Lasers",
              "A tube of gas, a lot of voltage, and a beam that never gets tired.",
              {{R, 303264.00000000006}},
              {"ruby_laser", "flashlamp_arrays"},
              {},
              {{K::PassiveAdd, 120}, {K::YieldMultiplier, 1.1}},
              "Lasers", 2, 4});

    tree.Add({"beam_focusing",
              "Beam Focusing",
              "Squeeze the beam through a lens. The pulse hits harder, the Sun gets jealous.",
              {{R, 303264.00000000006}},
              {"ruby_laser"},
              {},
              {{K::ClickPowerAdd, 20, "laser_pulse"}},
              "Lasers", 2, 4});

    tree.Add({"rare_earth_doping",
              "Rare Earth Doping",
              "Sprinkle exotic metals into the crystal. The periodic table's bottom shelf finally pays off.",
              {{R, 303264.00000000006}},
              {"synthetic_crystals", "ruby_laser"},
              {},
              {{K::PassiveAdd, 80}},
              "Materials", 2, 4});

    tree.Add({"beam_profiling",
              "Beam Profiling",
              "Check what shape your beam really is. Spoiler: not a perfect circle.",
              {{R, 303264.00000000006}},
              {"spectrometry"},
              {},
              {{K::ClickPowerAdd, 10}},
              "Instruments", 2, 4});

    tree.Add({"relay_optics",
              "Relay Optics",
              "Mirrors and lenses that let satellites pass light along. Unlocks the Laser Relay.",
              {{R, 303264.00000000006}},
              {"sounding_rockets", "ruby_laser"},
              {},
              {{K::YieldMultiplier, 1.05}},
              "Orbit", 2, 4});

    tree.Add({"semiconductor_diodes",
              "Semiconductor Diodes",
              "A laser the size of a grain of rice. Your pointer is now a distant relative.",
              {{R, 545875.2000000001}},
              {"gas_lasers", "rare_earth_doping"},
              {},
              {{K::PassiveAdd, 300}},
              "Lasers", 2, 5});

    tree.Add({"pointing_systems",
              "Pointing Systems",
              "Keep a beam on target from orbit. Harder than it sounds, easier than threading a needle.",
              {{R, 545875.2000000001}},
              {"relay_optics"},
              {},
              {{K::PassiveAdd, 150}},
              "Orbit", 2, 5});

    tree.Add({"interferometers",
              "Interferometers",
              "Measure distances in wavelengths. Finally a ruler that deserves the name.",
              {{R, 545875.2000000001}},
              {"beam_profiling", "optical_cavity"},
              {},
              {{K::YieldMultiplier, 1.15}},
              "Instruments", 2, 5});

    tree.Add({"pulse_capacitors",
              "Pulse Capacitors",
              "Hoard charge, then dump it in one flash. Makes the Laser Pulse hit like a freight train.",
              {{R, 545875.2000000001}},
              {"beam_focusing", "flashlamp_arrays"},
              {},
              {{K::ClickPowerAdd, 120, "laser_pulse"}},
              "Lasers", 2, 5});

    tree.Add({"fiber_amplifiers",
              "Fiber Amplifiers",
              "Glass thread that makes light stronger as it travels. Cables with a secret life.",
              {{R, 982575.3600000001}},
              {"semiconductor_diodes"},
              {},
              {{K::PassiveAdd, 700}, {K::YieldMultiplier, 1.1}},
              "Lasers", 2, 6});

    tree.Add({"adaptive_optics",
              "Adaptive Optics",
              "Mirrors that wiggle to cancel the sky's mood swings. Unlocks the Optical Ground Station.",
              {{R, 982575.3600000001}},
              {"pointing_systems", "interferometers"},
              {},
              {{K::YieldMultiplier, 1.2}},
              "Orbit", 2, 6});

    tree.Add({"mode_locking",
              "Mode Locking",
              "Force every color of light to arrive at once. Rude, but efficient.",
              {{R, 1768635.6480000003}},
              {"optical_cavity", "fiber_amplifiers"},
              {},
              {{K::YieldMultiplier, 1.25}},
              "Optics", 2, 7});

    tree.Add({"q_switching",
              "Q-Switching",
              "Bottle the energy, pop the cork. Turns a polite beam into a very loud one.",
              {{R, 3183544.1664000005}},
              {"mode_locking"},
              {},
              {{K::ClickPowerAdd, 500, "laser_pulse"}},
              "Optics", 2, 8});

    tree.Add({"chirped_pulses",
              "Chirped Pulse Amplification",
              "Stretch the pulse, amplify it, squeeze it back. Wins a Nobel prize and everyone's respect.",
              {{R, 5730379.499520001}},
              {"q_switching", "fiber_amplifiers"},
              {},
              {{K::YieldMultiplier, 1.5}},
              "Lasers", 2, 9});

    tree.Add({"laser_arrays",
              "Industrial Laser Arrays",
              "Hundreds of lasers firing in step. The electric company stops returning your calls.",
              {{R, 19032000}},
              {"chirped_pulses"},
              {{R, 1500}},
              {{K::YieldMultiplier, 1.25}},
              "Industrial", 3, 1});

    tree.Add({"industrial_grid",
              "Industrial Grid",
              "A grid built for factories, not for lamps. The lamps are just along for the ride.",
              {{R, 36160800}},
              {"laser_arrays"},
              {},
              {{K::PassiveAdd, 600}},
              "Industrial", 3, 2});

    tree.Add({"process_automation",
              "Process Automation",
              "Machines that supervise other machines. Middle management, solved.",
              {{R, 36160800}},
              {"laser_arrays"},
              {},
              {{K::ClickPowerAdd, 6}},
              "Instruments", 3, 2});

    tree.Add({"alloy_foundries",
              "Alloy Foundries",
              "Metals mixed on purpose, with a recipe. Copper finally has someone to talk to.",
              {{R, 36160800}},
              {"laser_arrays", "optical_coatings"},
              {},
              {{K::YieldMultiplier, 1.05}},
              "Materials", 3, 2});

    tree.Add({"high_voltage_lines",
              "High-Voltage Lines",
              "Towers marching across the countryside. Birds treat them as a fun fair.",
              {{R, 68705520}},
              {"industrial_grid"},
              {},
              {{K::PassiveAdd, 1200}},
              "Power", 3, 3});

    tree.Add({"plc_controllers",
              "PLC Controllers",
              "Tough little computers that run a factory floor and never ask for lunch.",
              {{R, 68705520}},
              {"process_automation"},
              {},
              {{K::YieldMultiplier, 1.1}},
              "Instruments", 3, 3});

    tree.Add({"ceramic_composites",
              "Ceramic Composites",
              "Stuff that laughs at heat. Great for furnaces, terrible for dinner plates.",
              {{R, 68705520}},
              {"alloy_foundries"},
              {},
              {{K::PassiveAdd, 800}},
              "Materials", 3, 3});

    tree.Add({"rail_freight",
              "Rail Freight",
              "Hundreds of tons moved by two lines of steel. Trains are just very long, very patient trucks.",
              {{R, 68705520}},
              {"industrial_grid"},
              {},
              {{K::YieldMultiplier, 1.05}},
              "Logistics", 3, 3});

    tree.Add({"fission_reactors",
              "Fission Reactors",
              "Split atoms on purpose, with adult supervision. A kettle with ambitions, level two.",
              {{R, 130540487.99999999}},
              {"high_voltage_lines", "ceramic_composites"},
              {},
              {{K::PassiveAdd, 3600}},
              "Power", 3, 4});

    tree.Add({"heavy_industry",
              "Heavy Industry",
              "Steel mills, refineries and a skyline that smells like money.",
              {{R, 130540487.99999999}},
              {"industrial_grid", "alloy_foundries"},
              {},
              {{K::YieldMultiplier, 1.1}},
              "Industrial", 3, 4});

    tree.Add({"smart_meters",
              "Smart Meters",
              "Every house now reports its usage in real time. Your fridge has opinions about it. Unlocks the Smart City.",
              {{R, 130540487.99999999}},
              {"plc_controllers"},
              {},
              {{K::PassiveAdd, 1600}},
              "Instruments", 3, 4});

    tree.Add({"containerized_shipping",
              "Containerized Shipping",
              "One box size to rule them all. Shipping gets faster, boredom gets bigger.",
              {{R, 130540487.99999999}},
              {"rail_freight"},
              {},
              {{K::YieldMultiplier, 1.1}},
              "Logistics", 3, 4});

    tree.Add({"superconductors",
              "Superconductors",
              "Wires with zero resistance, as long as you keep them very cold. Princesses of the electrical world.",
              {{R, 248026927.19999996}},
              {"ceramic_composites", "high_voltage_lines"},
              {},
              {{K::YieldMultiplier, 1.15}},
              "Materials", 3, 5});

    tree.Add({"breeder_reactors",
              "Breeder Reactors",
              "A reactor that makes more fuel than it burns. Accountants love it, physicists nod politely.",
              {{R, 248026927.19999996}},
              {"fission_reactors"},
              {},
              {{K::PassiveAdd, 8000}},
              "Power", 3, 5});

    tree.Add({"factory_robots",
              "Factory Robots",
              "Arms that weld, paint and never complain about the music.",
              {{R, 248026927.19999996}},
              {"heavy_industry", "plc_controllers"},
              {},
              {{K::PassiveAdd, 5000}},
              "Industrial", 3, 5});

    tree.Add({"distributed_computing",
              "Distributed Computing",
              "Ten thousand small computers pretending to be one big one. It mostly works.",
              {{R, 248026927.19999996}},
              {"smart_meters"},
              {},
              {{K::YieldMultiplier, 1.1}},
              "Instruments", 3, 5});

    tree.Add({"cryogenic_cooling",
              "Cryogenic Cooling",
              "Fridges that make a freezer look cozy. Keeps the superconductors in a good mood.",
              {{R, 471251161.6799998}},
              {"superconductors"},
              {},
              {{K::PassiveAdd, 12000}},
              "Materials", 3, 6});

    tree.Add({"grid_storage",
              "Grid Storage",
              "Batteries the size of buildings. Charges at night, flexes at noon.",
              {{R, 471251161.6799998}},
              {"breeder_reactors", "superconductors"},
              {},
              {{K::YieldMultiplier, 1.2}},
              "Power", 3, 6});

    tree.Add({"megafactories",
              "Megafactories",
              "A factory so big it has its own weather. Wear a jacket on the east side.",
              {{R, 471251161.6799998}},
              {"factory_robots"},
              {},
              {{K::PassiveAdd, 18000}},
              "Industrial", 3, 6});

    tree.Add({"high_speed_rail",
              "High-Speed Rail",
              "Trains that skip the stations out of spite. Cities suddenly feel closer.",
              {{R, 471251161.6799998}},
              {"containerized_shipping", "superconductors"},
              {},
              {{K::YieldMultiplier, 1.1}},
              "Logistics", 3, 6});

    tree.Add({"tokamak_prototype",
              "Tokamak Prototype",
              "A doughnut full of plasma, held in place by magnets and sheer optimism.",
              {{R, 895377207.1919998}},
              {"grid_storage"},
              {},
              {{K::PassiveAdd, 36000}},
              "Power", 3, 7});

    tree.Add({"predictive_maintenance",
              "Predictive Maintenance",
              "Machines tell you they're about to break. Finally, honest machines.",
              {{R, 895377207.1919998}},
              {"distributed_computing", "megafactories"},
              {},
              {{K::YieldMultiplier, 1.15}},
              "Instruments", 3, 7});

    tree.Add({"orbital_launch_sites",
              "Orbital Launch Sites",
              "Pads built to ship things to space on a schedule. Space has an inbox now.",
              {{R, 895377207.1919998}},
              {"containerized_shipping", "megafactories"},
              {},
              {{K::PassiveAdd, 20000}},
              "Orbit", 3, 7});

    tree.Add({"inertial_confinement",
              "Inertial Confinement",
              "Point a lot of lasers at a tiny pellet and see what happens. Sun-making, with extra steps.",
              {{R, 1701216693.6647995}},
              {"tokamak_prototype", "chirped_pulses"},
              {},
              {{K::PassiveAdd, 80000}},
              "Optics", 3, 8});

    tree.Add({"national_supergrid",
              "National Supergrid",
              "One grid for a whole country. When it's windy in the north, the south enjoys it.",
              {{R, 1701216693.6647995}},
              {"grid_storage", "tokamak_prototype"},
              {},
              {{K::YieldMultiplier, 1.25}},
              "Power", 3, 8});

    tree.Add({"city_scale_ai",
              "City-Scale AI",
              "An algorithm runs the traffic lights. Nobody understands how it decides, but it's faster.",
              {{R, 1701216693.6647995}},
              {"predictive_maintenance"},
              {},
              {{K::YieldMultiplier, 1.2}},
              "Instruments", 3, 8});

    tree.Add({"megaproject_financing",
              "Megaproject Financing",
              "A very large check and a very small footnote. Cranes appear on the horizon.",
              {{R, 3232311717.9631186}},
              {"orbital_launch_sites", "national_supergrid"},
              {},
              {{K::YieldMultiplier, 1.1}},
              "Logistics", 3, 9});

    tree.Add({"continental_grid",
              "Continental Grid",
              "Power flows across oceans. Time zones are now an energy resource.",
              {{R, 6141392264.129925}},
              {"national_supergrid", "megaproject_financing"},
              {},
              {{K::PassiveAdd, 200000}},
              "Industrial", 3, 10});

    tree.Add({"planetary_industry",
              "Planetary Industrial Complex",
              "The whole planet now works one shift. The planet did not ask for this.",
              {{R, 11668645301.846859}},
              {"inertial_confinement", "city_scale_ai", "continental_grid"},
              {},
              {{K::YieldMultiplier, 1.75}},
              "Industrial", 3, 11});

    tree.Add({"reusable_rockets",
              "Reusable Rockets",
              "Land the rocket, refuel it, launch it again. Turns out throwing them away was the expensive part.",
              {{R, 23257104000}},
              {"orbital_launch_sites"},
              {},
              {{K::YieldMultiplier, 1.1}},
              "Logistics", 4, 1});

    tree.Add({"orbital_tugs",
              "Orbital Tugs",
              "Little engines that push big things around up there. Space needs its own tow trucks.",
              {{R, 45351352800}},
              {"reusable_rockets"},
              {},
              {{K::PassiveAdd, 150000}},
              "Orbit", 4, 2});

    tree.Add({"carbon_nanotubes",
              "Carbon Nanotubes",
              "Threads of carbon stronger than steel and a fraction of the weight. The future was made of pencil lead.",
              {{R, 45351352800}},
              {"cryogenic_cooling"},
              {},
              {{K::YieldMultiplier, 1.05}},
              "Materials", 4, 2});

    tree.Add({"fusion_pilot",
              "Fusion Pilot Plant",
              "A real plant that burns hydrogen like a small star. Does not explode. Mostly.",
              {{R, 45351352800}},
              {"inertial_confinement"},
              {},
              {{K::PassiveAdd, 180000}},
              "Power", 4, 2});

    tree.Add({"orbital_assembly",
              "Orbital Assembly",
              "Build big things where there's no gravity to complain. The crane has the day off, forever.",
              {{R, 88435137960}},
              {"orbital_tugs", "megafactories"},
              {},
              {{K::PassiveAdd, 300000}},
              "Industrial", 4, 3});

    tree.Add({"space_solar_arrays",
              "Space Solar Arrays",
              "Panels above the clouds, with no night shift. The Sun never calls in sick.",
              {{R, 88435137960}},
              {"orbital_tugs"},
              {},
              {{K::PassiveAdd, 360000}},
              "Power", 4, 3});

    tree.Add({"microgravity_labs",
              "Microgravity Labs",
              "Experiments where nothing falls and everything floats. Very good for science, very bad for coffee.",
              {{R, 88435137960}},
              {"orbital_tugs"},
              {},
              {{K::YieldMultiplier, 1.1}},
              "Instruments", 4, 3});

    tree.Add({"mirror_substrates",
              "Mirror Substrates",
              "Ultra-light backing for giant mirrors. Think of an eggshell that tans well.",
              {{R, 88435137960}},
              {"carbon_nanotubes"},
              {},
              {{K::YieldMultiplier, 1.05}},
              "Materials", 4, 3});

    tree.Add({"thin_film_reflectors",
              "Thin-Film Reflectors",
              "A mirror thinner than a soap bubble, a thousand times shinier. Unlocks the Orbital Mirror.",
              {{R, 172448519022}},
              {"mirror_substrates", "orbital_tugs"},
              {},
              {{K::YieldMultiplier, 1.1}},
              "Optics", 4, 4});

    tree.Add({"power_beaming",
              "Power Beaming",
              "Send energy through the air instead of wires. Weirdly, it works better than the Wi-Fi.",
              {{R, 172448519022}},
              {"space_solar_arrays", "inertial_confinement"},
              {},
              {{K::PassiveAdd, 750000}},
              "Power", 4, 4});

    tree.Add({"orbital_foundries",
              "Orbital Foundries",
              "Melt metal in space, where it falls into the right shape on its own. Quality control: gravity-free.",
              {{R, 172448519022}},
              {"orbital_assembly"},
              {},
              {{K::PassiveAdd, 600000}},
              "Industrial", 4, 4});

    tree.Add({"satellite_servicing",
              "Satellite Servicing",
              "Technicians in suits fix satellites mid-orbit. Space mechanics now charge by the hour.",
              {{R, 172448519022}},
              {"orbital_assembly"},
              {},
              {{K::YieldMultiplier, 1.1}},
              "Orbit", 4, 4});

    tree.Add({"mirror_constellations",
              "Mirror Constellations",
              "Dozens of mirrors flying in formation. The tidiest constellation in the sky.",
              {{R, 336274612092.89996}},
              {"thin_film_reflectors", "satellite_servicing"},
              {},
              {{K::YieldMultiplier, 1.15}},
              "Orbit", 4, 5});

    tree.Add({"microwave_downlink",
              "Microwave Downlink",
              "Beam it all down to a field of antennas. Birds are advised to avoid the area.",
              {{R, 336274612092.89996}},
              {"power_beaming"},
              {},
              {{K::PassiveAdd, 1400000}},
              "Power", 4, 5});

    tree.Add({"asteroid_prospecting",
              "Asteroid Prospecting",
              "Find the rocks worth hitting. Much of the solar system is just a very slow gold rush.",
              {{R, 336274612092.89996}},
              {"satellite_servicing"},
              {},
              {{K::PassiveAdd, 1200000}},
              "Orbit", 4, 5});

    tree.Add({"fusion_commercial",
              "Commercial Fusion",
              "Fusion with a price tag and a customer service line. The helpline is mostly physicists.",
              {{R, 336274612092.89996}},
              {"fusion_pilot", "power_beaming"},
              {},
              {{K::YieldMultiplier, 1.2}},
              "Power", 4, 5});

    tree.Add({"graphene_cable",
              "Graphene Cable",
              "A one-atom-thick sheet rolled into a rope you could hang a city from. Don't tell the spider.",
              {{R, 655735493581.1549}},
              {"carbon_nanotubes", "mirror_substrates"},
              {},
              {{K::YieldMultiplier, 1.1}},
              "Materials", 4, 6});

    tree.Add({"autonomous_swarms",
              "Autonomous Swarms",
              "Hundreds of small machines that cooperate without a manager. A first for any workplace.",
              {{R, 655735493581.1549}},
              {"microgravity_labs", "mirror_constellations"},
              {},
              {{K::YieldMultiplier, 1.1}},
              "Instruments", 4, 6});

    tree.Add({"zero_g_manufacturing",
              "Zero-G Manufacturing",
              "Perfect crystals, flawless alloys and no dust. Cleanrooms are jealous.",
              {{R, 655735493581.1549}},
              {"orbital_foundries", "microgravity_labs"},
              {},
              {{K::PassiveAdd, 2400000}},
              "Industrial", 4, 6});

    tree.Add({"magnetic_launch",
              "Magnetic Launch",
              "A very long track that throws payloads at the sky. The rocket is optional.",
              {{R, 655735493581.1549}},
              {"reusable_rockets", "fusion_commercial"},
              {},
              {{K::YieldMultiplier, 1.1}},
              "Logistics", 4, 6});

    tree.Add({"elevator_tether",
              "Elevator Tether",
              "A cable from the ground to space, finally strong enough to be a bad idea in a good way. Unlocks the Space Elevator.",
              {{R, 1278684212483.2522}},
              {"graphene_cable", "magnetic_launch"},
              {},
              {{K::YieldMultiplier, 1.15}},
              "Orbit", 4, 7});

    tree.Add({"deep_space_comms",
              "Deep-Space Comms",
              "Talk to things that are very far away. They answer, eventually. It's like email, but with lag.",
              {{R, 1278684212483.2522}},
              {"autonomous_swarms"},
              {},
              {{K::PassiveAdd, 3600000}},
              "Instruments", 4, 7});

    tree.Add({"orbital_habitats",
              "Orbital Habitats",
              "Rotating rings with gardens inside. The first place where 'down' is a matter of opinion.",
              {{R, 1278684212483.2522}},
              {"zero_g_manufacturing"},
              {},
              {{K::PassiveAdd, 4500000}},
              "Industrial", 4, 7});

    tree.Add({"helium3_mining",
              "Helium-3 Mining",
              "Scoop rare gas off a gas giant. Ridiculous, expensive and exactly what a fusion plant dreams of.",
              {{R, 1278684212483.2522}},
              {"asteroid_prospecting", "fusion_commercial"},
              {},
              {{K::PassiveAdd, 3900000}},
              "Power", 4, 7});

    tree.Add({"beamed_propulsion",
              "Beamed Propulsion",
              "Push a spacecraft with a laser, no fuel on board. Sailing, but with a death ray.",
              {{R, 2493434214342.342}},
              {"mirror_constellations", "helium3_mining"},
              {},
              {{K::YieldMultiplier, 1.2}},
              "Optics", 4, 8});

    tree.Add({"lunar_base",
              "Lunar Base",
              "A flag, a dome and a very good view. Rent is low, deliveries are not.",
              {{R, 2493434214342.342}},
              {"magnetic_launch", "orbital_habitats"},
              {},
              {{K::PassiveAdd, 7500000}},
              "Logistics", 4, 8});

    tree.Add({"orbital_hub",
              "Orbital Hub",
              "A space station that is also a shipyard, a market and a bus stop. Plan your layover.",
              {{R, 2493434214342.342}},
              {"zero_g_manufacturing", "orbital_habitats"},
              {},
              {{K::PassiveAdd, 9000000}},
              "Industrial", 4, 8});

    tree.Add({"orbital_ring",
              "Orbital Ring",
              "A loop of power around the planet. If you stand in the right spot, it's a good view.",
              {{R, 4862196717967.566}},
              {"microwave_downlink", "elevator_tether"},
              {},
              {{K::PassiveAdd, 15000000}},
              "Power", 4, 9});

    tree.Add({"mass_drivers",
              "Mass Drivers",
              "Fling cargo from the Moon with magnets. Shipping costs fall off a cliff, gently.",
              {{R, 4862196717967.566}},
              {"lunar_base", "magnetic_launch"},
              {},
              {{K::PassiveAdd, 12000000}},
              "Logistics", 4, 9});

    tree.Add({"orbital_complex",
              "Orbital Industrial Complex",
              "Everything above the atmosphere, working together. The sky has a skyline now.",
              {{R, 9481283600036.754}},
              {"beamed_propulsion", "orbital_hub", "orbital_ring"},
              {},
              {{K::YieldMultiplier, 1.75}},
              "Orbit", 4, 10});

    tree.Add({"interplanetary_transit",
              "Interplanetary Transit",
              "Regular flights between planets. Frequent flyers get a moon named after them.",
              {{R, 23233846896000}},
              {"orbital_complex"},
              {},
              {{K::YieldMultiplier, 1.1}},
              "Logistics", 5, 1});

    tree.Add({"mercury_survey",
              "Mercury Survey",
              "Map the smallest planet from orbit. It's hot, it's close, and it's covered in metal. Perfect.",
              {{R, 40659232068000}},
              {"interplanetary_transit"},
              {},
              {{K::PassiveAdd, 10000000}},
              "Orbit", 5, 2});

    tree.Add({"solar_sails",
              "Solar Sails",
              "Sheets of foil that ride sunlight. Ships with no engine and no fuel bill.",
              {{R, 40659232068000}},
              {"interplanetary_transit", "graphene_cable"},
              {},
              {{K::YieldMultiplier, 1.05}},
              "Materials", 5, 2});

    tree.Add({"fusion_torches",
              "Fusion Torches",
              "A rocket that burns the same stuff as a star. Handle with a very large oven mitt.",
              {{R, 40659232068000}},
              {"interplanetary_transit", "fusion_commercial"},
              {},
              {{K::PassiveAdd, 12000000}},
              "Power", 5, 2});

    tree.Add({"mercury_mining",
              "Mercury Mining",
              "Scoop metal off the closest planet to the Sun. Sunscreen: SPF one million.",
              {{R, 71153656119000}},
              {"mercury_survey"},
              {},
              {{K::PassiveAdd, 20000000}},
              "Mining", 5, 3});

    tree.Add({"self_replicating_factories",
              "Self-Replicating Factories",
              "Factories that build more factories. Nobody remembers who asked for the first one.",
              {{R, 71153656119000}},
              {"orbital_hub", "mercury_survey"},
              {},
              {{K::PassiveAdd, 24000000}},
              "Industrial", 5, 3});

    tree.Add({"interplanetary_net",
              "Interplanetary Network",
              "A web of relays between worlds. Ping times are measured in coffee breaks.",
              {{R, 71153656119000}},
              {"deep_space_comms", "mercury_survey"},
              {},
              {{K::YieldMultiplier, 1.1}},
              "Instruments", 5, 3});

    tree.Add({"heat_resistant_alloys",
              "Heat-Resistant Alloys",
              "Metals that shrug at temperatures that melt lead, steel and most opinions.",
              {{R, 71153656119000}},
              {"solar_sails"},
              {},
              {{K::YieldMultiplier, 1.05}},
              "Materials", 5, 3});

    tree.Add({"heliostat_fields",
              "Heliostat Fields",
              "Thousands of mirrors all pointed at one spot. Nobody dares stand there. Unlocks the Mercury Mirror Farm.",
              {{R, 124518898208250}},
              {"mercury_mining", "thin_film_reflectors"},
              {},
              {{K::YieldMultiplier, 1.1}},
              "Optics", 5, 4});

    tree.Add({"orbital_smelters",
              "Orbital Smelters",
              "Melt rock in orbit with a sunbeam. The leftover slag is called 'future building material'.",
              {{R, 124518898208250}},
              {"mercury_mining", "self_replicating_factories"},
              {},
              {{K::PassiveAdd, 40000000}},
              "Industrial", 5, 4});

    tree.Add({"solar_power_satellites",
              "Heliocentric Collectors",
              "Collectors that orbit the Sun instead of the Earth. Lonely, but never in the shade.",
              {{R, 124518898208250}},
              {"solar_sails"},
              {},
              {{K::PassiveAdd, 50000000}},
              "Solar", 5, 4});

    tree.Add({"gas_giant_scooping",
              "Gas Giant Scooping",
              "Dip a tanker into Jupiter's clouds and fly away fast. Gas stations get a whole new meaning.",
              {{R, 124518898208250}},
              {"fusion_torches", "helium3_mining"},
              {},
              {{K::PassiveAdd, 45000000}},
              "Mining", 5, 4});

    tree.Add({"asteroid_capture",
              "Asteroid Capture",
              "Grab a rock and park it somewhere convenient. Parking fees are cosmic. Unlocks the Asteroid Miner.",
              {{R, 217908071864437.5}},
              {"mercury_survey", "asteroid_prospecting"},
              {},
              {{K::YieldMultiplier, 1.1}},
              "Orbit", 5, 5});

    tree.Add({"autonomous_prospectors",
              "Autonomous Prospectors",
              "Little probes that sniff out ore and email you the coordinates. Surprisingly polite.",
              {{R, 217908071864437.5}},
              {"interplanetary_net", "autonomous_swarms"},
              {},
              {{K::YieldMultiplier, 1.1}},
              "Instruments", 5, 5});

    tree.Add({"magnetic_sails",
              "Magnetic Sails",
              "Catch the solar wind with a magnetic net. Fishing, but for charged particles.",
              {{R, 217908071864437.5}},
              {"solar_sails"},
              {},
              {{K::YieldMultiplier, 1.05}},
              "Logistics", 5, 5});

    tree.Add({"thermal_shields",
              "Thermal Shields",
              "Fly closer to the Sun than anyone should. Please keep your hands inside the ship.",
              {{R, 217908071864437.5}},
              {"heat_resistant_alloys"},
              {},
              {{K::YieldMultiplier, 1.05}},
              "Materials", 5, 5});

    tree.Add({"mass_catchers",
              "Mass Catchers",
              "Big nets in orbit that catch incoming cargo. Baseball, but for asteroids.",
              {{R, 381339125762765.6}},
              {"asteroid_capture", "magnetic_sails"},
              {},
              {{K::YieldMultiplier, 1.1}},
              "Logistics", 5, 6});

    tree.Add({"mirror_phalanx",
              "Mirror Phalanx",
              "A formation of mirrors that all move at once. Tidy, shiny and slightly menacing.",
              {{R, 381339125762765.6}},
              {"heliostat_fields"},
              {},
              {{K::YieldMultiplier, 1.15}},
              "Optics", 5, 6});

    tree.Add({"antimatter_traps",
              "Antimatter Traps",
              "Hold the world's most dangerous fuel in a magnetic bottle. The bottle has a very long warranty.",
              {{R, 381339125762765.6}},
              {"gas_giant_scooping", "solar_power_satellites"},
              {},
              {{K::PassiveAdd, 120000000}},
              "Power", 5, 6});

    tree.Add({"belt_refineries",
              "Belt Refineries",
              "Process ore right in the asteroid belt. Saves the long commute to Earth.",
              {{R, 381339125762765.6}},
              {"orbital_smelters", "asteroid_capture"},
              {},
              {{K::PassiveAdd, 100000000}},
              "Industrial", 5, 6});

    tree.Add({"comet_harvesting",
              "Comet Harvesting",
              "Chase dirty snowballs for water and hydrogen. Perfect for fuel, awful for ice cream.",
              {{R, 667343470084839.9}},
              {"asteroid_capture", "gas_giant_scooping"},
              {},
              {{K::PassiveAdd, 180000000}},
              "Mining", 5, 7});

    tree.Add({"interplanetary_grid",
              "Interplanetary Grid",
              "One power network from Mercury to Mars. Sunshine delivered to your door, no matter the planet.",
              {{R, 667343470084839.9}},
              {"solar_power_satellites", "mass_catchers"},
              {},
              {{K::PassiveAdd, 220000000}},
              "Power", 5, 7});

    tree.Add({"planetary_telescopes",
              "Infrared Excess Survey",
              "Look for stars that glow warm in infrared: someone else's swarm. So far, nobody. Rude.",
              {{R, 667343470084839.9}},
              {"autonomous_prospectors", "mirror_phalanx"},
              {},
              {{K::YieldMultiplier, 1.15}},
              "Instruments", 5, 7});

    tree.Add({"orbital_shipyards",
              "Orbital Shipyards",
              "Where ships are built by robots that have never seen the sea. Launch parties are mandatory.",
              {{R, 667343470084839.9}},
              {"belt_refineries", "self_replicating_factories"},
              {},
              {{K::PassiveAdd, 200000000}},
              "Industrial", 5, 7});

    tree.Add({"venus_cloud_cities",
              "Venus Cloud Cities",
              "Floating cities above the acid clouds. The view is great, the weather is a joke.",
              {{R, 1167851072648469.8}},
              {"thermal_shields", "mass_catchers"},
              {},
              {{K::PassiveAdd, 350000000}},
              "Orbit", 5, 8});

    tree.Add({"light_sail_fleets",
              "Light Sail Fleets",
              "Hundreds of reflective ships sailing on pure photons. Wind is a thing of the past.",
              {{R, 1167851072648469.8}},
              {"magnetic_sails", "mirror_phalanx"},
              {},
              {{K::YieldMultiplier, 1.1}},
              "Logistics", 5, 8});

    tree.Add({"laser_launch_arrays",
              "Laser Launch Arrays",
              "Push spaceships with a giant laser. The sailors are the lucky ones.",
              {{R, 1167851072648469.8}},
              {"mirror_phalanx", "antimatter_traps"},
              {},
              {{K::YieldMultiplier, 1.2}},
              "Optics", 5, 8});

    tree.Add({"helium_refineries",
              "Helium Refineries",
              "Distill the lightest gases from the heaviest planets. Floating factories with a squeaky voice.",
              {{R, 1167851072648469.8}},
              {"gas_giant_scooping", "belt_refineries"},
              {},
              {{K::PassiveAdd, 300000000}},
              "Industrial", 5, 8});

    tree.Add({"antimatter_production",
              "Antimatter Production",
              "Make the stuff at scale instead of by the teaspoon. A very expensive thing to be careless with.",
              {{R, 2043739377134822}},
              {"antimatter_traps", "interplanetary_grid"},
              {},
              {{K::PassiveAdd, 600000000}},
              "Power", 5, 9});

    tree.Add({"stellar_surveys",
              "Stellar Surveys",
              "Chart every star nearby. Proxima Centauri is the closest one, 4.24 light-years out. Remember the name.",
              {{R, 2043739377134822}},
              {"planetary_telescopes"},
              {},
              {{K::YieldMultiplier, 1.1}},
              "Instruments", 5, 9});

    tree.Add({"gas_giant_stations",
              "Gas Giant Stations",
              "Floating refineries on the clouds of Jupiter. Open for business, closed for tourists.",
              {{R, 2043739377134822}},
              {"comet_harvesting", "helium_refineries"},
              {},
              {{K::PassiveAdd, 550000000}},
              "Mining", 5, 9});

    tree.Add({"solar_mirror_arrays",
              "Statite Prototype",
              "A mirror that hovers on sunlight instead of orbiting. It doesn't fall; it just refuses.",
              {{R, 2043739377134822}},
              {"solar_power_satellites", "light_sail_fleets", "laser_launch_arrays"},
              {},
              {{K::YieldMultiplier, 1.2}},
              "Solar", 5, 9});

    tree.Add({"fusion_drives",
              "Fusion Drives",
              "A rocket that never runs out of juice. The destination is now the only thing that matters.",
              {{R, 3576543909985938.5}},
              {"antimatter_production", "light_sail_fleets"},
              {},
              {{K::PassiveAdd, 1100000000}},
              "Power", 5, 10});

    tree.Add({"mass_relays",
              "Mass Relays",
              "Gates that fling cargo from planet to planet. Delivery times drop from months to afternoons.",
              {{R, 3576543909985938.5}},
              {"mass_catchers", "orbital_shipyards"},
              {},
              {{K::YieldMultiplier, 1.15}},
              "Logistics", 5, 10});

    tree.Add({"megastructure_design",
              "Megastructure Design",
              "Freeman Dyson's idea: not a solid shell, a swarm of collectors. Shells crack; swarms just multiply.",
              {{R, 3576543909985938.5}},
              {"orbital_shipyards", "helium_refineries"},
              {},
              {{K::PassiveAdd, 1000000000}},
              "Industrial", 5, 10});

    tree.Add({"power_collectors",
              "Pilot Collector Ring",
              "The first collectors meant to stay around the Sun. Small, cheap and very far from anything.",
              {{R, 6258951842475392}},
              {"interplanetary_grid", "solar_mirror_arrays", "megastructure_design"},
              {},
              {{K::PassiveAdd, 2200000000}},
              "Solar", 5, 11});

    tree.Add({"shipyard_networks",
              "Shipyard Networks",
              "Yards across the solar system trading parts like a swap meet. Warranty: wherever you find us.",
              {{R, 6258951842475392}},
              {"venus_cloud_cities", "mass_relays"},
              {},
              {{K::YieldMultiplier, 1.2}},
              "Orbit", 5, 11});

    tree.Add({"flare_harvesting",
              "Flare Harvesting",
              "Catch the Sun's tantrums in a net and sell them as electricity. Sunspots have a customer now.",
              {{R, 1.0953165724331936e+16}},
              {"power_collectors", "antimatter_production"},
              {},
              {{K::PassiveAdd, 4000000000}},
              "Power", 5, 12});

    tree.Add({"planetary_engineers",
              "Planetary Engineers",
              "People whose job is moving planets. Their first memo says: Mercury is mostly in the way.",
              {{R, 1.916804001758089e+16}},
              {"megastructure_design", "shipyard_networks"},
              {},
              {{K::PassiveAdd, 5000000000}},
              "Industrial", 5, 13});

    tree.Add({"interplanetary_ai",
              "Interplanetary AI",
              "A mind that runs a whole solar system's logistics. It has never asked for a raise.",
              {{R, 3.3544070030766556e+16}},
              {"stellar_surveys", "planetary_engineers"},
              {},
              {{K::YieldMultiplier, 1.25}},
              "Instruments", 5, 14});

    tree.Add({"lunar_gateway",
              "Lunar Gateway",
              "A station at the edge of the Earth's reach, pointed at the Moon. Opens the Moon.",
              {{R, 5.870212255384147e+16}},
              {"flare_harvesting", "planetary_engineers", "interplanetary_ai"},
              {},
              {{K::YieldMultiplier, 1.75}, {K::UnlockBody, 0, "moon"}},
              "Orbit", 5, 15});

    tree.Add({"lunar_survey_program",
              "Lunar Survey Program",
              "Map every crater and name the interesting ones. Naming rights are unbeatable.",
              {{R, 9.2772750655728e+16}},
              {"lunar_gateway"},
              {},
              {{K::YieldMultiplier, 1.1}},
              "Instruments", 6, 1});

    tree.Add({"regolith_excavation",
              "Regolith Excavation",
              "Dig up moon dust. It sticks to everything, including morale.",
              {{R, 1.716295887130968e+17}},
              {"lunar_survey_program"},
              {},
              {{K::PassiveAdd, 4000000000}},
              "Mining", 6, 2});

    tree.Add({"lunar_landers",
              "Lunar Landers",
              "Small ships that touch down on purpose. The word 'soft' is doing a lot of work.",
              {{R, 1.716295887130968e+17}},
              {"lunar_survey_program"},
              {},
              {{K::PassiveAdd, 4800000000}},
              "Orbit", 6, 2});

    tree.Add({"regolith_bricks",
              "Regolith Bricks",
              "Press moon dust into bricks. The first lunar building material, and the cheapest.",
              {{R, 1.716295887130968e+17}},
              {"lunar_survey_program"},
              {},
              {{K::YieldMultiplier, 1.05}},
              "Materials", 6, 2});

    tree.Add({"lunar_geology",
              "Lunar Geology",
              "Learn what the Moon is made of. Mostly rocks. Some surprise rocks.",
              {{R, 3.175147391192291e+17}},
              {"lunar_landers"},
              {},
              {{K::YieldMultiplier, 1.1}},
              "Instruments", 6, 3});

    tree.Add({"sintering_plants",
              "Sintering Plants",
              "Melt dust with focused sunlight until it behaves. Pottery, but industrial.",
              {{R, 3.175147391192291e+17}},
              {"regolith_excavation"},
              {},
              {{K::PassiveAdd, 9600000000}},
              "Industrial", 6, 3});

    tree.Add({"lunar_telescopes",
              "Lunar Coronagraph",
              "Hide the Sun behind a disc and watch its crown. No air, so the view is perfect.",
              {{R, 3.175147391192291e+17}},
              {"lunar_landers", "regolith_bricks"},
              {},
              {{K::YieldMultiplier, 1.1}},
              "Optics", 6, 3});

    tree.Add({"lunar_solar_arrays",
              "Lunar Solar Arrays",
              "Fourteen days of sun, fourteen of night. The panels hate the schedule.",
              {{R, 3.175147391192291e+17}},
              {"lunar_landers"},
              {},
              {{K::PassiveAdd, 11000000000}},
              "Power", 6, 3});

    tree.Add({"lunar_mining_rigs",
              "Lunar Mining Rigs",
              "Big digging machines for a place with no weather. Unlocks the Regolith Miner.",
              {{R, 5.874022673705738e+17}},
              {"lunar_geology", "sintering_plants"},
              {},
              {{K::YieldMultiplier, 1.1}},
              "Orbit", 6, 4});

    tree.Add({"lunar_rovers",
              "Lunar Rovers",
              "Buggies that roam the surface. Parking is free, forever.",
              {{R, 5.874022673705738e+17}},
              {"sintering_plants"},
              {},
              {{K::YieldMultiplier, 1.05}},
              "Logistics", 6, 4});

    tree.Add({"regolith_concrete",
              "Regolith Concrete",
              "Concrete made without water, because water costs more than gold up there.",
              {{R, 5.874022673705738e+17}},
              {"regolith_bricks", "lunar_telescopes"},
              {},
              {{K::PassiveAdd, 16000000000}},
              "Materials", 6, 4});

    tree.Add({"lunar_foundries",
              "Lunar Foundries",
              "Smelt moon rock into metal. Unlocks the Lunar Foundry.",
              {{R, 5.874022673705738e+17}},
              {"sintering_plants", "lunar_geology"},
              {},
              {{K::PassiveAdd, 18000000000}},
              "Industrial", 6, 4});

    tree.Add({"beamed_surface_power",
              "Beamed Surface Power",
              "Send power across the Moon with no wires. Wireless charging, for a whole continent.",
              {{R, 1.0866941946355617e+18}},
              {"lunar_solar_arrays", "lunar_telescopes"},
              {},
              {{K::PassiveAdd, 32000000000}},
              "Power", 6, 5});

    tree.Add({"robotic_foundry_crews",
              "Robotic Foundry Crews",
              "Robots run the furnaces. They don't mind the dust.",
              {{R, 1.0866941946355617e+18}},
              {"lunar_foundries"},
              {},
              {{K::PassiveAdd, 36000000000}},
              "Industrial", 6, 5});

    tree.Add({"far_side_observatory",
              "Far-Side Observatory",
              "The quietest place in the solar system. Earth's radio chatter can't reach it.",
              {{R, 1.0866941946355617e+18}},
              {"lunar_telescopes"},
              {},
              {{K::YieldMultiplier, 1.1}},
              "Optics", 6, 5});

    tree.Add({"polar_ice_mining",
              "Polar Ice Mining",
              "Ice in the dark craters means water, fuel and air. The Moon was hiding a pantry.",
              {{R, 1.0866941946355617e+18}},
              {"regolith_excavation", "lunar_rovers"},
              {},
              {{K::PassiveAdd, 28000000000}},
              "Mining", 6, 5});

    tree.Add({"moonquake_network",
              "Moonquake Network",
              "Listen to the Moon rumble. It's a surprisingly good conversationalist.",
              {{R, 2.010384260075789e+18}},
              {"lunar_mining_rigs", "lunar_geology"},
              {},
              {{K::YieldMultiplier, 1.15}},
              "Instruments", 6, 6});

    tree.Add({"lunar_orbit_stations",
              "Lunar Orbit Stations",
              "A pit stop between Earth and the surface. The lounge has a view.",
              {{R, 2.010384260075789e+18}},
              {"lunar_mining_rigs", "robotic_foundry_crews"},
              {},
              {{K::YieldMultiplier, 1.1}},
              "Orbit", 6, 6});

    tree.Add({"lunar_glass",
              "Thin-Film Mirrors",
              "Lunar glass with an aluminium coat thinner than a soap bubble. Light, cheap and very shiny.",
              {{R, 2.010384260075789e+18}},
              {"lunar_solar_arrays", "regolith_concrete"},
              {},
              {{K::YieldMultiplier, 1.1}},
              "Solar", 6, 6});

    tree.Add({"cislunar_shuttles",
              "Cislunar Shuttles",
              "Regular flights between Earth and Moon. The nearest commute in the galaxy.",
              {{R, 2.010384260075789e+18}},
              {"lunar_rovers", "beamed_surface_power"},
              {},
              {{K::PassiveAdd, 56000000000}},
              "Logistics", 6, 6});

    tree.Add({"helium3_extraction",
              "Helium-3 Extraction",
              "Rare gas baked into the dust by the Sun. Fusion plants dream of it.",
              {{R, 3.7192108811402107e+18}},
              {"beamed_surface_power", "far_side_observatory"},
              {},
              {{K::PassiveAdd, 96000000000}},
              "Power", 6, 7});

    tree.Add({"lunar_assembly_lines",
              "Lunar Assembly Lines",
              "Build collector panels on the Moon from Moon stuff. Shipping them sunward is a rounding error.",
              {{R, 3.7192108811402107e+18}},
              {"robotic_foundry_crews", "lunar_glass"},
              {},
              {{K::PassiveAdd, 100000000000}},
              "Industrial", 6, 7});

    tree.Add({"lunar_comms_relay",
              "Lunar Comms Relay",
              "A radio mast on the horizon. The ping time is mostly the speed of light apologizing.",
              {{R, 3.7192108811402107e+18}},
              {"moonquake_network"},
              {},
              {{K::YieldMultiplier, 1.15}},
              "Instruments", 6, 7});

    tree.Add({"regolith_shielding",
              "Regolith Shielding",
              "Pile dust on the roof and the radiation gives up. Cheapest armor ever.",
              {{R, 3.7192108811402107e+18}},
              {"lunar_glass", "polar_ice_mining"},
              {},
              {{K::YieldMultiplier, 1.05}},
              "Materials", 6, 7});

    tree.Add({"lagrange_stations",
              "Sun-Earth L1 Station",
              "Park a station where the Sun and the Earth pull the same. The best seat to watch the Sun.",
              {{R, 6.88054013010939e+18}},
              {"lunar_orbit_stations", "lunar_glass", "regolith_shielding"},
              {},
              {{K::YieldMultiplier, 1.15}},
              "Solar", 6, 8});

    tree.Add({"lunar_rail_network",
              "Lunar Rail Network",
              "Magnetic trains across the dust. Silent, because there's no air to carry the sound.",
              {{R, 6.88054013010939e+18}},
              {"cislunar_shuttles", "lunar_comms_relay"},
              {},
              {{K::YieldMultiplier, 1.1}},
              "Logistics", 6, 8});

    tree.Add({"laser_ranging",
              "Laser Ranging",
              "Bounce a laser off every collector. Now the swarm is measured to the millimeter.",
              {{R, 6.88054013010939e+18}},
              {"far_side_observatory", "lunar_comms_relay"},
              {},
              {{K::YieldMultiplier, 1.2}},
              "Optics", 6, 8});

    tree.Add({"lunar_domes",
              "Lunar Domes",
              "Pressurized domes with gardens inside. Gravity is a sixth, so everyone feels fit.",
              {{R, 6.88054013010939e+18}},
              {"lunar_assembly_lines", "polar_ice_mining"},
              {},
              {{K::PassiveAdd, 160000000000}},
              "Industrial", 6, 8});

    tree.Add({"lunar_fusion_plant",
              "Lunar Fusion Plant",
              "Burn helium-3 for clean power. The fuel was just lying there.",
              {{R, 1.2728999240702372e+19}},
              {"helium3_extraction", "laser_ranging"},
              {},
              {{K::PassiveAdd, 280000000000}},
              "Power", 6, 9});

    tree.Add({"lunar_control_ai",
              "Lunar Control AI",
              "A mind that runs the whole base. It likes the quiet.",
              {{R, 1.2728999240702372e+19}},
              {"lunar_comms_relay", "lunar_rail_network"},
              {},
              {{K::YieldMultiplier, 1.1}},
              "Instruments", 6, 9});

    tree.Add({"lunar_heavy_industry",
              "Lunar Heavy Industry",
              "Smelters, presses and mills in one-sixth gravity. Everything weighs less, even excuses.",
              {{R, 1.2728999240702372e+19}},
              {"lunar_domes"},
              {},
              {{K::PassiveAdd, 320000000000}},
              "Industrial", 6, 9});

    tree.Add({"deep_regolith_mining",
              "Deep Regolith Mining",
              "Dig deeper for richer rock. The Moon starts to look like Swiss cheese.",
              {{R, 1.2728999240702372e+19}},
              {"polar_ice_mining", "lunar_domes"},
              {},
              {{K::PassiveAdd, 260000000000}},
              "Mining", 6, 9});

    tree.Add({"earth_moon_ferry",
              "Earth-Moon Ferry",
              "A scheduled service with snacks. The cheap seat has a very nice window.",
              {{R, 2.354864859529939e+19}},
              {"lagrange_stations", "lunar_heavy_industry"},
              {},
              {{K::PassiveAdd, 520000000000}},
              "Orbit", 6, 10});

    tree.Add({"self_sealing_habitats",
              "Self-Sealing Habitats",
              "Walls that patch their own holes. Peace of mind, built in.",
              {{R, 2.354864859529939e+19}},
              {"lunar_control_ai"},
              {},
              {{K::YieldMultiplier, 1.15}},
              "Instruments", 6, 10});

    tree.Add({"lunar_solar_belt",
              "Lunar Solar Belt",
              "A ring of panels around the lunar equator. The Moon wears a power belt now.",
              {{R, 2.354864859529939e+19}},
              {"lunar_fusion_plant", "laser_ranging"},
              {},
              {{K::PassiveAdd, 600000000000}},
              "Power", 6, 10});

    tree.Add({"lunar_settlement_ring",
              "Lunar Settlement Ring",
              "Towns linked around the Moon. The first place with its own time zone.",
              {{R, 4.3564999901303865e+19}},
              {"earth_moon_ferry", "self_sealing_habitats"},
              {},
              {{K::YieldMultiplier, 1.2}},
              "Orbit", 6, 11});

    tree.Add({"lunar_power_grid",
              "Lunar Power Grid",
              "One grid for the whole Moon. A blackout now hits an entire world.",
              {{R, 4.3564999901303865e+19}},
              {"lunar_solar_belt", "cislunar_shuttles"},
              {},
              {{K::PassiveAdd, 1000000000000}},
              "Power", 6, 11});

    tree.Add({"lunar_megabase",
              "Lunar Megabase",
              "Every dome and depot, joined up. The address is Moon, Moon.",
              {{R, 8.059524981741216e+19}},
              {"lunar_settlement_ring", "deep_regolith_mining"},
              {},
              {{K::PassiveAdd, 1600000000000}},
              "Industrial", 6, 12});

    tree.Add({"lunar_energy_routing",
              "Lunar Energy Routing",
              "Decide who gets how much sunlight, and when. The most important spreadsheet on the Moon.",
              {{R, 1.491012121622125e+20}},
              {"lunar_power_grid", "lunar_megabase"},
              {},
              {{K::YieldMultiplier, 1.25}},
              "Power", 6, 13});

    tree.Add({"lunar_settlement",
              "Lunar Settlement",
              "A real, living, complaining town on the Moon. Neighbors already argue about parking.",
              {{R, 2.7583724250009315e+20}},
              {"lunar_settlement_ring", "lunar_megabase", "lunar_energy_routing"},
              {},
              {{K::YieldMultiplier, 1.75}},
              "Orbit", 6, 14});

    tree.Add({"lunar_master_plan",
              "Lunar Master Plan",
              "A plan for the whole Moon, with a color-coded map. Somebody loves a map.",
              {{R, 4.897473507115881e+20}},
              {"lunar_settlement"},
              {},
              {{K::YieldMultiplier, 1.1}},
              "Instruments", 7, 1});

    tree.Add({"regolith_refining",
              "Regolith Refining",
              "Turn dust into oxygen, metals and silicon. The Moon is a very slow vending machine.",
              {{R, 9.305199663520173e+20}},
              {"lunar_master_plan"},
              {},
              {{K::PassiveAdd, 2000000000000}},
              "Mining", 7, 2});

    tree.Add({"lunar_reactors",
              "Lunar Reactors",
              "Compact reactors that hum through the long night. Cozy, in a nuclear way.",
              {{R, 9.305199663520173e+20}},
              {"lunar_master_plan"},
              {},
              {{K::PassiveAdd, 2400000000000}},
              "Power", 7, 2});

    tree.Add({"lunar_alloys",
              "Lunar Alloys",
              "Metals mixed from what the Moon offers. Surprisingly good, mildly gray.",
              {{R, 9.305199663520173e+20}},
              {"lunar_master_plan"},
              {},
              {{K::YieldMultiplier, 1.05}},
              "Materials", 7, 2});

    tree.Add({"orbit_modelling",
              "Swarm Orbit Modelling",
              "Plan thousands of orbits that never cross. A very tidy kind of paranoia.",
              {{R, 1.7679879360688329e+21}},
              {"lunar_master_plan", "lunar_reactors"},
              {},
              {{K::YieldMultiplier, 1.1}},
              "Solar", 7, 3});

    tree.Add({"lunar_fuel_depots",
              "Lunar Fuel Depots",
              "Tanks of moon-made fuel. The cheapest gas station in the solar system.",
              {{R, 1.7679879360688329e+21}},
              {"regolith_refining"},
              {},
              {{K::YieldMultiplier, 1.05}},
              "Logistics", 7, 3});

    tree.Add({"mass_driver_design",
              "Mass Driver Design",
              "Plans for a magnetic track that throws panels off the Moon. The throwing is the easy part.",
              {{R, 1.7679879360688329e+21}},
              {"regolith_refining", "lunar_alloys"},
              {},
              {{K::PassiveAdd, 4800000000000}},
              "Industrial", 7, 3});

    tree.Add({"surface_laser_arrays",
              "Surface Laser Arrays",
              "Lasers on the lunar plains. They point up, which is the polite direction.",
              {{R, 1.7679879360688329e+21}},
              {"lunar_reactors"},
              {},
              {{K::YieldMultiplier, 1.1}},
              "Optics", 7, 3});

    tree.Add({"lunar_mass_drivers",
              "Lunar Mass Drivers",
              "Long magnetic tracks that fling collector panels sunward. Unlocks the Mass Driver.",
              {{R, 3.3591770785307825e+21}},
              {"mass_driver_design", "lunar_fuel_depots"},
              {},
              {{K::PassiveAdd, 9000000000000}},
              "Industrial", 7, 4});

    tree.Add({"lunar_heat_pipes",
              "Lunar Heat Pipes",
              "Move heat from the day side to the night side. The Moon finally shares.",
              {{R, 3.3591770785307825e+21}},
              {"lunar_reactors", "surface_laser_arrays"},
              {},
              {{K::PassiveAdd, 8000000000000}},
              "Power", 7, 4});

    tree.Add({"sintered_metals",
              "Sintered Metals",
              "Strong metal parts made from powder and patience.",
              {{R, 3.3591770785307825e+21}},
              {"lunar_alloys"},
              {},
              {{K::YieldMultiplier, 1.1}},
              "Materials", 7, 4});

    tree.Add({"volatile_harvesting",
              "Volatile Harvesting",
              "Collect gases that were frozen in the dark. The Moon was saving them.",
              {{R, 3.3591770785307825e+21}},
              {"regolith_refining", "lunar_fuel_depots"},
              {},
              {{K::PassiveAdd, 7000000000000}},
              "Mining", 7, 4});

    tree.Add({"sustained_lunar_fusion",
              "Sustained Lunar Fusion",
              "A plant that keeps burning with no fuss. It never asks for a day off.",
              {{R, 6.382436449208487e+21}},
              {"lunar_heat_pipes", "orbit_modelling"},
              {},
              {{K::YieldMultiplier, 1.1}},
              "Power", 7, 5});

    tree.Add({"lunar_solar_rings",
              "Lunar Solar Rings",
              "Rings of panels around the Moon, sending power outward. Unlocks the Lunar Solar Farm.",
              {{R, 6.382436449208487e+21}},
              {"surface_laser_arrays", "sintered_metals"},
              {},
              {{K::YieldMultiplier, 1.1}},
              "Optics", 7, 5});

    tree.Add({"deep_vein_mining",
              "Deep-Vein Mining",
              "Follow rich veins into the dark. Gold rushes are quiet in a vacuum.",
              {{R, 6.382436449208487e+21}},
              {"volatile_harvesting", "lunar_mass_drivers"},
              {},
              {{K::PassiveAdd, 16000000000000}},
              "Mining", 7, 5});

    tree.Add({"lunar_simulation",
              "Lunar Simulation",
              "Run the Moon in a computer first. The simulated Moon is much less dusty.",
              {{R, 6.382436449208487e+21}},
              {"orbit_modelling"},
              {},
              {{K::YieldMultiplier, 1.15}},
              "Instruments", 7, 5});

    tree.Add({"cislunar_highway",
              "Cislunar Highway",
              "Fixed lanes between Earth and Moon. Traffic is two ships and a probe.",
              {{R, 1.2126629253496122e+22}},
              {"deep_vein_mining", "lunar_heat_pipes"},
              {},
              {{K::PassiveAdd, 28000000000000}},
              "Orbit", 7, 6});

    tree.Add({"underground_labs",
              "Underground Labs",
              "Quiet caves for sensitive experiments. Whispering is optional.",
              {{R, 1.2126629253496122e+22}},
              {"lunar_simulation"},
              {},
              {{K::YieldMultiplier, 1.1}},
              "Instruments", 7, 6});

    tree.Add({"magnetic_shielding",
              "Magnetic Shielding",
              "Bend radiation around the base. The base has opinions about it.",
              {{R, 1.2126629253496122e+22}},
              {"sustained_lunar_fusion", "sintered_metals"},
              {},
              {{K::PassiveAdd, 24000000000000}},
              "Power", 7, 6});

    tree.Add({"lunar_shipyards",
              "Lunar Shipyards",
              "Build sail-tugs where gravity is weak. They carry the panels the last mile to the Sun.",
              {{R, 1.2126629253496122e+22}},
              {"lunar_mass_drivers", "deep_vein_mining"},
              {},
              {{K::PassiveAdd, 32000000000000}},
              "Industrial", 7, 6});

    tree.Add({"earth_return_cargo",
              "Earth-Return Cargo",
              "Ship moon metals home. Cargo cult, but real.",
              {{R, 2.304059558164263e+22}},
              {"cislunar_highway", "lunar_fuel_depots"},
              {},
              {{K::YieldMultiplier, 1.1}},
              "Logistics", 7, 7});

    tree.Add({"reactor_stabilizers",
              "Reactor Stabilizers",
              "Steady hands for a hot core. The coffee is still cold, on purpose.",
              {{R, 2.304059558164263e+22}},
              {"magnetic_shielding", "sustained_lunar_fusion"},
              {},
              {{K::PassiveAdd, 48000000000000}},
              "Power", 7, 7});

    tree.Add({"ore_trimming",
              "Ore Trimming",
              "Take just the good part. Waste not, want not, wait nothing.",
              {{R, 2.304059558164263e+22}},
              {"deep_vein_mining", "lunar_shipyards"},
              {},
              {{K::PassiveAdd, 44000000000000}},
              "Mining", 7, 7});

    tree.Add({"radiation_hardening_lunar",
              "Radiation Hardening",
              "Materials that shrug at the harshest light. Sunscreen with a PhD.",
              {{R, 2.304059558164263e+22}},
              {"sintered_metals", "magnetic_shielding"},
              {},
              {{K::YieldMultiplier, 1.1}},
              "Materials", 7, 7});

    tree.Add({"power_export",
              "Power Export",
              "Beam surplus power home. Earth's electric bill drops, politely.",
              {{R, 4.3777131605121e+22}},
              {"reactor_stabilizers"},
              {},
              {{K::PassiveAdd, 80000000000000}},
              "Power", 7, 8});

    tree.Add({"spectral_tuning_lunar",
              "Spectral Tuning",
              "Choose the exact color of the beam. Warm white tests best.",
              {{R, 4.3777131605121e+22}},
              {"lunar_solar_rings", "underground_labs"},
              {},
              {{K::YieldMultiplier, 1.2}},
              "Optics", 7, 8});

    tree.Add({"lunar_industrial_parks",
              "Lunar Industrial Parks",
              "Rows of factories under domes. The parking lot is the whole Moon.",
              {{R, 4.3777131605121e+22}},
              {"lunar_shipyards", "ore_trimming"},
              {},
              {{K::PassiveAdd, 90000000000000}},
              "Industrial", 7, 8});

    tree.Add({"lunar_navigation",
              "Pulsar Navigation",
              "Each collector reads the ticking of dead stars to know where it is. A GPS for the Sun.",
              {{R, 4.3777131605121e+22}},
              {"underground_labs", "earth_return_cargo"},
              {},
              {{K::YieldMultiplier, 1.1}},
              "Instruments", 7, 8});

    tree.Add({"logistics_hubs",
              "Logistics Hubs",
              "A warehouse in orbit and another on the ground. They argue about inventory.",
              {{R, 8.31765500497299e+22}},
              {"earth_return_cargo", "lunar_industrial_parks"},
              {},
              {{K::YieldMultiplier, 1.15}},
              "Logistics", 7, 9});

    tree.Add({"flare_forecasting",
              "Flare Forecasting",
              "Warn the base before the Sun sneezes. A weather report for solar storms.",
              {{R, 8.31765500497299e+22}},
              {"power_export", "radiation_hardening_lunar"},
              {},
              {{K::PassiveAdd, 140000000000000}},
              "Power", 7, 9});

    tree.Add({"earth_moon_elevator",
              "Earth-Moon Elevator",
              "A cable between two worlds. Long, thin and optimistic.",
              {{R, 8.31765500497299e+22}},
              {"power_export", "lunar_industrial_parks"},
              {},
              {{K::PassiveAdd, 150000000000000}},
              "Orbit", 7, 9});

    tree.Add({"beam_calibration",
              "Phased-Array Beaming",
              "A thousand collectors in step act as one antenna. The beam lands on a coin.",
              {{R, 8.31765500497299e+22}},
              {"spectral_tuning_lunar"},
              {},
              {{K::YieldMultiplier, 1.15}},
              "Optics", 7, 9});

    tree.Add({"first_export",
              "First Swarm Power",
              "The first watts from the swarm reach the Earth. Everyone claps politely.",
              {{R, 1.580354450944868e+23}},
              {"lunar_mass_drivers", "flare_forecasting", "beam_calibration"},
              {},
              {{K::PassiveAdd, 260000000000000}},
              "Solar", 7, 10});

    tree.Add({"lunar_composites",
              "Lunar Composites",
              "Materials made of dust and optimism.",
              {{R, 1.580354450944868e+23}},
              {"radiation_hardening_lunar", "earth_moon_elevator"},
              {},
              {{K::YieldMultiplier, 1.1}},
              "Materials", 7, 10});

    tree.Add({"base_autonomy",
              "Base Autonomy",
              "The base runs itself. Humans are invited to watch from a distance.",
              {{R, 1.580354450944868e+23}},
              {"lunar_navigation", "beam_calibration"},
              {},
              {{K::YieldMultiplier, 1.15}},
              "Instruments", 7, 10});

    tree.Add({"lunar_power_network",
              "Lunar Power Network",
              "Every plant on one network. One switch to rule them all.",
              {{R, 3.0026734567952492e+23}},
              {"first_export", "lunar_composites"},
              {},
              {{K::PassiveAdd, 500000000000000}},
              "Power", 7, 11});

    tree.Add({"factory_federation",
              "Factory Federation",
              "Factories trading parts like old friends. Efficiency has never looked so noisy.",
              {{R, 3.0026734567952492e+23}},
              {"lunar_industrial_parks", "logistics_hubs"},
              {},
              {{K::PassiveAdd, 440000000000000}},
              "Industrial", 7, 11});

    tree.Add({"moon_orbit_tuning",
              "Moon Orbit Tuning",
              "Nudge the Moon's traffic into tidy orbits. Parking a world is a two-person job.",
              {{R, 5.7050795679109726e+23}},
              {"lunar_power_network", "factory_federation"},
              {},
              {{K::YieldMultiplier, 1.2}},
              "Orbit", 7, 12});

    tree.Add({"export_balancing",
              "Inner Swarm Ring",
              "The lunar panels settle at Mercury's distance. Seven times the sunlight, same paperwork.",
              {{R, 1.0839651179030848e+24}},
              {"lunar_navigation", "first_export", "base_autonomy"},
              {},
              {{K::YieldMultiplier, 1.25}},
              "Solar", 7, 13});

    tree.Add({"lunar_industrial_complex",
              "Lunar Industrial Complex",
              "The whole Moon is now a workplace. The Moon was not consulted. Opens the Sun.",
              {{R, 2.059533724015861e+24}},
              {"lunar_power_network", "moon_orbit_tuning", "export_balancing"},
              {},
              {{K::YieldMultiplier, 1.75}, {K::UnlockBody, 0, "sun"}},
              "Orbit", 7, 14});

    tree.Add({"solar_charter",
              "Solar Charter",
              "A treaty about who gets to poke the Sun. Everyone signs, nobody reads.",
              {{R, 3.0956930038479487e+24}},
              {"lunar_industrial_complex"},
              {},
              {{K::YieldMultiplier, 1.1}},
              "Logistics", 8, 1});

    tree.Add({"solar_monitoring",
              "Helioseismology",
              "Listen to the Sun ring like a bell. It hums every five minutes.",
              {{R, 5.881816707311102e+24}},
              {"solar_charter"},
              {},
              {{K::PassiveAdd, 500000000000000}},
              "Instruments", 8, 2});

    tree.Add({"solar_telescopes",
              "Solar Telescopes",
              "Look at the Sun without going blind. Filters help.",
              {{R, 5.881816707311102e+24}},
              {"solar_charter"},
              {},
              {{K::PassiveAdd, 600000000000000}},
              "Optics", 8, 2});

    tree.Add({"heat_shield_materials",
              "Heat-Shield Materials",
              "Stuff that laughs at heat. Great for probes, bad for lunch.",
              {{R, 5.881816707311102e+24}},
              {"solar_charter"},
              {},
              {{K::YieldMultiplier, 1.05}},
              "Materials", 8, 2});

    tree.Add({"solar_forges",
              "Solar Forges",
              "Melt metal with pure sunlight. Free fuel, free heat, free mild sunburn.",
              {{R, 1.1175451743891095e+25}},
              {"heat_shield_materials"},
              {},
              {{K::PassiveAdd, 1200000000000000}},
              "Industrial", 8, 3});

    tree.Add({"close_orbit_arrays",
              "Close-Orbit Arrays",
              "Panels that sit closer to the Sun than anyone should.",
              {{R, 1.1175451743891095e+25}},
              {"solar_telescopes"},
              {},
              {{K::PassiveAdd, 1400000000000000}},
              "Solar", 8, 3});

    tree.Add({"flux_instruments",
              "Flux Instruments",
              "Sensors for light so bright it has its own weather.",
              {{R, 1.1175451743891095e+25}},
              {"solar_monitoring"},
              {},
              {{K::YieldMultiplier, 1.1}},
              "Instruments", 8, 3});

    tree.Add({"solar_wind_harvest",
              "Solar Wind Harvest",
              "Catch the Sun's breath with a magnetic loop. Free power, mildly radioactive.",
              {{R, 1.1175451743891095e+25}},
              {"solar_monitoring"},
              {},
              {{K::YieldMultiplier, 1.05}},
              "Mining", 8, 3});

    tree.Add({"solar_probes",
              "Solar Probes",
              "Little ships that fly toward the Sun on purpose. Unlocks the Solar Probe.",
              {{R, 2.123335831339308e+25}},
              {"solar_forges", "close_orbit_arrays"},
              {},
              {{K::PassiveAdd, 2200000000000000}},
              "Orbit", 8, 4});

    tree.Add({"coronal_power",
              "Coronal Power",
              "Use the Sun's outer layer as a power source. It's already hot.",
              {{R, 2.123335831339308e+25}},
              {"close_orbit_arrays", "heat_shield_materials"},
              {},
              {{K::PassiveAdd, 2000000000000000}},
              "Power", 8, 4});

    tree.Add({"corona_materials",
              "Corona Materials",
              "Alloys that survive where steel gives up.",
              {{R, 2.123335831339308e+25}},
              {"heat_shield_materials"},
              {},
              {{K::YieldMultiplier, 1.1}},
              "Materials", 8, 4});

    tree.Add({"sun_logistics",
              "Sun Logistics",
              "Deliveries to a place that melts the truck.",
              {{R, 2.123335831339308e+25}},
              {"flux_instruments", "solar_wind_harvest"},
              {},
              {{K::YieldMultiplier, 1.1}},
              "Logistics", 8, 4});

    tree.Add({"probe_swarms",
              "Probe Swarms",
              "Send a thousand probes so one gets through. Optimism, scaled.",
              {{R, 4.034338079544685e+25}},
              {"solar_probes", "flux_instruments"},
              {},
              {{K::YieldMultiplier, 1.1}},
              "Orbit", 8, 5});

    tree.Add({"magnetic_field_models",
              "Magnetic Field Models",
              "Understand the Sun's moods. They are many.",
              {{R, 4.034338079544685e+25}},
              {"flux_instruments", "corona_materials"},
              {},
              {{K::YieldMultiplier, 1.15}},
              "Instruments", 8, 5});

    tree.Add({"thermal_composites",
              "Waste-Heat Radiators",
              "Every collector glows warm on its back. Physics always sends the bill.",
              {{R, 4.034338079544685e+25}},
              {"corona_materials"},
              {},
              {{K::PassiveAdd, 4000000000000000}},
              "Materials", 8, 5});

    tree.Add({"sun_mirrors",
              "Sun Mirrors",
              "Giant mirrors that steer sunlight. The Sun gets to be a flashlight.",
              {{R, 4.034338079544685e+25}},
              {"solar_telescopes", "sun_logistics"},
              {},
              {{K::YieldMultiplier, 1.1}},
              "Optics", 8, 5});

    tree.Add({"solar_shipyards",
              "Solar Shipyards",
              "Build ships near the Sun. Quick tan, no refunds.",
              {{R, 7.6652423511348995e+25}},
              {"solar_forges", "coronal_power"},
              {},
              {{K::PassiveAdd, 7000000000000000}},
              "Industrial", 8, 6});

    tree.Add({"stellar_instruments",
              "Stellar Instruments",
              "Measure every flicker of the Sun.",
              {{R, 7.6652423511348995e+25}},
              {"sun_logistics", "magnetic_field_models"},
              {},
              {{K::YieldMultiplier, 1.1}},
              "Instruments", 8, 6});

    tree.Add({"solar_fusion_study",
              "Fusion Study",
              "Learn how the Sun does it. Spoiler: very big and very patient.",
              {{R, 7.6652423511348995e+25}},
              {"coronal_power", "corona_materials"},
              {},
              {{K::PassiveAdd, 6000000000000000}},
              "Power", 8, 6});

    tree.Add({"orbit_corridors",
              "Orbit Corridors",
              "Lanes where no two collectors ever meet. Mostly safe.",
              {{R, 7.6652423511348995e+25}},
              {"sun_logistics", "sun_mirrors"},
              {},
              {{K::PassiveAdd, 8000000000000000}},
              "Logistics", 8, 6});

    tree.Add({"solar_collector_swarm",
              "Solar Collector Swarm",
              "The swarm proper: a cloud of collectors that take sunlight home. Unlocks the Dyson Collector.",
              {{R, 1.4563960467156309e+26}},
              {"close_orbit_arrays", "magnetic_field_models", "thermal_composites"},
              {},
              {{K::PassiveAdd, 1.1e+16}},
              "Solar", 8, 7});

    tree.Add({"flare_models",
              "Flare Models",
              "Predict the Sun's tantrums. The Sun disagrees with the forecast.",
              {{R, 1.4563960467156309e+26}},
              {"probe_swarms", "stellar_instruments"},
              {},
              {{K::YieldMultiplier, 1.15}},
              "Instruments", 8, 7});

    tree.Add({"solar_foundries",
              "Solar Foundries",
              "Smelt anything with sunlight. Fuel is free.",
              {{R, 1.4563960467156309e+26}},
              {"solar_shipyards", "thermal_composites"},
              {},
              {{K::PassiveAdd, 1.2e+16}},
              "Industrial", 8, 7});

    tree.Add({"focusing_arrays",
              "Focusing Arrays",
              "Concentrate a star's glow into one spot. Don't stand there.",
              {{R, 1.4563960467156309e+26}},
              {"sun_mirrors", "orbit_corridors"},
              {},
              {{K::YieldMultiplier, 1.1}},
              "Optics", 8, 7});

    tree.Add({"reactor_study",
              "Reactor Study",
              "Copy the Sun's reactor, scaled down a few billion times.",
              {{R, 2.7671524887596985e+26}},
              {"solar_fusion_study", "solar_shipyards"},
              {},
              {{K::PassiveAdd, 2e+16}},
              "Power", 8, 8});

    tree.Add({"stellar_ai",
              "Stellar AI",
              "A mind that handles the whole star-watching business.",
              {{R, 2.7671524887596985e+26}},
              {"flare_models"},
              {},
              {{K::YieldMultiplier, 1.1}},
              "Instruments", 8, 8});

    tree.Add({"sun_proof_alloys",
              "Sun-Proof Alloys",
              "Metals that survive the near Sun. Mostly smug.",
              {{R, 2.7671524887596985e+26}},
              {"thermal_composites", "solar_foundries"},
              {},
              {{K::YieldMultiplier, 1.1}},
              "Materials", 8, 8});

    tree.Add({"solar_trade_routes",
              "Solar Trade Routes",
              "Delivery lanes from the Sun to everywhere.",
              {{R, 2.7671524887596985e+26}},
              {"solar_collector_swarm", "orbit_corridors"},
              {},
              {{K::YieldMultiplier, 1.15}},
              "Logistics", 8, 8});

    tree.Add({"solar_construction",
              "Mercury Disassembly",
              "Take the smallest planet apart, one bucket at a time. It wasn't using all of itself.",
              {{R, 5.257589728643427e+26}},
              {"solar_foundries", "sun_proof_alloys"},
              {},
              {{K::PassiveAdd, 3.5e+16}},
              "Industrial", 8, 9});

    tree.Add({"collector_ring",
              "Dyson Ring",
              "A closed ring of collectors around the Sun's equator. The Sun wears its first belt.",
              {{R, 5.257589728643427e+26}},
              {"orbit_corridors", "solar_collector_swarm"},
              {},
              {{K::PassiveAdd, 3.8e+16}},
              "Solar", 8, 9});

    tree.Add({"mirror_swarms",
              "Dyson Bubble",
              "Statites hover over the poles, held up by light alone. They never orbit and never collide.",
              {{R, 5.257589728643427e+26}},
              {"solar_collector_swarm", "focusing_arrays"},
              {},
              {{K::YieldMultiplier, 1.2}},
              "Solar", 8, 9});

    tree.Add({"close_orbit_insertion",
              "Close Orbit Insertion",
              "Slow down near the Sun without becoming a snack.",
              {{R, 5.257589728643427e+26}},
              {"flare_models", "stellar_ai"},
              {},
              {{K::PassiveAdd, 3.2e+16}},
              "Orbit", 8, 9});

    tree.Add({"solar_assembly",
              "Tilted Rings",
              "More rings, each at its own radius and tilt. The Sun looks like a gyroscope.",
              {{R, 9.98942048442251e+26}},
              {"solar_construction", "collector_ring", "close_orbit_insertion"},
              {},
              {{K::PassiveAdd, 6.5e+16}},
              "Solar", 8, 10});

    tree.Add({"collector_cargo",
              "Collector Cargo",
              "Move collectors around like furniture.",
              {{R, 9.98942048442251e+26}},
              {"solar_trade_routes", "mirror_swarms"},
              {},
              {{K::YieldMultiplier, 1.15}},
              "Logistics", 8, 10});

    tree.Add({"solar_systems_ai",
              "Solar Systems AI",
              "A mind that steers a billion collectors. It has strong opinions about traffic.",
              {{R, 9.98942048442251e+26}},
              {"stellar_ai", "collector_ring"},
              {},
              {{K::YieldMultiplier, 1.15}},
              "Instruments", 8, 10});

    tree.Add({"sun_landing_rig",
              "Star Lifting",
              "Pull hydrogen off the Sun with magnetic fields. The Sun has plenty, and we'll need it later.",
              {{R, 1.8979898920402773e+27}},
              {"magnetic_field_models", "reactor_study", "collector_cargo"},
              {},
              {{K::PassiveAdd, 1.2e+17}},
              "Power", 8, 11});

    tree.Add({"solar_power_hub",
              "Solar Power Hub",
              "Panels, mirrors and generators in one place. Lights on.",
              {{R, 1.8979898920402773e+27}},
              {"reactor_study", "mirror_swarms"},
              {},
              {{K::PassiveAdd, 1.1e+17}},
              "Power", 8, 11});

    tree.Add({"solar_networks",
              "Swarm Coverage",
              "So many collectors that the Sun looks dotted. From far away it is getting dimmer.",
              {{R, 3.606180794876526e+27}},
              {"mirror_swarms", "solar_assembly", "solar_power_hub"},
              {},
              {{K::YieldMultiplier, 1.2}},
              "Solar", 8, 12});

    tree.Add({"solar_gate_prep",
              "Proxima Alignment",
              "Point the swarm at Proxima Centauri and find the quiet spot in between. Ominous music optional.",
              {{R, 6.8517435102654e+27}},
              {"solar_networks", "solar_systems_ai"},
              {},
              {{K::YieldMultiplier, 1.25}},
              "Orbit", 8, 13});

    tree.Add({"solar_foothold",
              "Kardashev Threshold",
              "Most of the Sun's light now works for us. The last step for now.",
              {{R, 1.301831266950426e+28}},
              {"sun_landing_rig", "solar_networks", "solar_gate_prep"},
              {},
              {{K::YieldMultiplier, 1.75}},
              "Orbit", 8, 14});
}

inline void RegisterGeneratedInteractions(InteractionSet& set) {
    set.Add({"gather", "Gather Data", "Click the Earth to collect research points.",
             InteractionTarget::Earth, 1, 0, true});
    set.Add({"solar_collector", "Solar Collection", "Click the Sun to collect solar energy. Slow to recharge, but worth more than a click on the Earth.",
             InteractionTarget::Sun, 8, 5, false});
    set.Add({"laser_pulse", "Laser Pulse", "Fire a high-power laser pulse from the HUD. Long recharge, large yield.",
             InteractionTarget::Button, 60, 12, false});
}

inline void RegisterGeneratedInstallations(InstallationSet& set) {
    using L = InstallationLook;
    set.Add({"weather_station", "Weather Station", "Measures the weather, mostly to confirm it's weather. Cheap, tireless and a little smug.",
             "instrumentation", 5, 0.02, 1.15, 40,
             L::Surface, 0xE6F0FF, 1, BodyId::Earth});
    set.Add({"satellite", "Satellite", "Goes around the planet all day, collecting points. Never asks for a day off.",
             "sounding_rockets", 15, 0.05, 1.15, 60,
             L::Orbit, 0xAAC8FF, 1, BodyId::Earth});
    set.Add({"steam_plant", "Steam Plant", "A big kettle with ambitions. Smells like progress.",
             "steam_generators", 100, 0.35, 1.15, 40,
             L::Surface, 0xFFBE5A, 1, BodyId::Earth});
    set.Add({"solar_farm", "Solar Farm", "Acres of mirrors pretending to be a lake. The birds are confused.",
             "solar_panels", 900, 2, 1.15, 40,
             L::Surface, 0xFFE678, 1, BodyId::Earth});
    set.Add({"laser_relay", "Laser Relay", "Passes laser light from one satellite to the next. Gossip travels at light speed too.",
             "relay_optics", 2900, 6, 1.15, 30,
             L::Orbit, 0xFF5A6E, 2, BodyId::Earth});
    set.Add({"ground_station", "Optical Ground Station", "Points a big telescope at the sky and fires. The neighbors have filed complaints.",
             "adaptive_optics", 15900, 30, 1.15, 30,
             L::Surface, 0xFF788C, 2, BodyId::Earth});
    set.Add({"laser_array", "Industrial Laser Array", "Hundreds of lasers working in step. The whole grid hums.",
             "laser_arrays", 30000, 60, 1.15, 30,
             L::Surface, 0xFF465A, 3, BodyId::Earth});
    set.Add({"smart_city", "Smart City", "Every streetlight reports to the cloud. The cloud is very proud.",
             "smart_meters", 165000, 300, 1.15, 30,
             L::Surface, 0xFFD278, 3, BodyId::Earth});
    set.Add({"orbital_mirror", "Orbital Mirror", "A mirror in orbit. Seven years of bad luck is a rounding error at this scale.",
             "thin_film_reflectors", 980000, 1620, 1.15, 24,
             L::Orbit, 0xD2E6FF, 4, BodyId::Earth});
    set.Add({"space_elevator", "Space Elevator", "Takes the stairs to orbit. Express lane, no turbulence.",
             "elevator_tether", 5820000, 8750, 1.15, 12,
             L::Orbit, 0xC8C8D7, 4, BodyId::Earth});
    set.Add({"mercury_mirrors", "Mercury Mirror Farm", "It's hot, it's close, and nobody will check on it. Perfect.",
             "heliostat_fields", 34500000, 47200, 1.15, 24,
             L::Sun, 0xFFBE6E, 5, BodyId::Earth});
    set.Add({"asteroid_miner", "Asteroid Miner", "Hits a rock with a bigger rock. Profits follow.",
             "asteroid_capture", 205000000, 255000, 1.15, 24,
             L::Far, 0xBEAA96, 5, BodyId::Earth});
    set.Add({"regolith_miner", "Regolith Miner", "Digs moon dust all day. The dust gets everywhere.",
             "lunar_mining_rigs", 1220000000, 1380000, 1.15, 20,
             L::Surface, 0x8FA3C2, 6, BodyId::Moon});
    set.Add({"lunar_foundry", "Lunar Foundry", "Turns dust into metal, and metal into more foundries.",
             "lunar_foundries", 7240000000, 7440000, 1.15, 12,
             L::Surface, 0xFF9646, 6, BodyId::Moon});
    set.Add({"mass_driver", "Mass Driver", "Throws collector panels off the Moon, sunward. Aim is optional, momentum is not.",
             "lunar_mass_drivers", 43000000000, 40200000, 1.15, 3,
             L::Far, 0xFFF0AA, 7, BodyId::Moon});
    set.Add({"lunar_solar_farm", "Lunar Solar Farm", "A ring of panels with no weather to worry about. Birds are not an issue.",
             "lunar_solar_rings", 255000000000, 217000000, 1.15, 6,
             L::Orbit, 0xAA78FF, 7, BodyId::Moon});
    set.Add({"solar_probe", "Solar Probe", "Flies into the Sun on purpose. Sends postcards.",
             "solar_probes", 1510000000000, 1170000000, 1.15, 40,
             L::Orbit, 0x8CE6FF, 8, BodyId::Sun});
    set.Add({"solar_catcher", "Dyson Collector", "One collector of the swarm. Takes sunlight home; the Sun doesn't invoice.",
             "solar_collector_swarm", 9000000000000, 6330000000, 1.15, 3,
             L::Orbit, 0xFFC878, 8, BodyId::Sun});
    set.SetMilestones({{10, 2}, {25, 2}, {50, 2}, {100, 2}, {200, 2}});
}

// Cuantas tecnologias, interacciones e instalaciones salieron de la hoja: una prueba compara contra esto.
inline constexpr int kGeneratedTechCount = 260;
inline constexpr int kGeneratedInteractionCount = 3;
inline constexpr int kGeneratedInstallationCount = 18;

} // namespace evergrave
