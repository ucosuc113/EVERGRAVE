#pragma once

// SunFx: lo que las TECNOLOGIAS de las eras 5 y 8 (y las de las 6 y 7 que alimentan al enjambre) ponen ALREDEDOR DEL SOL: de
// unos pocos puntos de dos pixeles girando lejos, a anillos, nubes, corona y una cobertura que apaga la estrella. Hermano de
// EarthFx y MoonFx (misma regla: una tecnologia sin investigar no dibuja), pero sin red de nodos: el Sol no tiene suelo, lo
// que rodea al Sol son ORBITAS. Las familias salen de la hoja Families (swarm, statite, dyson_ring, mercury, sun_*...).
//
// Se dibuja en las tres vistas donde el Sol se ve (Tierra, Luna y la suya), siempre alrededor de SU cuerpo en pantalla
// (`sun`, que WorldFx ya arma): en la vista de la Tierra el Sol es chico y lejano, asi que los radios van en radios SOLARES y
// los puntos de 1 px; en la suya son de 2 px. Lo que pasa detras del disco se tapa con el.
//
// KEPLER: lo de dentro gira mas rapido (velocidad ~ r^-1,5). Es lo que hace que el enjambre se lea como un fluido y no
// como un disco rigido, igual que el cinturon de asteroides.
//
// Los radios, en radios solares: Mercurio 2,6; el enjambre lejano 3,4 a 5,6; el anillo de Dyson 2,4; el interior 1,45 a 1,8;
// los instrumentos y la corona, entre 1,1 y 1,4.

#include "FxKit.h"

class SunFx {
public:
    void Draw(UiDrawList& ui, const Cam& cam, const Body& sun, const GameState& game, float time, bool big, const Body* earth) {
        m_ui = &ui;
        m_cam = &cam;
        m_sun = &sun;
        m_tree = &game.tree();
        m_time = time;
        m_big = big;
        m_sz = 2.0f; // dos pixeles, tambien de lejos: es lo que pide el plan para el Sol de la era 5
        // Una llamarada cada 9 s; las tecnologias que la predicen avisan antes (se atenua el enjambre un instante).
        const float period = 9.0f;
        const float t = std::fmod(time, period);
        m_flareN = static_cast<unsigned>(std::floor(time / period));
        m_flareT = t;
        m_dim = (Has("flare_forecasting") && t > period - 1.5f) ? 0.5f : 1.0f;

        g_mod = FxMod{};
        DrawSwarm();
        DrawStatites();
        DrawDysonRing();
        DrawMercury();
        DrawFlares();
        DrawObservation();
        DrawProbes();
        DrawCorona();
        DrawForge();
        DrawMirrors();
        DrawLogistics();
        DrawStarLift();
        DrawProxima();
        DrawCoverage();
        g_mod = FxMod{};
        if (earth) DrawSystemTraffic(*earth);
    }

private:
    // ------------------------------------------------------------------ ayudas
    bool Has(const char* key) const { return m_tree->IsResearched(key); }
    float R() const { return m_sun->radius; }
    // Un punto del mundo en pantalla, solo si el disco del Sol no lo tapa.
    bool P(const XMVECTOR& w, float& x, float& y) const {
        float z;
        if (!m_cam->Project(w, x, y, z)) return false;
        if (Occluded(*m_sun, x, y, z)) return false;
        // En la vista del Sol el enfoque es el suyo, y lo que lo rodea tiene planos; de lejos (Tierra, Luna) el Sol entero es un
        // punto y no hay planos que separar.
        g_mod = m_big ? ModOf(DepthCue(z)) : FxMod{};
        return true;
    }
    // Un punto de una orbita: `k` radios solares, inclinada `tilt` y girada `node`, en el angulo `a`.
    // En la vista del Sol, que lo muestra casi a pantalla completa, los radios se COMPRIMEN (k de 4 pasa a 2,3): sin eso todo lo
    // que no es la corona cae fuera del cuadro. En las otras dos el Sol es chico y los radios van enteros.
    XMVECTOR OnRing(float k, float tilt, float node, float a) const {
        if (m_big) k = 1.0f + (k - 1.0f) * 0.42f;
        const XMVECTOR v = XMVectorSet(std::cos(a) * k * R(), 0.0f, std::sin(a) * k * R(), 0.0f);
        return XMVectorAdd(m_sun->center, XMVector3TransformNormal(v, XMMatrixRotationX(tilt) * XMMatrixRotationY(node)));
    }
    // El angulo de una orbita de radio `k` en este instante: Kepler, lo de dentro mas rapido.
    float Phase(float k, float phase, float speed = 1.0f) const { return phase + m_time * speed * 1.3f / std::pow(std::max(k, 1.05f), 1.5f); }
    void Pt(const XMVECTOR& w, std::uint32_t rgb, float a, float size = 0.0f) const {
        float x, y;
        if (P(w, x, y)) Dot(*m_ui, x, y, size > 0.0f ? size : m_sz, ColorOf(rgb, a));
    }
    // Una estructura: un pixel de luz con su centro blanco, para que se lea sobre la corona.
    void Spark(const XMVECTOR& w, std::uint32_t rgb, float a) const {
        float x, y;
        if (!P(w, x, y)) return;
        if (m_big) Dot(*m_ui, x, y, 4.0f, ColorOf(rgb, 0.14f * a)); // un halo suave, para que se lea sobre la corona
        Dot(*m_ui, x, y, m_sz, ColorOf(rgb, a));
        if (m_big) Dot(*m_ui, x, y, 1.0f, ColorOf(0xFFFFFF, a));
    }
    static float H(unsigned a, unsigned b) { return Hash01(a, b); }

