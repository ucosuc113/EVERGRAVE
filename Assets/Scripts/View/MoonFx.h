#pragma once

// MoonFx: lo que las TECNOLOGIAS de las eras 6 y 7 (y la 4 que abre la Luna) construyen en la superficie y alrededor de la
// Luna. Hermano de EarthFx: la misma red de nodos (BodyNet, en FxKit.h) y la misma regla, una tecnologia sin investigar no
// dibuja nada. Las familias salen de la hoja Families del Excel (moon_*, domes, foundry, mine...); ver docs/plan-visual.md.
//
// A diferencia de la Tierra, la Luna no tiene atmosfera ni noche corta: lo que importa es el lado que mira al Sol (paneles,
// destellos) y el que no (luces, hornos, domos). Y TODO lo que se comunica con otro mundo - el haz de energia, el
// ascensor, los transbordadores - va de la Luna a la Tierra o al Sol, asi que aqui entra donde estan los dos.
//
// Los nodos de uso, a proposito FIJOS (el mismo nodo para la misma cosa): una cosa no cambia de sitio al investigar otra.

#include "FxKit.h"

class MoonFx : public BodyNet {
public:
    MoonFx() { BuildNet(20, 91u, 30, 1.2f); }

    void Draw(UiDrawList& ui, const Cam& cam, const Body& moon, const GameState& game, const GameObject& moonObj, const XMVECTOR& sunCenter,
              const XMVECTOR& earthCenter, float time) {
        Begin(ui, cam, moon, game, moonObj, sunCenter, time);
        m_earthC = earthCenter;
        m_sunC = sunCenter;
        DrawSurvey();
        DrawLanders();
        DrawMines();
        DrawBuild();
        DrawFoundry();
        DrawDomes();
        DrawGrid();
        DrawRail();
        DrawFusion();
        DrawRefining();
        DrawSolar();
        DrawObservatories();
        DrawMassDriver();
        DrawLasers();
        DrawStations();
        DrawTraffic();
        DrawSupply();
    }

private:
    // ------------------------------------------------------------------ ayudas
    // Un punto que sube de un nodo, en pantalla (hacia afuera de la Luna).
    void Pulse(float x, float y, float tx, float ty, float t, std::uint32_t rgb, float a, float size = 1.0f) const { Dot(*m_ui, x + (tx - x) * t, y + (ty - y) * t, size, ColorOf(rgb, a)); }
    // El otro mundo en pantalla: la Tierra o el Sol, solo si la camara lo ve.
    bool Far(const XMVECTOR& world, float& x, float& y) const { return Proj(world, x, y); }
    bool MoonScreen(float& x, float& y) const { return Proj(m_body->center, x, y); }

    // ------------------------------------------------------------------ levantamiento
    // Un orbitador con una linea de barrido; el mapeo tine parches; la red sismica pulsa; la simulacion es una rejilla.
    void DrawSurvey() {
        if (!Has("lunar_survey_program")) return;
        UiDrawList& ui = *m_ui;
        const Slot s = OrbitSlot(m_body->center, m_body->radius * 1.2f, 3131u, 1, 0, m_time * 0.6f);
        float sx, sy, vz;
        if (m_cam->Project(s.pos, sx, sy, vz) && !Occluded(*m_body, sx, sy, vz)) {
            g_mod = ModOf(DepthCue(vz));
            DrawSprite(ui, kProbe, sx, sy, 0xB8C0D0, 1.0f, 0.95f);
            float cx, cy;
            if (MoonScreen(cx, cy)) ui.AddLine(sx, sy, cx + (sx - cx) * 0.55f, cy + (sy - cy) * 0.55f, ColorOf(0x9CE8FF, 0.35f), 1.0f); // el barrido
            g_mod = FxMod{};
        }
        if (Has("lunar_geology")) // parches con tinte: lo que ya se mapeo
            for (int i : {3, 8, 14}) {
                float x, y, night;
                if (NodeAt(i, x, y, night)) Dot(ui, x, y, 7.0f, ColorOf(i % 2 ? 0xC08850 : 0x6A8AB8, 0.13f));
            }
        if (Has("moonquake_network")) // sismografos: puntos que pulsan cuando la Luna retumba
            for (int i = 0; i < NodeCount(); i += 2) {
                float x, y, night;
                if (!NodeAt(i, x, y, night)) continue;
                const float quake = std::fmod(m_time / 9.0f, 1.0f) < 0.12f ? 1.0f : 0.0f;
                Dot(ui, x, y, 1.0f, ColorOf(0xFFFFFF, 0.25f + 0.6f * quake));
            }
        if (Has("lunar_simulation")) { // la Luna simulada primero: una rejilla fina de meridianos
            for (int k = 0; k < 6; ++k) {
                float px = 0, py = 0;
                bool have = false;
                const float lon = static_cast<float>(k) * kPi / 3.0f;
                for (int i = 0; i <= 16; ++i) {
                    const float lat = -1.4f + 2.8f * static_cast<float>(i) / 16.0f;
                    const XMVECTOR d = DirOf(lat, lon);
                    float x, y;
                    if (!NearSide(World(d, 0.0f)) || !Proj(World(d, 0.002f), x, y)) {
                        have = false;
                        continue;
                    }
                    if (have) ui.AddLine(px, py, x, y, ColorOf(0x8CE0FF, 0.12f), 1.0f);
                    px = x;
                    py = y;
                    have = true;
                }
            }
        }
    }

