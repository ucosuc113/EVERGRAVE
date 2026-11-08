#pragma once

// EarthFx: lo que las TECNOLOGIAS de las eras 1 a 4 ponen en el mundo alrededor de la Tierra. La tienda ya la dibuja
// WorldFx (cada unidad comprada); esto es la otra mitad: lo que el arbol de mejoras construye aunque no se compre
// nada. Cada familia sale de la hoja Families del Excel y se enciende por las tecnologias que la crean y la mejoran
// (`tree.IsResearched`); una tecnologia que no esta investigada simplemente no dibuja. Ver docs/plan-visual.md.
//
// DOS CAPAS, y las dos son datos fijos al planeta o a una orbita:
//  - LA RED: veintidos nodos repartidos por la esfera (una espiral dorada, para que no se amontonen) y las aristas
//    mas cortas entre ellos. Sobre la misma red viven la red electrica, el transporte, la industria, los reactores, los
//    parques eolicos, los laboratorios laser y los observatorios: cada uno ocupa unos nodos, asi la noche de la Tierra
//    se lee como UN lugar habitado y no como seis cosas sueltas. Las aristas se ordenan por largo: las cortas salen primero
//    (la red electrica nace local) y las largas, las que cruzan mares, al final.
//  - LA ORBITA: remolcadores, estacion modular, panel solar espacial y anillo orbital. Giran como las unidades de la tienda
//    (misma `OrbitSlot`) y se apagan en la sombra del planeta.
//
// No hay nada de esto en el motor: son los mismos rectangulos y lineas de UI que WorldFx. Y esta hecho a lo largo de la
// cuenta de siempre: la red esta en el mundo (rota con la Tierra, que es la matriz de su nodo), asi no hay que animarla.

#include "FxKit.h"


class EarthFx : public BodyNet {
public:
    EarthFx() { BuildNet(22, 77u, 38, 1.1f); }

    void Draw(UiDrawList& ui, const Cam& cam, const Body& earth, const GameState& game, const GameObject& bodyObj, const XMVECTOR& sunCenter, float time) {
        Begin(ui, cam, earth, game, bodyObj, sunCenter, time);
        Run(&EarthFx::DrawIndustry);
        Run(&EarthFx::DrawGrid);
        Run(&EarthFx::DrawTransport);
        Run(&EarthFx::DrawWind);
        Run(&EarthFx::DrawNuclear);
        Run(&EarthFx::DrawLabs);
        Run(&EarthFx::DrawObservatories);
        Run(&EarthFx::DrawRockets);
        Run(&EarthFx::DrawMagneticLaunch);
        Run(&EarthFx::DrawTugs);
        Run(&EarthFx::DrawStation);
        Run(&EarthFx::DrawOrbitalSolar);
        Run(&EarthFx::DrawRing);
        Run(&EarthFx::DrawNetwork);
        Run(&EarthFx::DrawAntimatter);
        Run(&EarthFx::DrawYards);
        Run(&EarthFx::DrawAsteroids);
        Run(&EarthFx::DrawTankers);
        Run(&EarthFx::DrawTelescopes);
        Run(&EarthFx::DrawSystemGrid);
        Run(&EarthFx::DrawMirrorFlashes);
    }

private:
    // Cada familia empieza y termina SIN profundidad (`g_mod`): lo que se dibuja en el espacio la fija con `Depth(vz)` o
    // `AirPt`, y no puede quedarle puesta a la familia siguiente, que puede ser de la superficie.
    void Run(void (EarthFx::*family)()) {
        g_mod = FxMod{};
        (this->*family)();
        g_mod = FxMod{};
    }
    // Pone la profundidad de lo que se dibuja a continuacion. Devuelve el `Cue`, para quien quiera mas (el desenfoque).
    static Cue Depth(float vz) {
        const Cue c = DepthCue(vz);
        g_mod = ModOf(c);
        return c;
    }

    int ActiveEdges() const {
        int n = 0;
        if (Has("power_grid")) n = 8;
        if (Has("industrial_grid")) n = 14;
        if (Has("national_supergrid")) n = 22;
        if (Has("continental_grid")) n = static_cast<int>(m_edges.size());
        return std::min(n, static_cast<int>(m_edges.size()));
    }
    // ------------------------------------------------------------------ la red electrica
    void DrawGrid() {
        const int n = ActiveEdges();
        if (n == 0) return;
        UiDrawList& ui = *m_ui;
        const bool copper = Has("copper_wiring"), super = Has("superconductors"), hv = Has("high_voltage_lines");
        const std::uint32_t col = super ? 0xBFE8FF : copper ? 0xFF9A50 : 0xFFC850;
        std::vector<char> used(m_nodes.size(), 0);
        for (int k = 0; k < n; ++k) {
            const Edge& e = m_edges[static_cast<std::size_t>(k)];
            used[static_cast<std::size_t>(e.a)] = used[static_cast<std::size_t>(e.b)] = 1;
            const bool backbone = hv && k % 3 == 0;
            float px = 0, py = 0;
            bool have = false;
            for (int i = 0; i <= e.samples; ++i) {
                float x, y, night;
                if (!EdgePoint(e, static_cast<float>(i) / static_cast<float>(e.samples), x, y, night)) {
                    have = false;
                    continue;
                }
                // LA RED ES DE NOCHE. Una linea de luz es visible donde es de noche: de dia sobre el mar azul parecia una jaula de alambre
                // (lineas blancas de un lado a otro del planeta que no son de nada). Ahora el peso de la noche va al cuadrado y de dia no se dibuja;
                // la columna vertebral es mas brillante, no mas gruesa.
                const float lit = night * night;
                if (have && lit > 0.04f) ui.AddLine(px, py, x, y, ColorOf(col, (0.03f + 0.75f * lit) * (backbone ? 1.4f : 1.0f)), 1.0f);
                px = x;
                py = y;
                have = true;
            }
        }
        // Pulsos de carga: uno por linea (condensadores) o tres parejos (balanceo de la red, que es lo que quita el parpadeo).
        const bool caps = Has("capacitor_banks"), even = Has("grid_balancing");
        if (caps || even) {
            for (int k = 0; k < n; ++k) {
                const Edge& e = m_edges[static_cast<std::size_t>(k)];
                const int count = even ? 3 : 1;
                for (int j = 0; j < count; ++j) {
                    const float t = std::fmod(m_time / (4.0f + static_cast<float>(k % 3)) + static_cast<float>(k) * 0.37f + (even ? static_cast<float>(j) / 3.0f : 0.0f), 1.0f);
                    float x, y, night;
                    if (EdgePoint(e, t, x, y, night)) Dot(ui, x, y, 2.0f, ColorOf(0xFFF2C8, 0.05f + 0.95f * night * night));
                }
            }
        }
        // En los nodos: baterias (un bloque que pulsa al anochecer) y el halo helado de los criogenicos.
        for (std::size_t i = 0; i < m_nodes.size(); ++i) {
            if (!used[i]) continue;
            float x, y, night;
            if (!NodeAt(static_cast<int>(i), x, y, night)) continue;
            if (night > 0.3f) Halo(ui, x, y, 4.0f, col, night * night); // cada nodo de la red brilla (de noche): una ciudad, no un punto
            if (Has("cryogenic_cooling")) Halo(ui, x, y, 5.0f, 0xA8E0FF, 0.5f + 0.5f * night);
            if (Has("grid_storage")) {
                const float pulse = 0.55f + 0.45f * std::sin(m_time * 1.3f + static_cast<float>(i));
                ui.AddRect(kit::Rect(std::floor(x) - 1.0f, std::floor(y) - 1.0f, 2.0f, 2.0f), ColorOf(0xFFD890, (0.2f + 0.8f * night) * pulse));
            }
        }
    }