    // ------------------------------------------------------------------ el enjambre (de lejos hacia adentro)
    void DrawSwarm() {
        struct Group {
            float k, tilt, node;
            int n;
        };
        std::vector<Group> groups;
        if (Has("solar_power_satellites")) groups.push_back({4.0f, 0.35f, 0.3f, 3});   // tres puntos, lejos
        if (Has("mirror_phalanx")) groups.push_back({4.6f, -0.55f, 1.1f, 3});          // otro grupo, otra inclinacion
        if (Has("power_collectors")) {                                                  // grupos chicos en 3 o 4 orbitas inclinadas
            groups.push_back({5.2f, 0.9f, 2.0f, 2});
            groups.push_back({3.8f, -0.2f, 2.6f, 2});
        }
        if (Has("orbit_modelling")) {                                                   // planos repartidos: el enjambre se vuelve 3D
            groups.push_back({3.4f, 0.6f, 0.5f, 3});
            groups.push_back({5.6f, -0.8f, 3.4f, 3});
            groups.push_back({4.2f, 1.2f, 4.4f, 3});
        }
        if (Has("close_orbit_arrays")) groups.push_back({1.45f, 0.9f, 1.0f, 10});       // un grupo interior, mucho mas rapido
        int gi = 0;
        for (const Group& g : groups) {
            for (int i = 0; i < g.n; ++i) {
                const float a = Phase(g.k, H(static_cast<unsigned>(gi), static_cast<unsigned>(i)) * 6.2832f);
                const bool hot = Has("thermal_composites") && (i + gi) % 2 == 0; // los radiadores: un lado rojizo
                Spark(OnRing(g.k, g.tilt, g.node, a), hot ? 0xE0603C : 0xFFC070, 0.95f * m_dim);
            }
            ++gi;
        }
        if (Has("export_balancing")) // el anillo interior: dos docenas de colectores a Mercurio de distancia
            for (int i = 0; i < 24; ++i) Spark(OnRing(1.8f, 0.3f, 0.5f, Phase(1.8f, static_cast<float>(i) * 6.2832f / 24.0f, 1.0f)), 0xFFC878, 0.9f * m_dim);
        if (Has("solar_collector_swarm") && !FxBus().sunWorksInScene) // la nube: sesenta puntos en muchas inclinaciones (con SunWorks: cubos de la escena)
            for (int i = 0; i < 60; ++i) {
                const unsigned u = static_cast<unsigned>(i);
                const float k = 1.6f + H(u, 500u) * 3.0f;
                Spark(OnRing(k, (H(u, 501u) - 0.5f) * 2.2f, H(u, 502u) * 6.2832f, Phase(k, H(u, 503u) * 6.2832f)), Has("thermal_composites") && i % 2 ? 0xE0603C : 0xFFC070, 0.85f * m_dim);
            }
    }