    // ------------------------------------------------------------------ alunizajes
    void DrawLanders() {
        if (!Has("lunar_landers")) return;
        UiDrawList& ui = *m_ui;
        float x, y, night;
        if (!NodeAt(0, x, y, night)) return;
        ui.AddRect(kit::Rect(std::floor(x), std::floor(y), 2.0f, 2.0f), ColorOf(0xD8DCE8, 0.95f));
        if (std::fmod(m_time, 2.0f) < 0.5f) ui.AddRect(kit::Rect(std::floor(x), std::floor(y) - 2.0f, 1.0f, 1.0f), ColorOf(0xFF4040)); // baliza
        if (!Has("lunar_rovers")) return;
        int made = 0;
        for (const Edge& e : m_edges) { // los rovers salen del sitio por las aristas que lo tocan
            if (e.a != 0 && e.b != 0) continue;
            for (int k = 0; k < 2; ++k) {
                float t = std::fmod(m_time / 9.0f + static_cast<float>(made) * 0.3f + static_cast<float>(k) * 0.5f, 1.0f);
                t = t < 0.5f ? t * 2.0f : 2.0f - t * 2.0f; // van y vuelven
                float rx, ry, n;
                if (EdgePoint(e, e.a == 0 ? t * 0.7f : 1.0f - t * 0.7f, rx, ry, n)) Dot(ui, rx, ry, 1.0f, ColorOf(0xFFFFFF, 0.9f));
            }
            if (++made >= 3) break;
        }
    }

    // ------------------------------------------------------------------ minas
    void DrawMines() {
        if (!Has("regolith_excavation")) return;
        UiDrawList& ui = *m_ui;
        const int sites = Has("lunar_mining_rigs") ? 4 : 2;
        static const int idx[] = {3, 7, 11, 15};
        for (int s = 0; s < sites; ++s) {
            float x, y, night;
            if (!NodeAt(idx[s], x, y, night)) continue;
            const int pits = Has("deep_regolith_mining") ? 7 : 1; // la mina profunda: un panal de hoyos
            for (int p = 0; p < pits; ++p) {
                const float a = static_cast<float>(p) * 2.0f * kPi / 6.0f;
                const float ox = p == 0 ? 0.0f : std::cos(a) * 3.0f, oy = p == 0 ? 0.0f : std::sin(a) * 2.0f;
                ui.AddRect(kit::Rect(std::floor(x + ox), std::floor(y + oy), 2.0f, 2.0f), ColorOf(0x14161C, 0.9f));
            }
            float tx, ty;
            if (UpFrom(idx[s], 0.06f, tx, ty)) // el polvo: un penacho palido que sube y se baja
                for (int k = 0; k < 3; ++k) {
                    const float f = std::fmod(m_time * 0.5f + static_cast<float>(k) / 3.0f + static_cast<float>(s) * 0.2f, 1.0f);
                    Pulse(x, y, tx, ty, f, 0xC8D0DC, (1.0f - f) * 0.5f);
                }
        }
        if (Has("polar_ice_mining")) // el hielo polar: un resplandor blanco azulado dentro de un crater
            for (const XMVECTOR& d : {DirOf(1.32f, 0.5f), DirOf(-1.32f, 2.4f)}) {
                float x, y;
                if (NearSide(World(d, 0.0f)) && Proj(World(d, 0.002f), x, y)) Dot(ui, x, y, 3.0f, ColorOf(0xBFE8FF, 0.45f + 0.25f * std::sin(m_time * 2.0f)));
            }
    }