    // ------------------------------------------------------------------ transporte
    void DrawTransport() {
        if (!Has("rail_freight")) return;
        UiDrawList& ui = *m_ui;
        const int rail = std::min(14, ActiveEdges());
        for (int k = 0; k < rail; ++k) { // trenes: puntos tibios que van y vienen por las aristas
            const Edge& e = m_edges[static_cast<std::size_t>(k)];
            for (int j = 0; j < 2; ++j) {
                float t = std::fmod(m_time / 7.0f + static_cast<float>(k) * 0.29f + static_cast<float>(j) * 0.5f, 1.0f);
                if (j == 1) t = 1.0f - t;
                float x, y, night;
                if (EdgePoint(e, t, x, y, night)) Dot(ui, x, y, 1.0f, ColorOf(0xFFE8B0, 0.3f + 0.7f * night));
            }
        }
        if (Has("containerized_shipping")) { // barcos: puntos blancos lentos por las aristas largas, las que cruzan mares
            for (int k = 14; k < ActiveEdges(); ++k) {
                const Edge& e = m_edges[static_cast<std::size_t>(k)];
                const float t = std::fmod(m_time / 14.0f + static_cast<float>(k) * 0.23f, 1.0f);
                float x, y, night;
                if (EdgePoint(e, t, x, y, night)) Dot(ui, x, y, 1.0f, ColorOf(0xFFFFFF, 0.35f + 0.5f * night));
            }
        }
        if (Has("high_speed_rail")) { // trenes rapidos: una rafaga blanca de tres puntos
            for (int k = 0; k < std::min(9, rail); k += 2) {
                const Edge& e = m_edges[static_cast<std::size_t>(k)];
                const float t0 = std::fmod(m_time / 2.6f + static_cast<float>(k) * 0.31f, 1.0f);
                for (int s = 0; s < 3; ++s) {
                    float x, y, night;
                    if (EdgePoint(e, std::max(0.0f, t0 - 0.03f * static_cast<float>(s)), x, y, night)) Dot(ui, x, y, 1.0f, ColorOf(0xFFFFFF, (0.9f - 0.28f * static_cast<float>(s)) * (0.4f + 0.6f * night)));
                }
            }
        }
        if (Has("megaproject_financing") && !Has("continental_grid")) { // grua: luces ambar que parpadean donde se construira lo siguiente
            for (int i : {7, 15, 19}) {
                float x, y, night;
                if (i < static_cast<int>(m_nodes.size()) && NodeAt(i, x, y, night) && std::fmod(m_time * 1.2f + static_cast<float>(i), 1.0f) < 0.5f) Dot(ui, x, y, 1.0f, ColorOf(0xFFB030, 0.9f));
            }
        }
    }

    // ------------------------------------------------------------------ industria
    void DrawIndustry() {
        if (!Has("heavy_industry")) return;
        UiDrawList& ui = *m_ui;
        const int sites = Has("megafactories") ? 7 : Has("factory_robots") ? 5 : 3;
        static const int idx[] = {0, 3, 6, 9, 12, 15, 18, 21};
        if (Has("planetary_industry")) { // cinturones: tira ambar tenue por TODA la red
            for (const Edge& e : m_edges) {
                float px = 0, py = 0;
                bool have = false;
                for (int i = 0; i <= e.samples; i += 2) {
                    float x, y, night;
                    if (!EdgePoint(e, static_cast<float>(i) / static_cast<float>(e.samples), x, y, night)) {
                        have = false;
                        continue;
                    }
                    if (have && night > 0.3f) ui.AddLine(px, py, x, y, ColorOf(0xFF9A30, 0.04f + 0.26f * night * night), 1.0f);
                    px = x;
                    py = y;
                    have = true;
                }
            }
        }
        for (int s = 0; s < (Has("planetary_industry") ? 8 : sites); ++s) {
            const int i = idx[s];
            if (i >= static_cast<int>(m_nodes.size())) continue;
            float x, y, night;
            if (!NodeAt(i, x, y, night)) continue;
            Halo(ui, x, y, Has("megafactories") ? 6.0f : 4.5f, 0xFFA030, 0.5f + 0.9f * night); // el brillo de una fabrica: redondo, como una luz de verdad
            Dot(ui, x, y, 2.0f, ColorOf(0xFFB050, 0.35f + 0.55f * night));
            if (Has("alloy_foundries")) // pinchos naranjas: los hornos
                for (int k = 0; k < 3; ++k) Dot(ui, x + (static_cast<float>(k) - 1.0f) * 2.0f, y - 2.0f + static_cast<float>(k % 2) * 4.0f, 1.0f, ColorOf(0xFF7020, 0.4f + 0.5f * night * (0.6f + 0.4f * std::sin(m_time * 7.0f + static_cast<float>(k + s)))));
            if (Has("factory_robots")) // bloques iluminados en cuadricula
                for (int r = 0; r < 2; ++r)
                    for (int c = 0; c < 3; ++c) ui.AddRect(kit::Rect(std::floor(x) - 3.0f + static_cast<float>(c) * 2.0f, std::floor(y) + 3.0f + static_cast<float>(r) * 2.0f, 1.0f, 1.0f), ColorOf(0xFFE0A0, 0.25f + 0.6f * night));
            // humo: una columna que sube hacia afuera del planeta; con las megafabricas, una nube propia.
            const XMVECTOR w = World(m_nodes[static_cast<std::size_t>(i)].dir, 0.0f);
            float tx, ty;
            const XMVECTOR nw = XMVector3Normalize(XMVectorSubtract(w, m_body->center));
            if (Proj(XMVectorAdd(w, XMVectorScale(nw, m_body->radius * 0.14f)), tx, ty)) {
                for (int k = 0; k < 3; ++k) {
                    const float f = (static_cast<float>(k) + std::fmod(m_time * 0.3f + static_cast<float>(s) * 0.2f, 1.0f)) / 3.0f;
                    Dot(ui, x + (tx - x) * f, y + (ty - y) * f, f < 0.5f ? 2.0f : 1.0f, ColorOf(0x6A6E78, (1.0f - f) * 0.6f));
                }
                if (Has("megafactories")) Dot(ui, tx, ty, 5.0f, ColorOf(0x8A8EA0, 0.25f));
            }
        }
    }

    // ------------------------------------------------------------------ reactores y fusion
    void DrawNuclear() {
        if (!Has("fission_reactors")) return;
        UiDrawList& ui = *m_ui;
        static const int idx[] = {4, 10, 16};
        for (int s = 0; s < 3; ++s) {
            if (idx[s] >= static_cast<int>(m_nodes.size())) continue;
            float x, y, night;
            if (!NodeAt(idx[s], x, y, night)) continue;
            const float pulse = 0.75f + 0.25f * std::sin(m_time * 1.5f + static_cast<float>(s));
            Halo(ui, x, y - 1.0f, 5.0f, 0x6EC8FF, 0.7f * pulse);
            DrawSprite(ui, kDome, x, y, 0x6EC8FF, 0.8f + 0.4f * night, 0.95f);
            if (Has("breeder_reactors")) DrawSprite(ui, kDome, x + 4.0f, y + 1.0f, 0x6EC8FF, 0.8f + 0.4f * night, 0.95f);
            // Anillos: un toro de plasma, vistos de lado. El piloto es azul y chico; el del Tokamak, mas grande y blanco; el comercial, en los tres sitios.
            const bool pilot = Has("fusion_pilot"), commercial = Has("fusion_commercial");
            const bool ring = (Has("tokamak_prototype") && s == 1) || (pilot && s == 1) || commercial;
            if (!ring) continue;
            const float rx = commercial ? 4.0f : pilot ? 5.0f : 3.0f, ry = rx * 0.5f;
            const std::uint32_t c = commercial || pilot ? 0xFFFFFF : 0x6EC8FF;
            for (int k = 0; k < 12; ++k) {
                const float a = static_cast<float>(k) * 2.0f * kPi / 12.0f + m_time * 0.8f;
                Dot(ui, x + std::cos(a) * rx, y - 5.0f + std::sin(a) * ry, 1.0f, ColorOf(c, 0.6f + 0.4f * std::sin(a + m_time)));
            }
        }
    }