    // ------------------------------------------------------------------ statites: los que no orbitan
    void DrawStatites() {
        if (!Has("solar_mirror_arrays")) return;
        const float bob = std::sin(m_time * 0.8f) * 0.04f;
        for (float sign : {1.0f, -1.0f}) {
            const int count = (Has("mirror_swarms") ? 5 : (sign > 0.0f ? 1 : 0));
            for (int i = 0; i < count; ++i) {
                const float a = static_cast<float>(i) * 6.2832f / 5.0f;
                const float r = i == 0 && count == 1 ? 0.0f : 0.35f;
                Spark(XMVectorAdd(m_sun->center, XMVectorSet(std::cos(a) * r * R(), sign * (1.7f + bob) * R(), std::sin(a) * r * R(), 0.0f)), 0xFFE6A0, 0.95f);
            }
        }
    }

    // ------------------------------------------------------------------ el anillo de Dyson
    void DrawDysonRing() {
        if (FxBus().sunWorksInScene) return; // el anillo es una malla de la escena (SunWorks.cpp)
        const bool ghost = Has("megastructure_design"), solid = Has("collector_ring");
        if (!ghost) return;
        const int N = solid ? 96 : 48;
        for (int i = 0; i < N; ++i) {
            if (!solid && i % 2) continue; // el fantasma: un contorno punteado
            Pt(OnRing(2.4f, 0.0f, 0.0f, static_cast<float>(i) * 6.2832f / static_cast<float>(N)), solid ? 0xFFD890 : 0xFFD890, solid ? 0.85f : 0.22f);
        }
        if (!solid) return;
        for (int k = 0; k < 3; ++k) Spark(OnRing(2.4f, 0.0f, 0.0f, m_time * 0.4f + static_cast<float>(k) * 2.1f), 0xFFFFFF, 1.0f); // pulsos de carga
        if (Has("solar_assembly")) // anillos a otra altura e inclinacion: el Sol parece un giroscopio
            for (int r = 0; r < 2; ++r)
                for (int i = 0; i < 70; ++i)
                    Pt(OnRing(r ? 2.9f : 2.0f, r ? -0.55f : 0.5f, r ? 1.6f : 0.4f, static_cast<float>(i) * 6.2832f / 70.0f), 0xFFC070, 0.7f);
    }

    // ------------------------------------------------------------------ Mercurio y Venus
    void DrawMercury() {
        const XMVECTOR mp = OnRing(2.6f, 0.08f, 0.9f, Phase(2.6f, 0.7f));
        if (Has("mercury_survey")) {
            float x, y;
            if (P(mp, x, y)) {
                const float shrink = Has("solar_construction") ? 0.5f + 0.5f * std::fabs(std::sin(m_time * 0.1f)) : 1.0f; // lo van desarmando
                Dot(*m_ui, x, y, (m_big ? 3.0f : 2.0f) * shrink, ColorOf(0x9A8A7A, 0.95f));
                Spark(OnRing(2.6f, 0.08f, 0.9f, Phase(2.6f, 0.7f) + 0.25f), 0x9CE8FF, 0.9f); // la sonda
                if (Has("mercury_mining") || Has("solar_construction")) { // chispas de metal hacia el Sol
                    for (int k = 0; k < 4; ++k) {
                        const float t = std::fmod(m_time * 0.4f + static_cast<float>(k) / 4.0f, 1.0f);
                        Dot(*m_ui, x + (m_sun->sx - x) * t * 0.7f, y + (m_sun->sy - y) * t * 0.7f, 1.0f, ColorOf(0xFFC070, 0.8f * (1.0f - t)));
                    }
                }
            }
        }
        if (Has("venus_cloud_cities")) { // un punto palido con luces de ciudad flotante
            const XMVECTOR vp = OnRing(3.6f, -0.1f, 3.0f, Phase(3.6f, 2.0f));
            Pt(vp, 0xF0E0A0, 0.95f, m_big ? 3.0f : 2.0f);
            float x, y;
            if (P(vp, x, y) && m_big) Dot(*m_ui, x + 1.0f, y, 1.0f, ColorOf(0xFFFFFF, 0.9f));
        }
    }