    // ------------------------------------------------------------------ construccion con regolito
    void DrawBuild() {
        if (!Has("regolith_bricks")) return;
        UiDrawList& ui = *m_ui;
        float x, y, night;
        if (!NodeAt(0, x, y, night)) return;
        for (int k = 0; k < 3; ++k) ui.AddRect(kit::Rect(std::floor(x) + 3.0f + static_cast<float>(k) * 2.0f, std::floor(y) + 2.0f, 1.0f, 1.0f), ColorOf(0xC8B090, 0.9f)); // ladrillos
        if (Has("regolith_concrete")) ui.AddRect(kit::Rect(std::floor(x) + 2.0f, std::floor(y) + 4.0f, 8.0f, 1.0f), ColorOf(0xA8A090, 0.9f)); // una pista
        if (Has("regolith_shielding")) // el polvo apilado encima de los domos
            for (int i : {1, 4, 8}) {
                float dx, dy, n;
                if (NodeAt(i, dx, dy, n)) Dot(ui, dx, dy + 2.0f, 4.0f, ColorOf(0x8A8478, 0.35f));
            }
    }

    // ------------------------------------------------------------------ industria
    void DrawFoundry() {
        if (!Has("sintering_plants")) return;
        UiDrawList& ui = *m_ui;
        const int sites = Has("lunar_heavy_industry") ? 5 : Has("lunar_foundries") ? 4 : 2;
        static const int idx[] = {2, 6, 10, 14, 18};
        const bool white = Has("lunar_alloys");
        const std::uint32_t col = white ? 0xFFD890 : 0xFF9646;
        for (int s = 0; s < sites; ++s) {
            float x, y, night;
            if (!NodeAt(idx[s], x, y, night)) continue;
            const float flick = 0.6f + 0.4f * std::sin(m_time * 9.0f + static_cast<float>(s)) * std::sin(m_time * 5.3f + static_cast<float>(s) * 2.0f);
            Dot(ui, x, y, 4.0f, ColorOf(col, 0.14f * flick));
            Dot(ui, x, y, 2.0f, ColorOf(col, (0.35f + 0.55f * night) * flick));
            if (Has("lunar_foundries")) // los hornos en fila
                for (int k = 1; k <= 2; ++k) Dot(ui, x + static_cast<float>(k) * 3.0f, y, 1.0f, ColorOf(col, (0.3f + 0.5f * night) * flick));
            if (Has("robotic_foundry_crews")) { // robots que van y vienen
                const float t = std::fmod(m_time * 0.4f + static_cast<float>(s) * 0.27f, 1.0f);
                Dot(ui, x - 4.0f + t * 8.0f, y + 3.0f, 1.0f, ColorOf(0xFFFFFF, 0.8f));
            }
            if (Has("sintered_metals")) { // piezas limpias que salen del horno
                const float t = std::fmod(m_time * 0.3f + static_cast<float>(s) * 0.4f, 1.0f);
                Dot(ui, x + t * 5.0f, y - 2.0f - t * 2.0f, 1.0f, ColorOf(0xFFF0C0, 1.0f - t));
            }
            if (Has("lunar_industrial_parks")) // filas de fabricas bajo los domos
                for (int k = 0; k < 3; ++k) ui.AddRect(kit::Rect(std::floor(x) - 3.0f + static_cast<float>(k) * 3.0f, std::floor(y) + 4.0f, 2.0f, 1.0f), ColorOf(0xFFE0A0, 0.3f + 0.5f * night));
            if (Has("lunar_composites")) ui.AddRect(kit::Rect(std::floor(x) + 5.0f, std::floor(y) + 3.0f, 3.0f, 2.0f), ColorOf(0xE0D8C8, 0.8f)); // vigas palidas apiladas
        }
        if (Has("factory_federation")) // la federacion: lanzaderas entre las filas de hornos
            for (int s = 0; s + 1 < sites; ++s) {
                float x0, y0, n0, x1, y1, n1;
                if (!NodeAt(idx[s], x0, y0, n0) || !NodeAt(idx[s + 1], x1, y1, n1)) continue;
                Pulse(x0, y0, x1, y1, std::fmod(m_time * 0.3f + static_cast<float>(s) * 0.31f, 1.0f), 0xFFE0A0, 0.9f);
            }
    }