    // ------------------------------------------------------------------ parques eolicos
    void DrawWind() {
        if (!Has("wind_turbines")) return;
        UiDrawList& ui = *m_ui;
        static const int idx[] = {1, 5, 8, 13, 17, 19};
        for (int s = 0; s < 6; ++s) {
            if (idx[s] >= static_cast<int>(m_nodes.size())) continue;
            float x, y, night;
            if (!NodeAt(idx[s], x, y, night)) continue;
            for (int k = 0; k < 3; ++k) { // tres molinos en fila; las aspas alternan vertical y horizontal
                const float cx = std::floor(x) + static_cast<float>(k) * 4.0f - 4.0f, cy = std::floor(y);
                const bool vert = std::fmod(m_time * 1.6f + static_cast<float>(k) * 0.3f + static_cast<float>(s) * 0.17f, 1.0f) < 0.5f;
                const UiColor c = ColorOf(0xEEF4FF, 0.55f + 0.2f * (1.0f - night));
                if (vert) ui.AddRect(kit::Rect(cx, cy - 1.0f, 1.0f, 3.0f), c);
                else ui.AddRect(kit::Rect(cx - 1.0f, cy, 3.0f, 1.0f), c);
            }
            if (Has("hybrid_plant")) { // el complejo: un parche solar y un hornillo junto al parque
                ui.AddRect(kit::Rect(std::floor(x) - 4.0f, std::floor(y) + 3.0f, 3.0f, 2.0f), ColorOf(0x2A4A8A, 1.0f));
                Dot(ui, x + 3.0f, y + 4.0f, 2.0f, ColorOf(0xFFB050, 0.4f + 0.5f * night));
            }
        }
    }