    // ------------------------------------------------------------------ llamaradas
    void DrawFlares() {
        if (!Has("flare_harvesting")) return;
        const float a = H(m_flareN, 7u) * 6.2832f;
        if (m_big)
            for (int i = 0; i < 18; ++i) // la red: un arco punteado cerca del Sol
                Pt(OnRing(1.3f, 0.0f, 0.0f, a + (static_cast<float>(i) - 9.0f) * 0.06f), 0xFFB070, 0.35f);
        if (m_flareT < 1.5f && Has("flare_models") == false) {
            const float t = m_flareT / 1.5f;
            for (int i = 0; i < 10; ++i) { // la llamarada: un arco que sale del limbo y vuelve a caer
                const float f = static_cast<float>(i) / 9.0f;
                const float h = 1.0f + 0.7f * std::sin(kPi * f) * std::sin(kPi * std::min(1.0f, t * 1.2f));
                Pt(OnRing(h, 0.0f, 0.0f, a + f * 0.35f), 0xFFE0A0, 0.9f * (1.0f - t * 0.6f), m_sz + 1.0f);
            }
        } else if (m_flareT < 1.5f) {
            const float t = m_flareT / 1.5f; // con modelos de llamaradas, la red la atrapa a medio camino
            for (int i = 0; i < 10; ++i) {
                const float f = static_cast<float>(i) / 9.0f;
                Pt(OnRing(1.0f + 0.3f * std::sin(kPi * f) * std::sin(kPi * t), 0.0f, 0.0f, a + f * 0.35f), 0xFFE0A0, 0.8f * (1.0f - t * 0.5f), m_sz + 1.0f);
            }
        }
        if (Has("flare_models") && m_flareT > 9.0f - 1.5f) // el aviso: un pulso antes de la llamarada
            Pt(OnRing(1.12f, 0.0f, 0.0f, H(m_flareN + 1u, 7u) * 6.2832f), 0xFFFFFF, 0.5f + 0.4f * std::sin(m_time * 10.0f), m_sz + 2.0f);
    }