    // ------------------------------------------------------------------ domos y pueblos
    void DrawDomes() {
        if (!Has("lunar_domes")) return;
        UiDrawList& ui = *m_ui;
        static const int idx[] = {1, 4, 8, 12, 16};
        const int count = Has("lunar_megabase") ? 5 : Has("lunar_settlement_ring") ? 4 : 3;
        float xs[5], ys[5];
        bool ok[5] = {false, false, false, false, false};
        for (int s = 0; s < count; ++s) {
            float x, y, night;
            if (!NodeAt(idx[s], x, y, night)) continue;
            xs[s] = x;
            ys[s] = y;
            ok[s] = true;
            DrawSprite(ui, kDome, x, y, 0xC8E6D2, 0.7f + 0.4f * (1.0f - night), 0.95f);
            Dot(ui, x, y + 0.5f, 1.0f, ColorOf(0x70E070, 0.6f)); // el jardin de adentro
            if (Has("lunar_settlement")) Dot(ui, x, y, 7.0f, ColorOf(0xFFE6A0, 0.10f + 0.18f * night)); // el pueblo brilla de noche
            if (Has("underground_labs") && night > 0.4f) Dot(ui, x + 3.0f, y + 3.0f, 1.0f, ColorOf(0xFFF0C0, 0.5f)); // luces bajo tierra
        }
        if (Has("lunar_settlement_ring")) // el anillo de pueblos: cada domo ligado con el siguiente
            for (int s = 0; s < count; ++s) {
                const int t = (s + 1) % count;
                if (ok[s] && ok[t]) ui.AddLine(xs[s], ys[s], xs[t], ys[t], ColorOf(0xFFE6A0, 0.28f), 1.0f);
            }
        if (Has("lunar_megabase")) // todo junto: un halo comun
            for (int s = 0; s < count; ++s)
                if (ok[s]) Dot(ui, xs[s], ys[s], 5.0f, ColorOf(0xC8E6D2, 0.12f));
    }

    // ------------------------------------------------------------------ red electrica lunar
    void DrawGrid() {
        if (!Has("lunar_power_grid")) return;
        UiDrawList& ui = *m_ui;
        const int n = Has("lunar_power_network") ? static_cast<int>(m_edges.size()) : std::min(8, static_cast<int>(m_edges.size()));
        for (int k = 0; k < n; ++k) {
            const Edge& e = m_edges[static_cast<std::size_t>(k)];
            float px = 0, py = 0;
            bool have = false;
            for (int i = 0; i <= e.samples; ++i) {
                float x, y, night;
                if (!EdgePoint(e, static_cast<float>(i) / static_cast<float>(e.samples), x, y, night)) {
                    have = false;
                    continue;
                }
                // De noche, como la de la Tierra: una linea de luz solo se ve donde es de noche (de dia era una jaula sobre el regolito).
                if (have && night > 0.3f) ui.AddLine(px, py, x, y, ColorOf(0xFFD278, 0.04f + 0.6f * night * night), 1.0f);
                px = x;
                py = y;
                have = true;
            }
            if (Has("lunar_energy_routing")) { // los pulsos eligen quien recibe luz y cuando
                float x, y, night;
                if (EdgePoint(e, std::fmod(m_time / 5.0f + static_cast<float>(k) * 0.37f, 1.0f), x, y, night)) Dot(ui, x, y, 2.0f, ColorOf(0xFFF2C8, 0.3f + 0.7f * night));
            }
        }
        if (Has("lunar_industrial_complex")) // todo el lado nocturno brilla como un mundo iluminado
            for (int i = 0; i < NodeCount(); ++i) {
                float x, y, night;
                if (NodeAt(i, x, y, night)) Dot(ui, x, y, 7.0f, ColorOf(0xFFE6A0, 0.05f + 0.12f * night));
            }
    }

    // ------------------------------------------------------------------ riel
    void DrawRail() {
        if (!Has("lunar_rail_network")) return;
        UiDrawList& ui = *m_ui;
        const int ei = std::min(static_cast<int>(m_edges.size()) - 1, 20);
        if (ei < 0) return;
        const Edge& e = m_edges[static_cast<std::size_t>(ei)];
        float px = 0, py = 0;
        bool have = false;
        for (int i = 0; i <= e.samples; ++i) {
            float x, y, night;
            if (!EdgePoint(e, static_cast<float>(i) / static_cast<float>(e.samples), x, y, night)) {
                have = false;
                continue;
            }
            if (have) ui.AddLine(px, py, x, y, ColorOf(0xC8E0FF, 0.3f), 1.0f);
            px = x;
            py = y;
            have = true;
        }
        float x, y, n;
        for (int k = 0; k < 2; ++k)
            if (EdgePoint(e, std::fmod(m_time / 6.0f + static_cast<float>(k) * 0.5f, 1.0f), x, y, n)) Dot(ui, x, y, 2.0f, ColorOf(0xFFFFFF, 0.9f));
        if (Has("lunar_fuel_depots"))
            for (int i : {e.a, e.b})
                if (NodeAt(i, x, y, n)) Dot(ui, x, y, 2.0f, ColorOf(0xFFC878, 0.9f)); // los depositos de combustible
        if (Has("logistics_hubs")) { // bodegas que se avisan entre si
            float x0, y0, n0, x1, y1, n1;
            if (NodeAt(e.a, x0, y0, n0) && NodeAt(e.b, x1, y1, n1)) Pulse(x0, y0, x1, y1, std::fmod(m_time * 0.7f, 1.0f), 0xFFFFFF, 0.9f);
        }
    }