    // ------------------------------------------------------------------ laboratorios laser
    void DrawLabs() {
        if (!Has("stimulated_emission")) return;
        UiDrawList& ui = *m_ui;
        const bool ruby = Has("ruby_laser"), gas = Has("gas_lasers"), semi = Has("semiconductor_diodes"), fiber = Has("fiber_amplifiers"), cpa = Has("chirped_pulses");
        const std::uint32_t col = cpa ? 0xFFFFFF : gas ? 0x80FF9A : 0xFF5060;
        const float len = cpa ? 0.75f : gas ? 0.55f : ruby ? 0.45f : 0.3f;
        const float period = cpa ? 4.0f : semi ? 5.0f : ruby ? 6.0f : 9.0f;
        const float dur = gas ? 2.0f : cpa ? 0.35f : ruby ? 0.4f : 0.25f;
        const int labs = gas ? 3 : ruby ? 2 : 1;
        static const int idx[] = {2, 11, 16};
        for (int l = 0; l < labs; ++l) {
            if (idx[l] >= static_cast<int>(m_nodes.size())) continue;
            const XMVECTOR dir = m_nodes[static_cast<std::size_t>(idx[l])].dir;
            const XMVECTOR base = World(dir, 0.0f);
            const bool inScene = FxBus().fleetInScene;
            // Con la flota en la escena el haz es un OBJETO (lo dibuja Fleet, con brillo y profundidad reales) y no hace falta que su
            // base mire a la camara: un laser de la cara oculta que asoma por el borde del planeta tiene que verse. Sin ella, como antes.
            if (!inScene && !NearSide(base)) continue;
            float bx = 0, by = 0;
            const bool baseSeen = NearSide(base) && Proj(base, bx, by);
            if (!inScene && !baseSeen) continue;
            const float night = Night(base);
            const float t = std::fmod(m_time / period + static_cast<float>(l) * 0.33f, 1.0f) * period;
            if (baseSeen) Dot(ui, bx, by, 2.0f, ColorOf(0xE8ECF8, 0.9f)); // el laboratorio
            const int beams = semi ? 5 : 1;
            for (int b = 0; b < beams; ++b) {
                const bool on = semi ? std::fmod(m_time * 3.0f + static_cast<float>(b) * 0.37f + static_cast<float>(l), 1.0f) < 0.6f : t < dur;
                if (!on) continue;
                const float off = semi ? (static_cast<float>(b) - 2.0f) * 0.012f : 0.0f;
                const XMVECTOR d2 = XMVector3Normalize(XMVectorAdd(dir, XMVectorSet(off, 0.0f, off * 0.5f, 0.0f)));
                const XMVECTOR top = World(d2, semi ? 0.3f : len);
                if (inScene) {
                    WorldBeam wb{};
                    XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(wb.a), World(d2, 0.0f));
                    XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(wb.b), top);
                    wb.width = fiber ? 0.03f : cpa ? 0.026f : 0.022f;
                    wb.rgb[0] = static_cast<float>((col >> 16) & 0xFF) / 255.0f;
                    wb.rgb[1] = static_cast<float>((col >> 8) & 0xFF) / 255.0f;
                    wb.rgb[2] = static_cast<float>(col & 0xFF) / 255.0f;
                    wb.power = (semi ? 0.8f : 1.0f) * (0.7f + 0.3f * night);
                    FxBus().beams.push_back(wb);
                    continue;
                }
                float tx, ty, tz;
                if (!m_cam->Project(top, tx, ty, tz) || Occluded(*m_body, tx, ty, tz)) continue;
                const float a = (0.65f + 0.35f * night) * (semi ? 0.8f : 1.0f);
                // Un haz de luz y no una barra: nucleo fino, estela tenue y el brillo en la punta (antes era una linea
                // blanca de 2 px que tapaba el planeta). La fibra lo hace mas ancho, no mas duro.
                Beam(ui, bx, by, tx, ty, col, a * (fiber ? 1.0f : 0.9f), cpa ? 0xFFFFFF : col);
                if (!semi) Halo(ui, tx, ty, cpa ? 6.0f : 4.5f, col, a); // el destello en la punta
            }
            if (baseSeen && t < 0.25f && !semi) Dot(ui, bx, by, 5.0f, ColorOf(0xFFFFFF, 0.5f * (1.0f - t / 0.25f)));
        }
    }

    // ------------------------------------------------------------------ observatorios
    void DrawObservatories() {
        if (!Has("spectrometry")) return;
        UiDrawList& ui = *m_ui;
        static const int idx[] = {6, 14};
        float xs[2] = {0, 0}, ys[2] = {0, 0};
        bool ok[2] = {false, false};
        for (int s = 0; s < 2; ++s) {
            if (idx[s] >= static_cast<int>(m_nodes.size())) continue;
            float x, y, night;
            if (!NodeAt(idx[s], x, y, night)) continue;
            xs[s] = x;
            ys[s] = y;
            ok[s] = true;
            DrawSprite(ui, kDome, x, y, 0xF0F4FF, 1.0f, 0.95f);
            float tx, ty;
            const XMVECTOR w = World(m_nodes[static_cast<std::size_t>(idx[s])].dir, 0.0f);
            const XMVECTOR nw = XMVector3Normalize(XMVectorSubtract(w, m_body->center));
            if (!Proj(XMVectorAdd(w, XMVectorScale(nw, m_body->radius * 0.25f)), tx, ty)) continue;
            ui.AddLine(x, y - 1.0f, tx, ty, ColorOf(0xCFE0FF, 0.15f + 0.3f * night), 1.0f); // el cono de vision
            if (Has("beam_profiling")) // el perfil del haz: un anillo de puntos en la punta del cono
                for (int k = 0; k < 6; ++k) {
                    const float a = static_cast<float>(k) * kPi / 3.0f;
                    Dot(ui, tx + std::cos(a) * 2.0f, ty + std::sin(a) * 1.2f, 1.0f, ColorOf(0xCFE0FF, 0.5f));
                }
        }
        if (Has("interferometers") && ok[0] && ok[1]) // las dos cupulas, ligadas: un arreglo
            ui.AddLine(xs[0], ys[0], xs[1], ys[1], ColorOf(0x9CD8FF, 0.35f), 1.0f);
    }

    // LA FAMILIA `rockets` (hoja Families): lo que el arbol de tecnologias pone en el cielo, no la tienda.
    //   Sounding Rockets (26): UNA plataforma; un cohete sube en arco y vuelve a caer. Su torre de acero es Steel
    //     Framework (22), que es requisito de la 26: aparece junto con la plataforma.
    //   Orbital Launch Sites (64): cuatro plataformas con su calendario; el cohete llega a la orbita y se une a las demas.
    //   Reusable Rockets (71): la primera etapa se separa, vuelve y aterriza en su plataforma, con un destello.
    // Las plataformas estan fijas a la superficie (giran con la Tierra), igual que lo de DrawSurface.
    void DrawRockets() {
        UiDrawList& ui = *m_ui;
        const Cam& cam = *m_cam;
        const Body& earth = *m_body;
        const TechTree& tree = *m_tree;
        // Las tres son ramas sueltas del arbol (la 64 no pide la 26), asi que cada una funciona sola: no se espera a la
        // "creadora". Sin ninguna, no hay plataforma.
        const bool sites = tree.IsResearched("orbital_launch_sites");
        const bool reuse = tree.IsResearched("reusable_rockets");
        if (!sites && !reuse && !tree.IsResearched("sounding_rockets")) return;
        const bool tower = tree.IsResearched("steel_framework") || sites;
        struct Pad {
            float lat, lon, period, offset;
        };
        static const Pad pads[] = {{0.50f, 0.20f, 11.0f, 0.00f}, {0.10f, 1.80f, 9.0f, 0.37f}, {0.65f, 3.40f, 12.0f, 0.71f}, {0.30f, 5.00f, 10.0f, 0.15f}};
        const int padCount = sites ? 4 : 1;
        const float flight = sites ? 5.5f : 4.2f;       // segundos de vuelo
        const float hOrbit = 0.2f;                      // altura de la orbita, en radios (la de los satelites)
        const float ascent = 0.55f;                     // hasta donde llega a la orbita, de 0 a 1
        const float sep = 0.28f, landEnd = 0.9f;        // separacion de la primera etapa y su aterrizaje, de 0 a 1

        const XMMATRIX m = m_m;
        const XMVECTOR vc = m_vc;
        const XMVECTOR yAxis = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

        for (int i = 0; i < padCount; ++i) {
            const Pad& pad = pads[i];
            const XMVECTOR p = XMVectorSet(std::cos(pad.lat) * std::cos(pad.lon), std::sin(pad.lat), std::cos(pad.lat) * std::sin(pad.lon), 0.0f);
            const XMVECTOR e = XMVector3Normalize(XMVector3Cross(yAxis, p)); // hacia el este: la direccion en que se lanza
            // Un punto de la trayectoria: `h` radios de altura y `phi` radianes hacia el este de la plataforma.
            auto at = [&](float h, float phi) {
                const XMVECTOR dir = XMVectorAdd(XMVectorScale(p, std::cos(phi)), XMVectorScale(e, std::sin(phi)));
                return XMVector3TransformCoord(XMVectorScale(dir, 1.0f + h), m);
            };
            const XMVECTOR padWorld = at(0.0f, 0.0f);
            const XMVECTOR pv = XMVector3TransformCoord(padWorld, cam.view);
            if (XMVectorGetX(XMVector3Dot(XMVectorSubtract(pv, vc), XMVectorNegate(pv))) <= 0.0f) continue; // del otro lado del planeta
            float px, py, pz;
            if (!cam.Project(padWorld, px, py, pz)) continue;

            // La plataforma: un punto gris, y su torre si ya hay acero.
            Dot(ui, px, py, 3.0f, ColorOf(0x2A2E38, 0.9f));
            if (tower) {
                DrawSprite(ui, kTower, px, py - 2.0f, 0xE6ECF8, 1.0f, 1.0f);
                // Una baliza roja que parpadea en la punta: se ve de dia y de noche, y dice "aqui hay una plataforma".
                if (std::fmod(m_time * 0.9f + pad.offset * 3.0f, 1.0f) < 0.5f) ui.AddRect(kit::Rect(std::floor(px), std::floor(py) - 5.0f, 1.0f, 1.0f), ColorOf(0xFF3030));
            } else {
                Dot(ui, px, py, 2.0f, ColorOf(0xE6ECF8, 0.9f));
            }

            const float period = pad.period;
            const float t = m_time / period + pad.offset;
            const float tau = (t - std::floor(t)) * period; // segundos desde el ultimo despegue
            // Despegue: un destello en la base.
            if (tau < 0.6f) Flash(ui, px, py, 0xFFA040, 1.0f - tau / 0.6f);
            if (tau >= flight) continue;
            const float u = tau / flight;

            // Donde esta el cohete en `uu` (0..1): sube a la orbita y sigue por ella; el de sondeo es un arco que vuelve.
            auto pos = [&](float uu, float& h, float& phi) {
                if (!sites) {
                    h = 0.5f * std::sin(kPi * uu);
                    phi = 0.25f * uu;
                } else if (uu < ascent) {
                    const float s = uu / ascent;
                    h = hOrbit * std::pow(s, 1.6f);
                    phi = 0.2f * s * s;
                } else {
                    h = hOrbit;
                    phi = 0.2f + 0.9f * (uu - ascent) / (1.0f - ascent);
                }
            };
            const float fade = sites && u > ascent ? 1.0f - 0.85f * (u - ascent) / (1.0f - ascent) : 1.0f;
            const float burnEnd = sites ? ascent : 0.45f;

            for (int k = 0; k < 16; ++k) {
                const float uu = u - 0.025f * static_cast<float>(k);
                if (uu < 0.0f) break;
                float h, phi, sx, sy, vz;
                pos(uu, h, phi);
                if (!cam.Project(at(h, phi), sx, sy, vz) || Occluded(earth, sx, sy, vz)) continue;
                const float a = (1.0f - 0.06f * static_cast<float>(k)) * fade;
                if (k == 0) {
                    Dot(ui, sx, sy, 5.0f, ColorOf(0xFFFFFF, 0.3f * fade)); // el halo, para que se vea sobre el dia
                    Dot(ui, sx, sy, 3.0f, ColorOf(0xFFFFFF, fade));
                    if (u < burnEnd) Dot(ui, sx, sy + 2.0f, 3.0f, ColorOf(0xFF8A30, 0.95f)); // la llama
                } else {
                    Dot(ui, sx, sy, k < 9 ? 2.0f : 1.0f, ColorOf(k < 4 ? 0xFFD8A0 : 0xF2F4FA, a));
                }
            }

            // La primera etapa de un cohete reutilizable: se separa, vuelve y aterriza en su plataforma.
            if (reuse && u >= sep) {
                float hs, phis;
                pos(sep, hs, phis);
                const float w = std::clamp((u - sep) / (landEnd - sep), 0.0f, 1.0f);
                const float h = hs * (1.0f - w) * (1.0f + 0.35f * std::sin(kPi * w));
                const float phi = phis * (1.0f - w);
                float sx, sy, vz;
                if (w < 1.0f && cam.Project(at(h, phi), sx, sy, vz) && !Occluded(earth, sx, sy, vz)) {
                    for (int k = 0; k < 3; ++k) {
                        float tx, ty, tz;
                        const float ww = std::max(0.0f, w - 0.04f * static_cast<float>(k));
                        const float hh = hs * (1.0f - ww) * (1.0f + 0.35f * std::sin(kPi * ww)), pp = phis * (1.0f - ww);
                        if (cam.Project(at(hh, pp), tx, ty, tz)) Dot(ui, tx, ty, 1.0f, ColorOf(0x9CC8FF, 0.5f - 0.15f * static_cast<float>(k)));
                    }
                    Dot(ui, sx, sy, 1.0f, ColorOf(0xFFFFFF));
                    if (w > 0.75f) Dot(ui, sx, sy + 1.0f, 2.0f, ColorOf(0xFF9A40, 0.9f)); // el motor del aterrizaje
                } else if (w >= 1.0f && u < landEnd + 0.1f) {
                    // Aterrizo: un destello azulado en la plataforma.
                    Flash(ui, px, py, 0x7CB8FF, 1.0f - (u - landEnd) / 0.1f);
                }
            }
        }
    }


    // ------------------------------------------------------------------ lanzador magnetico
    // Una pista recta de bobinas en la costa: se enciende de a una, y al final tira la carga en un arco alto y rapido.
    void DrawMagneticLaunch() {
        if (!Has("magnetic_launch")) return;
        UiDrawList& ui = *m_ui;
        const float lat = 0.62f, lon = 4.0f;
        const XMVECTOR p = DirOf(lat, lon);
        const XMVECTOR e = XMVector3Normalize(XMVector3Cross(XMVectorSet(0, 1, 0, 0), p));
        auto at = [&](float h, float phi) { return World(XMVectorAdd(XMVectorScale(p, std::cos(phi)), XMVectorScale(e, std::sin(phi))), h); };
        if (!NearSide(at(0.0f, 0.0f))) return;
        const float period = 14.0f;
        const float tau = std::fmod(m_time / period + 0.5f, 1.0f) * period;
        for (int k = 0; k < 8; ++k) { // la pista
            float x, y;
            if (!Proj(at(0.002f, 0.018f * static_cast<float>(k)), x, y)) continue;
            const bool lit = tau < 1.4f && tau * 6.0f > static_cast<float>(k);
            Dot(ui, x, y, 1.0f, lit ? ColorOf(0x9CE8FF, 1.0f) : ColorOf(0x8A94A8, 0.8f));
        }
        const float fly = 4.0f;
        if (tau < 1.4f || tau >= 1.4f + fly) return;
        const float u = (tau - 1.4f) / fly;
        for (int k = 0; k < 9; ++k) {
            const float uu = u - 0.03f * static_cast<float>(k);
            if (uu < 0.0f) break;
            float x, y, z;
            if (!m_cam->Project(at(0.4f * std::sin(kPi * uu * 0.9f), 0.14f + 0.35f * uu), x, y, z) || Occluded(*m_body, x, y, z)) continue;
            Dot(ui, x, y, k == 0 ? 3.0f : 2.0f, ColorOf(k == 0 ? 0xFFFFFF : 0xBFE8FF, 1.0f - 0.1f * static_cast<float>(k)));
        }
    }

    // ------------------------------------------------------------------ orbita
    // Remolcadores: naves chicas con una llama azul que saltan entre orbitas bajas. Con los enjambres autonomos, nueve.
    void DrawTugs() {
        if (!Has("orbital_tugs")) return;
        const int n = Has("autonomous_swarms") ? 9 : 3;
        const float base = m_body->radius * 1.12f;
        for (int j = 0; j < n; ++j) {
            const Slot s = OrbitSlot(m_body->center, base, 4242u, n, j, m_time * 1.2f);
            float sx, sy, vz;
            if (!m_cam->Project(s.pos, sx, sy, vz) || Occluded(*m_body, sx, sy, vz)) continue;
            const float lit = Eclipse(s.pos, m_body->center, m_toSun, m_body->radius);
            const Cue cue = Depth(vz);
            // Chicos y numerosos, como los satelites: delante del plano de enfoque son un punto de luz desenfocado.
            if (cue.blur > 0.5f && cue.near) {
                Bokeh(*m_ui, sx, sy, 0.8f, cue.blur, 0xB8C0D0, lit);
                continue;
            }
            DrawSprite(*m_ui, kTug, sx, sy, 0xB8C0D0, lit, 0.95f);
        }
    }

    // La estacion: UN punto de una orbita lenta que crece por modulos, tecnologia a tecnologia.
    void DrawStation() {
        if (!Has("orbital_assembly") || FxBus().fleetInScene) return;
        UiDrawList& ui = *m_ui;
        const Slot s = OrbitSlot(m_body->center, m_body->radius * 1.3f, 5151u, 1, 0, m_time * 0.25f);
        float sx, sy, vz;
        if (!m_cam->Project(s.pos, sx, sy, vz) || Occluded(*m_body, sx, sy, vz)) return;
        const float lit = Eclipse(s.pos, m_body->center, m_toSun, m_body->radius);
        Depth(vz);
        const bool complex = Has("orbital_complex");
        if (complex) Dot(ui, sx, sy, 14.0f, ColorOf(0xFFE6A0, 0.08f * lit)); // la estacion hecha ciudad brilla
        DrawSprite(ui, kStationCore, sx, sy, 0xB8C0D0, lit, 1.0f);
        if (Has("microgravity_labs")) DrawSprite(ui, kLabModule, sx - 4.0f, sy, 0xF0F4FF, lit, 1.0f);
        if (Has("orbital_foundries")) DrawSprite(ui, kLabModule, sx + 4.0f, sy, 0xFF9A40, lit * (0.8f + 0.2f * std::sin(m_time * 6.0f)), 1.0f);
        if (Has("zero_g_manufacturing")) { // el larguero con destellos de cristal
            ui.AddRect(kit::Rect(std::floor(sx) - 8.0f, std::floor(sy) + 3.0f, 17.0f, 1.0f), Shaded(0x8A94A8, lit, 0.9f));
            if (std::fmod(m_time * 0.8f, 1.0f) < 0.15f) Glint(ui, sx + std::sin(m_time) * 6.0f, sy + 3.0f, 0.9f);
        }
        if (Has("orbital_habitats")) { // el anillo habitado, girando, con jardines verdes
            for (int k = 0; k < 14; ++k) {
                const float a = static_cast<float>(k) * 2.0f * kPi / 14.0f + m_time * 0.7f;
                Dot(ui, sx + std::cos(a) * 7.0f, sy - 5.0f + std::sin(a) * 2.5f, 1.0f, Shaded(k % 3 == 0 ? 0x70E070 : 0xE8ECF8, lit, 0.9f));
            }
        }
        if (Has("orbital_hub")) { // el hub: brazos de acoplamiento y naves esperando en fila
            for (int k = 0; k < 4; ++k) ui.AddRect(kit::Rect(std::floor(sx) + (k < 2 ? -3.0f : 3.0f), std::floor(sy) + (k % 2 == 0 ? -3.0f : 3.0f), 1.0f, 1.0f), Shaded(0xB8C0D0, lit, 1.0f));
            for (int k = 0; k < 2; ++k) DrawSprite(ui, kTug, sx - 10.0f + static_cast<float>(k) * 20.0f, sy + 6.0f, 0xB8C0D0, lit, 0.9f);
        }
        if (complex) { // la pila de modulos del complejo
            for (int k = -2; k <= 2; ++k) {
                if (k == 0) continue;
                DrawSprite(ui, kLabModule, sx, sy + static_cast<float>(k) * 4.5f, k % 2 ? 0xF0F4FF : 0xFFC878, lit, 1.0f);
            }
        }
    }

    // El panel solar espacial: dos alas azules oscuras en una orbita fija; con la transmision, un pulso baja a una antena en el suelo.
    void DrawOrbitalSolar() {
        if (!Has("space_solar_arrays") || FxBus().fleetInScene) return;
        UiDrawList& ui = *m_ui;
        const Slot s = OrbitSlot(m_body->center, m_body->radius * 1.5f, 6262u, 1, 0, m_time * 0.2f);
        float sx, sy, vz;
        if (!m_cam->Project(s.pos, sx, sy, vz) || Occluded(*m_body, sx, sy, vz)) return;
        const float lit = Eclipse(s.pos, m_body->center, m_toSun, m_body->radius);
        Depth(vz);
        DrawSprite(ui, kWing, sx, sy, 0xC0C8D8, lit, 1.0f);
        if (lit > 0.9f && std::fmod(m_time * 0.4f, 1.0f) < 0.08f) Glint(ui, sx - 3.0f, sy, 0.9f);
        if (!Has("power_beaming")) return;
        const XMVECTOR target = World(DirOf(0.15f, 2.2f), 0.0f);
        float tx, ty;
        const bool seen = NearSide(target) && Proj(target, tx, ty);
        if (seen) {
            for (int k = 0; k < 6; ++k) { // el pulso de microondas
                const float t = std::fmod(m_time * 0.6f + static_cast<float>(k) / 6.0f, 1.0f);
                Dot(ui, sx + (tx - sx) * t, sy + (ty - sy) * t, 1.0f, ColorOf(0xFFE08A, 0.7f * (1.0f - t * 0.4f)));
            }
        }
        if (Has("microwave_downlink") && seen) { // la rectena: una cuadricula de antenas que se enciende donde cae el haz
            for (int r = 0; r < 3; ++r)
                for (int c = 0; c < 3; ++c) {
                    const float blink = 0.5f + 0.5f * std::sin(m_time * 4.0f + static_cast<float>(r * 3 + c));
                    ui.AddRect(kit::Rect(std::floor(tx) - 3.0f + static_cast<float>(c) * 3.0f, std::floor(ty) - 3.0f + static_cast<float>(r) * 3.0f, 1.0f, 1.0f), ColorOf(0xFFD070, 0.4f + 0.6f * blink));
                }
        }
    }

    // El anillo orbital: una linea de puntos alrededor de TODO el planeta, con pulsos que corren por ella.
    void DrawRing() {
        if (!Has("orbital_ring") || FxBus().fleetInScene) return; // en la escena: un anillo de verdad, que el planeta tapa por profundidad
        UiDrawList& ui = *m_ui;
        const XMMATRIX rot = XMMatrixRotationX(0.35f) * XMMatrixRotationY(0.6f);
        auto at = [&](float a) {
            const XMVECTOR v = XMVector3TransformNormal(XMVectorSet(std::cos(a), 0.0f, std::sin(a), 0.0f), rot);
            return XMVectorAdd(m_body->center, XMVectorScale(v, m_body->radius * 1.16f));
        };
        for (int k = 0; k < 140; ++k) {
            float x, y, z;
            if (!m_cam->Project(at(static_cast<float>(k) * 2.0f * kPi / 140.0f), x, y, z) || Occluded(*m_body, x, y, z)) continue;
            Depth(z); // el anillo es un aro: lo de delante brilla, lo de atras se apaga y se enfria
            Dot(ui, x, y, 1.0f, ColorOf(0xBFE8FF, k % 5 == 0 ? 0.7f : 0.35f));
        }
        for (int k = 0; k < 3; ++k) {
            float x, y, z;
            if (!m_cam->Project(at(m_time * 0.5f + static_cast<float>(k) * 2.1f), x, y, z) || Occluded(*m_body, x, y, z)) continue;
            Depth(z);
            Dot(ui, x, y, 2.0f, ColorOf(0xFFFFFF, 0.95f));
        }
    }

    // ================================================================== EL SISTEMA (eras 4 y 5), visto desde la Tierra
    // Lo que no esta en el planeta ni en su orbita baja: las balizas de los puntos de Lagrange, los astilleros, las rocas
    // capturadas, los petroleros que van a un gigante gaseoso, los telescopios y la linea de energia hacia el Sol. Todo en
    // el espacio ALREDEDOR de la Tierra, orientado con el Sol (`m_toSun`): el mundo, no la pantalla.

    // Un punto del mundo en pantalla, solo si el planeta no lo tapa.
    bool AirPt(const XMVECTOR& w, float& x, float& y) const {
        float z;
        if (!m_cam->Project(w, x, y, z)) return false;
        if (Occluded(*m_body, x, y, z)) return false;
        Depth(z); // todo lo del sistema que se dibuja despues de pedir este punto toma su profundidad
        return true;
    }
    // Los puntos de Lagrange del sistema Tierra-Sol: 0 = L1 (hacia el Sol), 1 = L2, 2 = L4, 3 = L5.
    XMVECTOR Lagrange(int k) const {
        const float R = m_body->radius;
        if (k == 0) return XMVectorAdd(m_body->center, XMVectorScale(m_toSun, R * 3.2f));
        if (k == 1) return XMVectorSubtract(m_body->center, XMVectorScale(m_toSun, R * 3.2f));
        const XMVECTOR v = XMVector3TransformNormal(m_toSun, XMMatrixRotationY(k == 2 ? 1.0472f : -1.0472f));
        return XMVectorAdd(m_body->center, XMVectorScale(v, R * 4.5f));
    }
    // Un disco redondo de pixeles (barridos de rectangulos): un planeta lejano. Un `Dot` grande se ve como un cuadrado.
    void Disc(float cx, float cy, float r, const UiColor& c) const {
        for (int dy = -static_cast<int>(r); dy <= static_cast<int>(r); ++dy) {
            const float w = std::sqrt(std::max(0.0f, r * r - static_cast<float>(dy * dy)));
            m_ui->AddRect(kit::Rect(std::floor(cx - w), std::floor(cy) + static_cast<float>(dy), std::floor(w) * 2.0f + 1.0f, 1.0f), c);
        }
    }
    // Una posicion en un ir y volver de `period` segundos: 0 -> 1 -> 0.
    float Triangle(float period, float phase = 0.0f) const {
        const float f = std::fmod(m_time / period + phase, 1.0f);
        return f < 0.5f ? f * 2.0f : 2.0f - f * 2.0f;
    }

    void DrawNetwork() {
        UiDrawList& ui = *m_ui;
        if (Has("deep_space_comms")) { // una antena en el suelo; los datos salen hacia el borde
            float x, y, night;
            if (NodeAt(7, x, y, night)) {
                DrawSprite(ui, kDome, x, y, 0xE8ECF8, 1.0f, 0.95f);
                const float dx = x - m_body->sx, dy = y - m_body->sy, len = std::max(1.0f, std::sqrt(dx * dx + dy * dy));
                for (int k = 0; k < 3; ++k) {
                    const float t = std::fmod(m_time * 0.35f + static_cast<float>(k) / 3.0f, 1.0f);
                    Dot(ui, x + dx / len * t * 70.0f, y + dy / len * t * 70.0f, 1.0f, ColorOf(0xBFE8FF, 0.9f * (1.0f - t)));
                }
            }
        }
        if (!Has("interplanetary_net")) return;
        float bx[4], by[4];
        bool ok[4];
        for (int k = 0; k < 4; ++k) {
            ok[k] = AirPt(Lagrange(k), bx[k], by[k]);
            if (!ok[k]) continue;
            Halo(ui, bx[k], by[k], 6.0f, 0x9CE8FF, 1.0f);
            DrawSprite(ui, kRelay, bx[k], by[k], 0x9CE8FF, 1.0f, 0.95f); // una baliza con forma (una cruz de tres pixeles), no un punto suelto
        }
        // Los pulsos: al azar entre dos balizas; con la IA del sistema, un solo pulso que da la vuelta por el camino mas corto.
        if (Has("interplanetary_ai")) {
            static const int loop[4] = {0, 2, 1, 3};
            const float f = std::fmod(m_time * 0.25f, 1.0f) * 4.0f;
            const int s = static_cast<int>(f) % 4, a = loop[s], b = loop[(s + 1) % 4];
            if (ok[a] && ok[b]) {
                const float t = f - std::floor(f);
                Dot(ui, bx[a] + (bx[b] - bx[a]) * t, by[a] + (by[b] - by[a]) * t, 2.0f, ColorOf(0xFFFFFF, 0.95f));
            }
        } else {
            const unsigned n = static_cast<unsigned>(std::floor(m_time * 0.5f));
            const int a = static_cast<int>(Hash(n, 3u) % 4u), b = (a + 1 + static_cast<int>(Hash(n, 4u) % 3u)) % 4;
            if (ok[a] && ok[b]) {
                const float t = std::fmod(m_time * 0.5f, 1.0f);
                Dot(ui, bx[a] + (bx[b] - bx[a]) * t, by[a] + (by[b] - by[a]) * t, 2.0f, ColorOf(0xFFFFFF, 0.9f * (1.0f - t * 0.4f)));
            }
        }
        if (Has("mass_relays")) // las compuertas lanzan la carga hacia afuera: rayas brillantes y rapidas
            for (int g = 0; g < 2; ++g) {
                if (!ok[g]) continue;
                const XMVECTOR dir = g == 0 ? m_toSun : XMVectorNegate(m_toSun);
                for (int k = 0; k < 3; ++k) {
                    const float t = std::fmod(m_time * 0.7f + static_cast<float>(k) * 0.07f + static_cast<float>(g) * 0.4f, 1.0f);
                    float x, y;
                    if (AirPt(XMVectorAdd(Lagrange(g), XMVectorScale(dir, m_body->radius * (0.3f + t * 6.0f))), x, y)) Dot(ui, x, y, k == 0 ? 2.0f : 1.0f, ColorOf(0xFFFFFF, (1.0f - t) * (0.9f - 0.25f * static_cast<float>(k))));
                }
            }
    }

    // El antimateria: un punto violeta que late dentro de una botella magnetica (un anillo de puntos que crece con la produccion).
    void DrawAntimatter() {
        if (!Has("antimatter_traps")) return;
        UiDrawList& ui = *m_ui;
        const Slot s = OrbitSlot(m_body->center, m_body->radius * 1.7f, 8383u, 1, 0, m_time * 0.4f);
        float x, y;
        if (!AirPt(s.pos, x, y)) return;
        const float beat = 0.6f + 0.4f * std::sin(m_time * 4.0f);
        const bool big = Has("antimatter_production");
        Halo(ui, x, y, big ? 7.0f : 5.0f, 0xB070FF, 0.9f * beat);
        Dot(ui, x, y, 2.0f, ColorOf(0xD8B0FF, 0.95f));
        const int n = big ? 10 : 6;
        const float r = big ? 5.0f : 3.0f;
        for (int k = 0; k < n; ++k) {
            const float a = static_cast<float>(k) * 6.2832f / static_cast<float>(n) + m_time * 1.5f;
            Dot(ui, x + std::cos(a) * r, y + std::sin(a) * r * 0.5f, 1.0f, ColorOf(0xB070FF, 0.7f));
        }
    }

    // Astilleros: una fabrica que se duplica, la escoria que corre, una nave que toma forma y lanzaderas entre dos astilleros.
    void DrawYards() {
        if (!Has("self_replicating_factories")) return;
        UiDrawList& ui = *m_ui;
        const Slot s1 = OrbitSlot(m_body->center, m_body->radius * 1.55f, 7373u, 1, 0, m_time * 0.12f);
        float x, y;
        if (!AirPt(s1.pos, x, y)) return;
        const int count = std::array<int, 3>{1, 2, 4}[static_cast<std::size_t>(std::fmod(m_time / 4.0f, 3.0f))]; // 1, 2, 4: se duplica
        for (int i = 0; i < count; ++i)
            DrawSprite(ui, kLabModule, x + static_cast<float>(i % 2) * 3.0f - 1.5f, y + static_cast<float>(i / 2) * 3.0f - 1.5f, 0xC8D0E0, 1.0f, 0.95f);
        if (Has("orbital_smelters")) // la escoria fundida corre en un hilo naranja
            for (int k = 0; k < 4; ++k) Dot(ui, x + 4.0f + static_cast<float>(k) * 2.0f, y + 3.0f + std::sin(m_time * 3.0f + static_cast<float>(k)) * 0.6f, 1.0f, ColorOf(0xFF8A30, 0.85f - 0.15f * static_cast<float>(k)));
        if (Has("orbital_shipyards")) { // una nave toma forma en el astillero oscuro: el casco crece de a un tramo
            const int stage = static_cast<int>(std::fmod(m_time / 3.0f, 4.0f)) + 1;
            ui.AddRect(kit::Rect(std::floor(x) - 6.0f, std::floor(y) + 5.0f, static_cast<float>(stage) * 2.0f, 2.0f), ColorOf(0x8A94A8, 0.95f));
            if (std::fmod(m_time * 5.0f, 1.0f) < 0.4f) Dot(ui, x - 6.0f + static_cast<float>(stage) * 2.0f, y + 5.0f, 1.0f, ColorOf(0xFFF0C0, 1.0f)); // la soldadura
        }
        if (Has("shipyard_networks")) { // otro astillero, y las lanzaderas que se pasan piezas
            const Slot s2 = OrbitSlot(m_body->center, m_body->radius * 1.55f, 7474u, 1, 0, m_time * 0.12f);
            float x2, y2;
            if (AirPt(s2.pos, x2, y2)) {
                DrawSprite(ui, kLabModule, x2, y2, 0xC8D0E0, 1.0f, 0.95f);
                const float t = Triangle(6.0f);
                Dot(ui, x + (x2 - x) * t, y + (y2 - y) * t, 1.0f, ColorOf(0xFFFFFF, 0.9f));
            }
        }
    }

    // Asteroides: la sonda que va al cinturon y vuelve, la roca estacionada, las sondas que la rodean, la red que atrapa
    // la carga y la refineria pegada a la roca.
    void DrawAsteroids() {
        UiDrawList& ui = *m_ui;
        if (Has("asteroid_prospecting")) { // hacia el Sol (donde esta el cinturon) y de vuelta con una chispa
            const float u = Triangle(22.0f);
            float x, y;
            if (AirPt(XMVectorAdd(m_body->center, XMVectorScale(m_toSun, m_body->radius * (2.0f + 10.0f * u))), x, y)) {
                Dot(ui, x, y, 2.0f, ColorOf(0xFFFFFF, 0.9f));
                if (u < 0.12f && std::fmod(m_time * 6.0f, 1.0f) < 0.5f) Glint(ui, x, y, 0.9f); // volvio con carga
            }
        }
        if (!Has("asteroid_capture")) return;
        const Slot s = OrbitSlot(m_body->center, m_body->radius * 1.9f, 9191u, 1, 0, m_time * 0.1f);
        float rx, ry;
        if (!AirPt(s.pos, rx, ry)) return;
        DrawSprite(ui, kRock, rx, ry, 0x9A9490, 1.0f, 1.0f);
        if (Has("autonomous_prospectors"))
            for (int k = 0; k < 2; ++k) {
                const float a = m_time * 1.8f + static_cast<float>(k) * 3.14f;
                Dot(ui, rx + std::cos(a) * 5.0f, ry + std::sin(a) * 3.0f, 1.0f, ColorOf(0x9CE8FF, 0.95f));
            }
        if (Has("mass_catchers")) { // una red junto a la roca; la carga llega de lejos y destella al caer en ella
            const float nx = rx - 9.0f, ny = ry - 5.0f;
            DrawSprite(ui, kNet, nx, ny, 0x9CC8FF, 1.0f, 0.9f);
            const float t = std::fmod(m_time / 6.0f, 1.0f);
            if (t < 0.5f) Dot(ui, nx - 30.0f * (1.0f - t * 2.0f), ny - 14.0f * (1.0f - t * 2.0f), 1.0f, ColorOf(0xFFFFFF, 0.9f));
            else if (t < 0.6f) Flash(ui, nx, ny, 0xFFFFFF, 1.0f - (t - 0.5f) / 0.1f);
        }
        if (Has("belt_refineries")) { // una refineria brillando pegada a la roca
            Dot(ui, rx + 3.0f, ry + 1.0f, 2.0f, ColorOf(0xFF8A30, 0.85f));
            Dot(ui, rx + 3.0f, ry - std::fmod(m_time * 2.0f, 4.0f), 1.0f, ColorOf(0xE6E6EE, 0.5f));
        }
    }

    // Petroleros de gas: el gigante lejano, la nave que va y vuelve, el cometa, las fabricas flotantes y la estacion.
    void DrawTankers() {
        if (!Has("helium3_mining")) return;
        UiDrawList& ui = *m_ui;
        const UiRect vis = UiService::VisibleRect();
        const float gx = m_body->sx + vis.width * 0.36f, gy = m_body->sy - vis.height * 0.30f; // el gigante: fijo respecto a la Tierra en pantalla
        const float u = Triangle(18.0f);
        const bool giant = Has("gas_giant_scooping");
        if (giant) { // el planeta: un disco con bandas
            Disc(gx, gy, 5.0f, ColorOf(0xD8B080, 0.95f));
            ui.AddRect(kit::Rect(std::floor(gx) - 4.0f, std::floor(gy), 8.0f, 1.0f), ColorOf(0xA07850, 0.9f));
            ui.AddRect(kit::Rect(std::floor(gx) - 3.0f, std::floor(gy) - 2.0f, 6.0f, 1.0f), ColorOf(0xF0D8B0, 0.8f));
        }
        const float tx = m_body->sx + (gx - m_body->sx) * u, ty = m_body->sy + (gy - m_body->sy) * u;
        Dot(ui, tx, ty, 2.0f, ColorOf(0xFFFFFF, 0.95f));
        Dot(ui, tx - (gx - m_body->sx) * 0.02f, ty - (gy - m_body->sy) * 0.02f, 1.0f, ColorOf(0xFFB070, 0.7f));
        if (giant && u > 0.85f) // se sumerge en las nubes: un remolino a la orilla
            for (int k = 0; k < 4; ++k) {
                const float a = m_time * 4.0f + static_cast<float>(k) * 1.57f;
                Dot(ui, gx + std::cos(a) * 6.0f, gy + std::sin(a) * 3.0f, 1.0f, ColorOf(0xFFF0C0, 0.8f));
            }
        if (Has("comet_harvesting")) { // un cometa que cae hacia el gigante y se enciende al llegar
            const float t = std::fmod(m_time / 25.0f, 1.0f);
            const float cx = gx - 70.0f * (1.0f - t), cy = gy - 40.0f * (1.0f - t);
            Dot(ui, cx, cy, 2.0f, ColorOf(0xE8F4FF, 0.95f));
            for (int k = 1; k <= 5; ++k) Dot(ui, cx - 4.0f * static_cast<float>(k), cy - 2.3f * static_cast<float>(k), 1.0f, ColorOf(0xBFE8FF, 0.7f - 0.12f * static_cast<float>(k)));
            if (t > 0.95f) Flash(ui, gx, gy, 0xBFE8FF, (1.0f - t) / 0.05f);
        }
        if (Has("helium_refineries")) // fabricas flotantes: puntos que giran sobre el gigante
            for (int k = 0; k < 3; ++k) {
                const float a = m_time * 0.9f + static_cast<float>(k) * 2.1f;
                Dot(ui, gx + std::cos(a) * 11.0f, gy + std::sin(a) * 4.0f, 1.0f, ColorOf(0xFFE6A0, 0.9f));
            }
        if (Has("gas_giant_stations")) { // una estacion: un punto brillante con su anillo
            const float sx = gx + 12.0f, sy = gy - 6.0f;
            Dot(ui, sx, sy, 2.0f, ColorOf(0xFFFFFF, 0.95f));
            for (int k = 0; k < 6; ++k) {
                const float a = static_cast<float>(k) * 1.047f + m_time;
                Dot(ui, sx + std::cos(a) * 4.0f, sy + std::sin(a) * 1.8f, 1.0f, ColorOf(0xBFE8FF, 0.8f));
            }
        }
    }

    // Telescopios espaciales: un plato enorme en orbita con un cono; los estudios estelares hacen barrer el cono entre estrellas que titilan.
    void DrawTelescopes() {
        if (!Has("planetary_telescopes")) return;
        UiDrawList& ui = *m_ui;
        const Slot s = OrbitSlot(m_body->center, m_body->radius * 2.2f, 6464u, 1, 0, m_time * 0.1f);
        float x, y;
        if (!AirPt(s.pos, x, y)) return;
        DrawSprite(ui, kDish, x, y, 0xE8ECF8, 1.0f, 0.95f);
        const float dx = x - m_body->sx, dy = y - m_body->sy, len = std::max(1.0f, std::sqrt(dx * dx + dy * dy));
        float ux = dx / len, uy = dy / len;
        if (Has("stellar_surveys")) { // el barrido: el cono se mueve y las estrellas titilan
            const float a = std::sin(m_time * 0.6f) * 0.35f;
            const float c = std::cos(a), sn = std::sin(a);
            const float rx = ux * c - uy * sn, ry = ux * sn + uy * c;
            ux = rx;
            uy = ry;
            for (int k = 0; k < 4; ++k) {
                const float d = 10.0f + static_cast<float>(k) * 6.0f;
                Dot(ui, x + ux * d + uy * (static_cast<float>(k % 2) * 4.0f - 2.0f), y + uy * d - ux * (static_cast<float>(k % 2) * 4.0f - 2.0f), 1.0f, ColorOf(0xFFFFFF, 0.3f + 0.6f * std::fabs(std::sin(m_time * 3.0f + static_cast<float>(k) * 1.7f))));
            }
        }
        ui.AddLine(x, y, x + ux * 26.0f, y + uy * 26.0f, ColorOf(0xCFE0FF, 0.22f), 1.0f);
    }

    // La red de energia del sistema: una linea punteada dorada de la Tierra hacia el Sol, con pulsos que corren.
    void DrawSystemGrid() {
        if (!Has("interplanetary_grid")) return;
        UiDrawList& ui = *m_ui;
        // UN RAYO QUE SE DESVANECE, no una linea de puntos hasta el borde de la pantalla: nace en el planeta con su luz y se apaga con la
        // distancia (el cuadrado de lo que falta), asi se lee como una corriente de energia y no como una regla cruzando el cuadro.
        constexpr int kSteps = 28;
        float px = 0, py = 0;
        bool have = false;
        for (int i = 0; i <= kSteps; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(kSteps);
            float x, y;
            if (!AirPt(XMVectorAdd(m_body->center, XMVectorScale(m_toSun, m_body->radius * (1.4f + t * 14.0f))), x, y)) {
                have = false;
                continue;
            }
            if (have) ui.AddLine(px, py, x, y, ColorOf(0xFFC850, 0.55f * (1.0f - t) * (1.0f - t)), 1.0f);
            px = x;
            py = y;
            have = true;
        }
        for (int k = 0; k < 3; ++k) {
            float x, y;
            const float t = std::fmod(m_time * 0.3f + static_cast<float>(k) / 3.0f, 1.0f);
            if (AirPt(XMVectorAdd(m_body->center, XMVectorScale(m_toSun, m_body->radius * (1.4f + t * 14.0f))), x, y)) {
                Halo(ui, x, y, 4.0f, 0xFFD890, 1.0f - t);
                Dot(ui, x, y, 2.0f, ColorOf(0xFFF2C8, 0.95f * (1.0f - t * 0.6f)));
            }
        }
    }

    // Destellos de los espejos orbitales sobre el mar, y la nave que acelera con su haz. Lee las unidades compradas: los
    // espejos son los de la tienda, esto solo agrega lo que los hace brillar.
    void DrawMirrorFlashes() {
        if (!m_game || !Has("mirror_constellations")) return;
        const InstallationDef* def = nullptr;
        for (const InstallationDef& d : m_game->installations().All())
            if (d.id == "orbital_mirror") {
                def = &d;
                break;
            }
        if (!def) return;
        const int shown = std::min(m_game->Owned(def->id), def->visibleCap);
        if (shown <= 0) return;
        UiDrawList& ui = *m_ui;
        const unsigned seed = HashString(def->id);
        for (int j = 0; j < std::min(shown, 8); ++j) {
            const Slot s = OrbitSlot(m_body->center, m_body->radius * 1.2f, seed, shown, j, m_time);
            const XMVECTOR n = XMVector3Normalize(XMVectorSubtract(s.pos, m_body->center));
            const XMVECTOR ground = XMVectorAdd(m_body->center, XMVectorScale(n, m_body->radius));
            float x, y;
            if (!NearSide(ground) || !Proj(ground, x, y)) continue;
            if (Night(ground) > 0.6f) continue; // solo donde da el Sol
            const float f = std::fmod(m_time * 0.4f + static_cast<float>(j) * 0.37f, 1.0f);
            if (f < 0.12f) Flash(ui, x, y, 0xD2E6FF, 1.0f - f / 0.12f);
        }
        if (Has("beamed_propulsion")) { // una nave acelerada por un haz de espejo: se aleja con el haz tras ella
            const Slot s = OrbitSlot(m_body->center, m_body->radius * 1.2f, seed, shown, 0, m_time);
            const XMVECTOR away = XMVector3Normalize(XMVectorSubtract(s.pos, m_body->center));
            const float t = std::fmod(m_time / 9.0f, 1.0f);
            float x0, y0, x1, y1;
            if (AirPt(s.pos, x0, y0) && AirPt(XMVectorAdd(s.pos, XMVectorScale(away, m_body->radius * t * t * 9.0f)), x1, y1)) {
                ui.AddLine(x0, y0, x1, y1, ColorOf(0xFFF0C8, 0.35f * (1.0f - t)), 1.0f);
                Dot(ui, x1, y1, 2.0f, ColorOf(0xFFFFFF, 1.0f - t * 0.6f));
            }
        }
    }
};