    // ------------------------------------------------------------------ observacion
    void DrawObservation() {
        if (Has("solar_monitoring")) { // el Sol respira: un anillo fino que se infla y se desinfla
            const float r = 1.04f + 0.02f * std::sin(m_time * 1.2f);
            for (int i = 0; i < 40; ++i) Pt(OnRing(r, 0.0f, 0.0f, static_cast<float>(i) * 6.2832f / 40.0f), 0xFFF0C0, 0.22f, 1.0f);
        }
        if (Has("solar_telescopes")) {
            const XMVECTOR w = OnRing(2.1f, 0.2f, 2.4f, Phase(2.1f, 2.4f, 0.5f));
            float x, y;
            if (P(w, x, y)) {
                DrawSprite(*m_ui, kDome, x, y, 0xF0F4FF, 1.0f, 0.95f);
                Dot(*m_ui, x, y, 5.0f, ColorOf(0xFFA040, 0.18f)); // el filtro naranja
            }
        }
        if (Has("flux_instruments")) // sensores que parpadean con las llamaradas
            for (int i = 0; i < 6; ++i)
                Pt(OnRing(1.25f, 0.0f, 0.0f, static_cast<float>(i) * 1.0472f + m_time * 0.2f), 0xBFE8FF, (m_flareT < 1.5f ? 0.9f : 0.4f), 1.0f);
        if (Has("magnetic_field_models")) // lineas de campo: tres arcos de polo a polo
            for (int m = 0; m < 3; ++m)
                for (int i = 0; i <= 18; ++i) {
                    const float f = static_cast<float>(i) / 18.0f;
                    const float lat = -kPi * 0.5f + f * kPi, lon = static_cast<float>(m) * 2.0944f + m_time * 0.1f;
                    const float r = 1.3f + 0.25f * std::cos(lat);
                    Pt(XMVectorAdd(m_sun->center, XMVectorSet(std::cos(lat) * std::cos(lon) * r * R(), std::sin(lat) * r * R(), std::cos(lat) * std::sin(lon) * r * R(), 0.0f)), 0x9CC8FF, 0.22f, 1.0f);
                }
        if (Has("stellar_instruments")) { // diez sensores parejos; con las IA se mueven todos a la vez
            const float spin = Has("stellar_ai") || Has("solar_systems_ai") ? m_time * 0.3f : 0.0f;
            for (int i = 0; i < 10; ++i) Pt(OnRing(1.32f, 0.5f, 0.0f, static_cast<float>(i) * 0.6283f + spin), 0xFFFFFF, 0.55f, 1.0f);
        }
    }

    // ------------------------------------------------------------------ sondas
    void DrawProbes() {
        if (!Has("solar_probes")) return;
        const int n = Has("probe_swarms") ? 40 : 3;
        const float rmin = Has("sun_proof_alloys") ? 1.03f : Has("heat_shield_materials") ? 1.15f : 1.35f;
        const bool brake = Has("close_orbit_insertion"), lanes = Has("orbit_corridors"), glow = Has("corona_materials");
        for (int i = 0; i < n; ++i) {
            const unsigned u = static_cast<unsigned>(i);
            const float cycle = 14.0f + static_cast<float>(i % 5);
            const float t = std::fmod(m_time / cycle + H(u, 600u), 1.0f);
            float r = 3.2f - (3.2f - rmin) * t; // una espiral hacia adentro
            if (lanes) r = 1.2f + std::floor((r - 1.2f) / 0.35f) * 0.35f; // por carriles
            const float a = H(u, 601u) * 6.2832f + m_time * 1.6f / std::pow(std::max(r, 1.05f), 1.5f);
            const float fade = brake ? 1.0f : 1.0f - std::max(0.0f, (t - 0.85f) / 0.15f);
            const XMVECTOR w = OnRing(r, (H(u, 602u) - 0.5f) * 1.4f, H(u, 603u) * 6.2832f, a);
            Spark(w, 0x8CE6FF, 0.9f * fade);
            if (glow && r < 1.5f) Pt(w, 0xFFB050, 0.3f * fade, m_sz + 2.0f); // dentro de la corona, brillan
        }
    }