    // ------------------------------------------------------------------ fusion y reactores
    void DrawFusion() {
        UiDrawList& ui = *m_ui;
        if (Has("helium3_extraction")) // gas horneado: estelas de polvo
            for (int i : {5, 9}) {
                float x, y, night;
                if (NodeAt(i, x, y, night)) Dot(ui, x + std::fmod(m_time * 2.0f + static_cast<float>(i), 5.0f) - 2.0f, y + 2.0f, 1.0f, ColorOf(0xC8D0DC, 0.4f));
            }
        if (Has("lunar_fusion_plant")) {
            float x, y, night;
            if (NodeAt(5, x, y, night)) { // un anillo de plasma
                const float calm = Has("sustained_lunar_fusion") ? 0.0f : 0.3f * std::sin(m_time * 11.0f); // sin parpadeo cuando es sostenida
                for (int k = 0; k < 10; ++k) {
                    const float a = static_cast<float>(k) * 2.0f * kPi / 10.0f + m_time * 0.9f;
                    Dot(ui, x + std::cos(a) * 3.5f, y - 4.0f + std::sin(a) * 1.8f, 1.0f, ColorOf(0xFFFFFF, 0.7f + calm));
                }
            }
        }
        if (Has("lunar_reactors"))
            for (int i : {5, 9, 13}) {
                float x, y, night;
                if (!NodeAt(i, x, y, night)) continue;
                DrawSprite(ui, kDome, x, y, 0x6EC8FF, 0.9f, 0.95f);
                Dot(ui, x, y, 5.0f, ColorOf(0x6EC8FF, 0.12f + 0.06f * std::sin(m_time * 1.5f + static_cast<float>(i))));
                if (Has("magnetic_shielding")) Dot(ui, x, y, 9.0f, ColorOf(0x9CC8FF, 0.07f)); // el aura que desvia la radiacion
            }
        if (Has("lunar_heat_pipes")) { // tubos que llevan el calor del dia a la noche
            const int ei = std::min(static_cast<int>(m_edges.size()) - 1, 3);
            if (ei >= 0) {
                const Edge& e = m_edges[static_cast<std::size_t>(ei)];
                float x, y, n;
                for (int k = 0; k < 3; ++k)
                    if (EdgePoint(e, std::fmod(m_time * 0.4f + static_cast<float>(k) / 3.0f, 1.0f), x, y, n)) Dot(ui, x, y, 1.0f, ColorOf(0xFF8A40, 0.8f));
            }
        }
    }

    // ------------------------------------------------------------------ refinado
    void DrawRefining() {
        if (!Has("regolith_refining")) return;
        UiDrawList& ui = *m_ui;
        for (int i : {13, 17}) {
            float x, y, night;
            if (!NodeAt(i, x, y, night)) continue;
            Dot(ui, x, y, 3.0f, ColorOf(0xFFB060, 0.2f + 0.3f * night));
            float tx, ty;
            if (UpFrom(i, 0.07f, tx, ty))
                for (int k = 0; k < 3; ++k) Pulse(x, y, tx, ty, std::fmod(m_time * 0.4f + static_cast<float>(k) / 3.0f, 1.0f), 0xE6E6EE, 0.4f);
            if (Has("volatile_harvesting")) // el gas sale de los crateres oscuros
                Dot(ui, x + 3.0f, y - std::fmod(m_time * 1.5f + static_cast<float>(i), 3.0f), 1.0f, ColorOf(0xBFE8FF, 0.6f));
        }
        if (Has("deep_vein_mining")) // vetas oscuras que brillan bajo la superficie
            for (int k = 2; k < 6 && k < static_cast<int>(m_edges.size()); ++k) {
                const Edge& e = m_edges[static_cast<std::size_t>(k)];
                float x, y, n;
                if (EdgePoint(e, 0.5f, x, y, n)) Dot(ui, x, y, 2.0f, ColorOf(0xFF7020, 0.25f + 0.2f * std::sin(m_time + static_cast<float>(k))));
            }
    }