    // ------------------------------------------------------------------ corona
    void DrawCorona() {
        if (!Has("coronal_power")) return;
        const float a = m_time * 0.7f;
        const XMVECTOR bead = OnRing(1.12f, 0.15f, 0.7f, a);
        const bool pulse = Has("solar_fusion_study");
        Pt(bead, 0xFFB050, (pulse ? 0.15f + 0.1f * std::sin(m_time * 2.0f) : 0.25f), m_sz + 3.0f);
        Spark(bead, 0xFFE0A0, 1.0f);
        if (Has("reactor_study")) // la copia chica del reactor del Sol: un anillo brillante alrededor
            for (int i = 0; i < 6; ++i) Pt(XMVectorAdd(bead, XMVectorSet(std::cos(m_time * 3.0f + static_cast<float>(i)) * 0.12f * R(), std::sin(m_time * 3.0f + static_cast<float>(i)) * 0.12f * R(), 0.0f, 0.0f)), 0xFFFFFF, 0.8f, 1.0f);
        if (Has("solar_wind_harvest")) // el bucle que atrapa el viento: una elipse de luz
            for (int i = 0; i < 14; ++i) {
                const float b = static_cast<float>(i) * 6.2832f / 14.0f + m_time * 0.4f;
                Pt(XMVectorAdd(OnRing(1.6f, 0.1f, 2.2f, 0.8f), XMVectorSet(std::cos(b) * 0.3f * R(), std::sin(b) * 0.14f * R(), 0.0f, 0.0f)), 0xFFC878, 0.7f, 1.0f);
            }
        if (Has("solar_power_hub")) { // paneles, espejos y generadores juntos
            float x, y;
            if (P(OnRing(1.35f, 0.0f, 4.0f, 1.0f), x, y)) {
                DrawSprite(*m_ui, kMirror, x - 3.0f, y, 0xFFE6A0, 1.0f, 0.95f);
                DrawSprite(*m_ui, kCollector, x + 3.0f, y - 1.0f, 0xFFC070, 1.0f, 0.95f);
                DrawSprite(*m_ui, kDome, x, y + 3.0f, 0xF0F4FF, 1.0f, 0.95f);
            }
        }
    }

    // ------------------------------------------------------------------ forjas
    void DrawForge() {
        if (!Has("solar_forges")) return;
        float x, y;
        if (!P(OnRing(2.6f, 0.4f, 1.0f, Phase(2.6f, 1.0f, 0.4f)), x, y)) return;
        DrawSprite(*m_ui, kMirror, x, y, 0xFFE6A0, 1.0f, 0.95f);
        const float tx = x + 7.0f, ty = y + 3.0f;
        m_ui->AddLine(x + 2.0f, y + 1.0f, tx, ty, ColorOf(0xFFE0A0, 0.6f), 1.0f); // el haz concentrado
        Dot(*m_ui, tx, ty, 4.0f, ColorOf(0xFF7020, 0.55f)); // donde se funde el metal
        if (Has("solar_shipyards")) // una nave toma forma al lado
            for (int k = 0; k < 3; ++k) DrawSprite(*m_ui, kTug, tx + 5.0f + static_cast<float>(k) * 2.0f, ty + 2.0f - static_cast<float>(k), 0xB8C0D0, 1.0f, 0.9f);
        if (Has("solar_foundries")) // fundidores que brillan en la luz
            for (int k = 0; k < 2; ++k) Dot(*m_ui, tx - 4.0f + static_cast<float>(k) * 8.0f, ty + 5.0f, 2.0f, ColorOf(0xFF9A40, 0.6f + 0.3f * std::sin(m_time * 5.0f + static_cast<float>(k))));
    }

    // ------------------------------------------------------------------ espejos
    void DrawMirrors() {
        if (!Has("sun_mirrors")) return;
        float x, y;
        if (!P(OnRing(2.0f, 0.1f, 3.5f, Phase(2.0f, 3.5f, 0.3f)), x, y)) return;
        DrawSprite(*m_ui, kMirror, x, y, 0xFFF0C8, 1.0f, 1.0f);
        const float tx = x - 24.0f, ty = y - 10.0f; // la luz, empujada lejos del Sol
        m_ui->AddLine(x, y, tx, ty, ColorOf(0xFFF0C8, 0.45f), 1.0f);
        if (Has("focusing_arrays")) { // el punto de luz quemando un blanco
            Dot(*m_ui, tx, ty, 4.0f, ColorOf(0xFFFFFF, 0.55f + 0.2f * std::sin(m_time * 4.0f)));
            Glint(*m_ui, tx, ty, 0.9f);
        }
    }