    // ------------------------------------------------------------------ energia solar lunar
    void DrawSolar() {
        if (!Has("lunar_solar_arrays")) return;
        UiDrawList& ui = *m_ui;
        static const int idx[] = {2, 5, 9, 13, 17};
        float xs[5], ys[5];
        bool ok[5] = {false, false, false, false, false};
        for (int s = 0; s < 5; ++s) {
            float x, y, night;
            if (!NodeAt(idx[s], x, y, night)) continue;
            x += 4.0f;
            xs[s] = x;
            ys[s] = y;
            ok[s] = true;
            ui.AddRect(kit::Rect(std::floor(x), std::floor(y), 3.0f, 2.0f), ColorOf(0x2A4A8A, 1.0f - 0.7f * night));
            if (night < 0.5f && std::sin(m_time * 1.7f + static_cast<float>(s) * 1.3f) > 0.92f) Glint(ui, x + 1.0f, y, 0.9f);
        }
        if (Has("beamed_surface_power"))
            for (int s = 0; s + 1 < 5; ++s)
                if (ok[s] && ok[s + 1]) Pulse(xs[s], ys[s], xs[s + 1], ys[s + 1], std::fmod(m_time * 0.5f + static_cast<float>(s) * 0.2f, 1.0f), 0xFFE08A, 0.7f);
        if (Has("lunar_solar_belt")) // el cinturon: puntos oscuros alrededor del ecuador
            for (int k = 0; k < 64; ++k) {
                const XMVECTOR d = DirOf(0.0f, static_cast<float>(k) * 2.0f * kPi / 64.0f);
                float x, y;
                if (NearSide(World(d, 0.0f)) && Proj(World(d, 0.003f), x, y)) ui.AddRect(kit::Rect(std::floor(x), std::floor(y), 1.0f, 1.0f), ColorOf(k % 8 == 0 ? 0xA8C8FF : 0x2A4A8A, 0.9f));
            }
        if (Has("lunar_solar_rings")) // anillos de paneles alrededor de la Luna, que mandan la energia hacia afuera
            for (int r = 0; r < 2; ++r) {
                const XMMATRIX rot = XMMatrixRotationX(0.5f + static_cast<float>(r) * 0.8f) * XMMatrixRotationY(0.9f + static_cast<float>(r) * 1.4f);
                for (int k = 0; k < 60; ++k) {
                    const float a = static_cast<float>(k) * 2.0f * kPi / 60.0f + m_time * 0.15f * (r ? -1.0f : 1.0f);
                    const XMVECTOR v = XMVector3TransformNormal(XMVectorSet(std::cos(a), 0, std::sin(a), 0), rot);
                    float x, y, z;
                    if (!m_cam->Project(XMVectorAdd(m_body->center, XMVectorScale(v, m_body->radius * (1.18f + 0.08f * static_cast<float>(r)))), x, y, z) || Occluded(*m_body, x, y, z)) continue;
                    ui.AddRect(kit::Rect(std::floor(x), std::floor(y), 1.0f, 1.0f), ColorOf(k % 6 == 0 ? 0xFFFFFF : 0x6A8AD8, 0.85f));
                }
            }
    }

    // ------------------------------------------------------------------ observatorios
    void DrawObservatories() {
        if (!Has("lunar_telescopes")) return;
        UiDrawList& ui = *m_ui;
        float x, y, night;
        if (NodeAt(19, x, y, night)) {
            DrawSprite(ui, kDome, x, y, 0xF0F4FF, 1.0f, 0.95f);
            float tx, ty;
            if (UpFrom(19, 0.3f, tx, ty)) ui.AddLine(x, y - 1.0f, tx, ty, ColorOf(0xFFE0A0, 0.3f), 1.0f); // el cono, hacia el Sol, tapado por un disco
        }
        if (Has("far_side_observatory") && NodeAt(0, x, y, night)) Dot(ui, x + 6.0f, y - 4.0f, 2.0f, ColorOf(0xBFE8FF, 0.3f + 0.2f * std::sin(m_time))); // el de la cara oculta, apenas
        if (Has("lunar_comms_relay") && NodeAt(12, x, y, night)) { // un mastil con pulsos hacia la Tierra
            ui.AddRect(kit::Rect(std::floor(x), std::floor(y) - 2.0f, 1.0f, 3.0f), ColorOf(0xE0E4F0, 0.9f));
            float ex, ey;
            if (Far(m_earthC, ex, ey)) {
                const float dx = ex - x, dy = ey - y, len = std::max(1.0f, std::sqrt(dx * dx + dy * dy));
                for (int k = 0; k < 3; ++k) Pulse(x, y - 2.0f, x + dx / len * 18.0f, y - 2.0f + dy / len * 18.0f, std::fmod(m_time * 0.8f + static_cast<float>(k) / 3.0f, 1.0f), 0xBFE8FF, 0.8f);
            }
        }
    }

    // ------------------------------------------------------------------ lanzador de masa
    void DrawMassDriver() {
        if (!Has("mass_drivers")) return;
        UiDrawList& ui = *m_ui;
        const int length = Has("lunar_mass_drivers") ? 14 : Has("mass_driver_design") ? 10 : 6;
        const XMVECTOR p = m_nodes[6 % m_nodes.size()].dir;
        const XMVECTOR e = XMVector3Normalize(XMVector3Cross(XMVectorSet(0, 1, 0, 0), p));
        auto at = [&](float phi) { return World(XMVectorAdd(XMVectorScale(p, std::cos(phi)), XMVectorScale(e, std::sin(phi))), 0.003f); };
        if (!NearSide(at(0.0f))) return;
        float hx = 0, hy = 0;
        for (int k = 0; k < length; ++k) {
            float x, y;
            if (!Proj(at(0.022f * static_cast<float>(k)), x, y)) continue;
            Dot(ui, x, y, 1.0f, ColorOf(0x9CC8FF, 0.5f + 0.4f * std::sin(m_time * 6.0f - static_cast<float>(k) * 0.8f)));
            hx = x;
            hy = y;
        }
        if (!Has("lunar_mass_drivers")) return;
        float sx, sy;
        if (!Far(m_sunC, sx, sy)) return;
        const float dx = sx - hx, dy = sy - hy, len = std::max(1.0f, std::sqrt(dx * dx + dy * dy));
        for (int k = 0; k < 5; ++k) { // la carga sale hacia el Sol y se apaga
            const float t = std::fmod(m_time * 0.45f + static_cast<float>(k) / 5.0f, 1.0f);
            Dot(ui, hx + dx / len * t * 80.0f, hy + dy / len * t * 80.0f, 1.0f, ColorOf(0xFFF0AA, 1.0f - t));
        }
    }

    // ------------------------------------------------------------------ laseres
    void DrawLasers() {
        if (!Has("surface_laser_arrays")) return;
        UiDrawList& ui = *m_ui;
        const std::uint32_t col = Has("spectral_tuning_lunar") ? 0xFFF0C8 : 0xFF6070;
        float ex = 0, ey = 0;
        const bool toEarth = Has("beam_calibration") && Far(m_earthC, ex, ey);
        for (int i : {4, 12, 18}) {
            float x, y, night, tx, ty;
            if (!NodeAt(i, x, y, night)) continue;
            Dot(ui, x, y, 1.0f, ColorOf(col, 0.9f));
            if (!UpFrom(i, 0.25f, tx, ty)) continue;
            ui.AddLine(x, y, tx, ty, ColorOf(col, 0.55f), 1.0f);
            if (toEarth) { // los haces se juntan en uno y apuntan a la Tierra (un tramo corto: la Tierra esta muy lejos)
                const float dx = ex - tx, dy = ey - ty, len = std::max(1.0f, std::sqrt(dx * dx + dy * dy));
                ui.AddLine(tx, ty, tx + dx / len * 14.0f, ty + dy / len * 14.0f, ColorOf(0xFFFFFF, 0.5f), 1.0f);
            }
        }
    }

    // ------------------------------------------------------------------ estaciones alrededor de la Luna
    void DrawStations() {
        if (!Has("lunar_gateway")) return;
        UiDrawList& ui = *m_ui;
        const int n = Has("lunar_orbit_stations") ? 3 : 1;
        for (int j = 0; j < n; ++j) {
            const Slot s = OrbitSlot(m_body->center, m_body->radius * 1.28f, 8181u, n, j, m_time * 0.3f);
            float sx, sy, vz;
            if (!m_cam->Project(s.pos, sx, sy, vz) || Occluded(*m_body, sx, sy, vz)) continue;
            const float lit = Eclipse(s.pos, m_body->center, m_toSun, m_body->radius);
            g_mod = ModOf(DepthCue(vz));
            DrawSprite(ui, kStationCore, sx, sy, 0xB8C0D0, lit, 1.0f);
            if (j == 0) DrawSprite(ui, kTug, sx + 4.0f, sy + 2.0f, 0xB8C0D0, lit, 0.9f); // una nave acoplada
            else Dot(ui, sx, sy + 3.0f, 1.0f, ColorOf(0xFFE6A0, 0.7f * lit)); // la ventana del salon
            g_mod = FxMod{};
        }
    }