    // ------------------------------------------------------------------ logistica
    void DrawLogistics() {
        if (!Has("solar_charter")) return;
        if (std::fmod(m_time, 1.6f) < 0.8f) Spark(OnRing(2.0f, 0.0f, 0.0f, 0.0f), 0xFF5050, 0.95f); // la baliza del plano ecuatorial
        if (Has("solar_trade_routes")) // carriles tenues del Sol a cada mundo
            for (int k = 0; k < 6; ++k)
                for (int i = 0; i < 8; ++i) Pt(OnRing(1.3f + static_cast<float>(i) * 0.33f, 0.0f, 0.0f, static_cast<float>(k) * 1.0472f), 0xBFE8FF, 0.12f, 1.0f);
        if (Has("sun_logistics")) // carga que llega y se va por los carriles
            for (int k = 0; k < 6; ++k) {
                const float t = std::fmod(m_time * 0.2f + static_cast<float>(k) * 0.17f, 1.0f);
                Pt(OnRing(1.3f + t * 2.3f, 0.0f, 0.0f, static_cast<float>(k) * 1.0472f), 0xFFFFFF, 0.7f, m_sz);
            }
        if (Has("collector_cargo")) // los colectores se cambian de lugar como muebles
            for (int k = 0; k < 4; ++k) {
                const float t = std::fmod(m_time * 0.1f + static_cast<float>(k) * 0.25f, 1.0f);
                Spark(OnRing(2.4f + std::sin(t * 6.2832f) * 0.15f, 0.0f, 0.0f, 3.0f + t * 1.5f), 0xFFC878, 0.9f);
            }
    }

    // ------------------------------------------------------------------ levantamiento estelar
    void DrawStarLift() {
        if (!Has("sun_landing_rig")) return;
        for (int i = 0; i < 12; ++i) { // un penacho que sube del polo norte
            const float t = std::fmod(m_time * 0.25f + static_cast<float>(i) / 12.0f, 1.0f);
            Pt(XMVectorAdd(m_sun->center, XMVectorSet(std::sin(t * 7.0f + static_cast<float>(i)) * 0.06f * R(), (1.0f + t * 1.1f) * R(), 0.0f, 0.0f)), 0xFFD890, 0.9f * (1.0f - t), m_sz + 1.0f);
        }
    }

    // ------------------------------------------------------------------ Proxima
    void DrawProxima() {
        if (!Has("solar_gate_prep")) return;
        const float ang = -0.35f;
        const float dx = std::cos(ang), dy = std::sin(ang);
        const float x0 = m_sun->sx + dx * (m_sun->sr + 3.0f), y0 = m_sun->sy + dy * (m_sun->sr + 3.0f);
        for (int i = 0; i < 40; ++i) { // un haz punteado hacia el borde de la pantalla, con un brillo que corre
            const float d = static_cast<float>(i) * 6.0f;
            const float hot = std::fmod(m_time * 0.7f - static_cast<float>(i) * 0.04f, 1.0f) < 0.1f ? 1.0f : 0.0f;
            Dot(*m_ui, x0 + dx * d, y0 + dy * d, 1.0f, ColorOf(0xFFFFFF, 0.25f + 0.6f * hot));
        }
    }

    // ------------------------------------------------------------------ cobertura: el Sol se punteado y se apaga
    void DrawCoverage() {
        if (Has("solar_networks")) {
            // El disco se llena de puntos oscuros (los colectores de frente): una trama de celdas de 3 px.
            const float sr = m_sun->sr;
            const int cells = static_cast<int>(sr / 1.5f);
            const float density = Has("solar_foothold") ? 0.55f : 0.35f;
            for (int cy = -cells; cy <= cells; ++cy)
                for (int cx = -cells; cx <= cells; ++cx) {
                    const float ox = static_cast<float>(cx) * 3.0f, oy = static_cast<float>(cy) * 3.0f;
                    if (ox * ox + oy * oy > sr * sr * 0.98f) continue;
                    if (H(static_cast<unsigned>(cx + 400), static_cast<unsigned>(cy + 400)) > density) continue;
                    m_ui->AddRect(kit::Rect(std::floor(m_sun->sx + ox), std::floor(m_sun->sy + oy), 2.0f, 2.0f), ColorOf(0x180800, 0.34f));
                }
        }
        if (Has("solar_foothold")) // la estrella envuelta: un resplandor naranja tenue entre los huecos y un halo infrarrojo
            for (int r = 0; r < 3; ++r)
                for (int i = 0; i < 80; ++i) Pt(OnRing(1.55f + static_cast<float>(r) * 0.3f, 0.0f, 0.0f, static_cast<float>(i) * 6.2832f / 80.0f + static_cast<float>(r)), 0xFF6A30, 0.14f - 0.03f * static_cast<float>(r), 1.0f);
    }