    // ------------------------------------------------------------------ trafico entre mundos
    void DrawTraffic() {
        float mx, my, ex, ey;
        if (!MoonScreen(mx, my) || !Far(m_earthC, ex, ey)) return;
        UiDrawList& ui = *m_ui;
        const float dx = ex - mx, dy = ey - my, len = std::max(1.0f, std::sqrt(dx * dx + dy * dy));
        const float ux = dx / len, uy = dy / len, r = m_body->sr;
        auto along = [&](float t, float& x, float& y) { // de un limbo al otro, con un arco leve
            const float d = r * 1.2f + (len - r * 2.2f) * t;
            x = mx + ux * d - uy * std::sin(kPi * t) * 5.0f;
            y = my + uy * d + ux * std::sin(kPi * t) * 5.0f;
        };
        if (Has("earth_moon_elevator")) { // el cable: una linea muy fina entre los dos mundos
            float x0, y0, x1, y1;
            along(0.0f, x0, y0);
            along(1.0f, x1, y1);
            ui.AddLine(x0, y0, x1, y1, ColorOf(0xC8C8D7, 0.25f), 1.0f);
            for (int k = 0; k < 2; ++k) {
                float x, y;
                along(std::fmod(m_time * 0.05f + static_cast<float>(k) * 0.5f, 1.0f), x, y);
                Dot(ui, x, y, 2.0f, ColorOf(0xFFFFFF, 0.9f));
            }
        }
        if (Has("power_export")) { // la energia: pulsos dorados de la Luna a la Tierra
            const int count = Has("first_export") ? 5 : 2;
            for (int k = 0; k < count; ++k) {
                float x, y;
                along(std::fmod(m_time * 0.25f + static_cast<float>(k) / static_cast<float>(count), 1.0f), x, y);
                Dot(ui, x, y, Has("first_export") ? 2.0f : 1.0f, ColorOf(0xFFE08A, 0.85f));
            }
        }
        const int ships = Has("cislunar_highway") ? 3 : Has("earth_moon_ferry") ? 2 : Has("cislunar_shuttles") ? 1 : 0;
        const bool orderly = Has("moon_orbit_tuning");
        for (int k = 0; k < ships; ++k) {
            const float off = orderly ? static_cast<float>(k) / static_cast<float>(ships) : Hash01(11u, static_cast<unsigned>(k));
            float t = std::fmod(m_time * 0.08f + off, 1.0f);
            const bool back = Has("earth_return_cargo") && k % 2 == 1; // los que vuelven, llenos de metal
            if (back) t = 1.0f - t;
            float x, y;
            along(t, x, y);
            Dot(ui, x, y, 2.0f, ColorOf(back ? 0xFFC878 : 0xD2DCF0, 0.95f));
            if (Has("earth_moon_ferry")) Dot(ui, x, y + 1.0f, 1.0f, ColorOf(0xFFE6A0, 0.8f)); // la ventana
        }
    }

    // ------------------------------------------------------------------ la linea de suministro del enjambre
    // Paneles hechos en la Luna que se van hacia el Sol: de una hilera de puntos a una corriente.
    void DrawSupply() {
        if (!Has("lunar_assembly_lines")) return;
        float mx, my, sx, sy;
        if (!MoonScreen(mx, my) || !Far(m_sunC, sx, sy)) return;
        UiDrawList& ui = *m_ui;
        const float dx = sx - mx, dy = sy - my, len = std::max(1.0f, std::sqrt(dx * dx + dy * dy));
        const int count = 2 + (Has("lunar_glass") ? 1 : 0) + (Has("lunar_shipyards") ? 4 : 0) + (Has("laser_ranging") ? 1 : 0);
        const float spread = Has("lunar_navigation") ? 0.0f : 3.0f; // con navegacion, un solo carril ordenado
        for (int k = 0; k < count; ++k) {
            const float t = std::fmod(m_time * 0.18f + static_cast<float>(k) / static_cast<float>(count), 1.0f);
            const float off = (Hash01(31u, static_cast<unsigned>(k)) - 0.5f) * spread;
            const float d = m_body->sr * 1.1f + t * 70.0f;
            Dot(ui, mx + dx / len * d - dy / len * off, my + dy / len * d + dx / len * off, 1.0f, ColorOf(0xFFE6A0, 0.9f * (1.0f - t)));
        }
    }

    XMVECTOR m_earthC = XMVectorZero();
    XMVECTOR m_sunC = XMVectorZero();
};