    // ------------------------------------------------------------------ trafico del sistema (visto desde la Tierra)
    void DrawSystemTraffic(const Body& earth) {
        const UiRect vis = UiService::VisibleRect();
        if (Has("interplanetary_transit")) {
            const int n = 3 + (Has("fusion_torches") ? 3 : 0) + (Has("fusion_drives") ? 3 : 0);
            const int tail = Has("fusion_drives") ? 8 : 3;
            for (int j = 0; j < n; ++j) {
                const unsigned u = static_cast<unsigned>(j);
                const float dir = H(u, 800u) * 6.2832f, cycle = 8.0f + 6.0f * H(u, 801u);
                const float t = std::fmod(m_time / cycle + H(u, 802u), 1.0f);
                const float reach = std::max(vis.width, vis.height) * 0.6f;
                const float r = earth.sr * 1.3f + t * t * reach;
                const float x = earth.sx + std::cos(dir) * r, y = earth.sy + std::sin(dir) * r * 0.62f;
                if (!vis.Contains(x, y)) continue;
                const float a = t < 0.15f ? t / 0.15f : 1.0f - (t - 0.15f) / 0.85f;
                const std::uint32_t flame = Has("fusion_torches") ? 0xA0D0FF : 0xFFB070;
                Dot(*m_ui, x, y, 2.0f, ColorOf(0xFFFFFF, a));
                for (int k = 1; k <= tail; ++k)
                    Dot(*m_ui, x - std::cos(dir) * 3.0f * static_cast<float>(k), y - std::sin(dir) * 1.9f * static_cast<float>(k), 1.0f, ColorOf(k < 3 ? flame : 0xE0E4EE, a * (0.6f - 0.06f * static_cast<float>(k))));
            }
        }
        if (Has("solar_sails")) {
            const int n = Has("light_sail_fleets") ? 8 : 1;
            for (int j = 0; j < n; ++j) {
                const unsigned u = static_cast<unsigned>(j);
                const float dir = 0.5f + (H(u, 810u) - 0.5f) * 1.6f, cycle = 20.0f + 8.0f * H(u, 811u);
                const float t = std::fmod(m_time / cycle + H(u, 812u), 1.0f);
                const float r = earth.sr * 1.4f + t * std::max(vis.width, vis.height) * 0.5f;
                const float x = earth.sx + std::cos(dir) * r, y = earth.sy - std::sin(dir) * r * 0.6f;
                if (!vis.Contains(x, y)) continue;
                const float a = t < 0.1f ? t / 0.1f : 1.0f - (t - 0.1f) / 0.9f;
                DrawSprite(*m_ui, kSail, x, y, 0xFFF0C8, 1.0f, a * 0.85f);
                if (Has("laser_launch_arrays") && t < 0.5f) // el haz del suelo que empuja la flota
                    m_ui->AddLine(earth.sx, earth.sy, x, y, ColorOf(0xFF6070, 0.12f), 1.0f);
            }
        }
    }

    UiDrawList* m_ui = nullptr;
    const Cam* m_cam = nullptr;
    const Body* m_sun = nullptr;
    const TechTree* m_tree = nullptr;
    float m_time = 0.0f, m_sz = 1.0f, m_dim = 1.0f, m_flareT = 0.0f;
    unsigned m_flareN = 0;
    bool m_big = false;
};
