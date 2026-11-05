// WorldFx: lo que se COMPRO, a la vista alrededor del planeta. Satelites que giran,
// luces sobre la Tierra, anillos alrededor del Sol, puntos que se alejan.
//
// Es presentacion pura: lee cuantas hay de cada instalacion y las dibuja, y nada de
// aqui cambia la partida. Que cada tipo se vea como un tipo de cosa (Orbit, Surface,
// Sun, Far) lo dice la hoja (columna Look); aqui solo se sabe dibujar esas cuatro.
//
// COMO SE DIBUJA. No son objetos de la escena: son puntos de la UI puestos donde la
// camara del juego veria un punto del mundo (Tierra y Sol vienen de la escena, el resto
// se calcula). Una escena con cientos de instalaciones seria cientos de objetos que
// animar; asi son un bucle de rectangulos, y la cuenta de cuantas se dibujan la acota la
// hoja (`Visible cap`).
//   - Orbit: cada punto va en una orbita inclinada alrededor de la Tierra. Las orbitas
//     que pasan DETRAS del planeta se tapan con su disco, para que de verdad giren
//     alrededor y no encima. Con 25 o mas se dibujan tambien las orbitas.
//   - Surface: luces fijas a la superficie (giran con la Tierra, que es la matriz de su
//     nodo). Se ven mas fuertes del lado de noche, que es donde una luz se ve.
//   - Sun: lo mismo que Orbit, alrededor del Sol.
//   - Far: puntos que salen del planeta hacia el borde de la pantalla y se apagan.
//
// Corre justo despues de GameRoot y antes del HUD y los paneles: la interfaz va encima.

#include "../Services/Session.h"
#include "UiKit.h"

#include "Engine/Rendering/Camera.h"
#include "Engine/Scene/GameObject.h"
#include "Engine/Scene/SceneQuery.h"
#include "Engine/Scene/SceneService.h"
#include "Engine/Scene/SceneTransform.h"
#include "Engine/Scripting/IScript.h"
#include "Engine/Scripting/ScriptRegistry.h"
#include "Engine/UI/UiService.h"

#include <DirectXMath.h>

#include <algorithm>
#include <cmath>
#include <string>

using namespace strace;
using namespace evergrave;
using namespace DirectX;

namespace {

constexpr const char* kEarthPath = "Space/OrbitPivot/EarthAnchor/Tilt/Earth";
constexpr const char* kMoonPath = "Space/OrbitPivot/EarthAnchor/Moon";
constexpr const char* kSunPath = "Space/OrbitPivot/Sun";
constexpr float kPi = 3.14159265f;

unsigned Hash(unsigned a, unsigned b = 0) {
    unsigned h = a * 374761393u + b * 668265263u + 0x9E3779B9u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}
float Hash01(unsigned a, unsigned b = 0) { return static_cast<float>(Hash(a, b) & 0xFFFFFF) / static_cast<float>(0x1000000); }

unsigned HashString(const std::string& s) {
    unsigned h = 2166136261u;
    for (char c : s) h = (h ^ static_cast<unsigned char>(c)) * 16777619u;
    return h;
}

UiColor ColorOf(std::uint32_t rgb, float a = 1.0f) {
    return UiColor::Bytes(static_cast<int>((rgb >> 16) & 0xFF), static_cast<int>((rgb >> 8) & 0xFF), static_cast<int>(rgb & 0xFF),
                          static_cast<int>(255.0f * std::clamp(a, 0.0f, 1.0f)));
}

// La camara del juego: del mundo a la pantalla, con la misma cuenta que el rayo del puntero.
struct Cam {
    XMMATRIX view, proj;
    float w = 0, h = 0;

    // Un punto del mundo a pixeles de juego. `vz` es su profundidad en la vista (mayor = mas lejos).
    bool Project(FXMVECTOR world, float& sx, float& sy, float& vz) const {
        const XMVECTOR v = XMVector3TransformCoord(world, view);
        vz = XMVectorGetZ(v);
        if (vz <= 0.05f) return false;
        const XMVECTOR c = XMVector3TransformCoord(v, proj);
        sx = (XMVectorGetX(c) * 0.5f + 0.5f) * w;
        sy = (0.5f - XMVectorGetY(c) * 0.5f) * h;
        return true;
    }
};

// Una esfera vista desde la camara: donde cae su centro, que tan grande se ve y a que profundidad.
struct Body {
    XMVECTOR center = XMVectorZero();
    float radius = 1.0f;
    float sx = 0, sy = 0, sr = 0, vz = 0;
    bool ok = false;
};

Body MakeBody(const Cam& cam, const GameObject& o) {
    Body b;
    const XMMATRIX m = ComputeWorldMatrix(o);
    b.center = XMVector3TransformCoord(XMVectorZero(), m);
    b.radius = XMVectorGetX(XMVector3Length(XMVectorSubtract(XMVector3TransformCoord(XMVectorSet(1, 0, 0, 0), m), b.center)));
    b.ok = cam.Project(b.center, b.sx, b.sy, b.vz);
    if (!b.ok) return b;
    // El borde: el centro mas el radio sobre el eje x de la VISTA (no del mundo): asi el
    // radio en pantalla es el del disco que se ve, sin importar hacia donde mire la camara.
    const XMVECTOR vc = XMVector3TransformCoord(b.center, cam.view);
    const XMVECTOR ce = XMVector3TransformCoord(XMVectorAdd(vc, XMVectorSet(b.radius, 0, 0, 0)), cam.proj);
    const float ex = (XMVectorGetX(ce) * 0.5f + 0.5f) * cam.w;
    b.sr = std::fabs(ex - b.sx);
    return b;
}

// Un punto que esta TAPADO por el cuerpo: detras de su centro y dentro de su disco.
bool Occluded(const Body& b, float sx, float sy, float vz) {
    if (vz <= b.vz) return false;
    const float dx = sx - b.sx, dy = sy - b.sy;
    return dx * dx + dy * dy < b.sr * b.sr;
}

void Dot(UiDrawList& ui, float x, float y, float size, const UiColor& c) {
    ui.AddRect(kit::Rect(std::floor(x - size * 0.5f), std::floor(y - size * 0.5f), size, size), c);
}

// La orbita `ring` de `n`: inclinada y girada distinto de las otras, cada una a su ritmo.
struct Orbit {
    float radius, speed, tilt, node, phase;
};
Orbit OrbitOf(unsigned seed, int ring, float baseRadius) {
    Orbit o;
    o.radius = baseRadius * (1.0f + 0.07f * static_cast<float>(ring));
    o.speed = (0.55f / (1.0f + 0.25f * static_cast<float>(ring))) * (ring % 2 == 0 ? 1.0f : -1.0f);
    o.tilt = (0.35f + 0.22f * static_cast<float>(ring % 3)) * (ring % 2 == 0 ? 1.0f : -1.0f);
    o.node = 1.05f * static_cast<float>(ring) + Hash01(seed, static_cast<unsigned>(ring)) * 0.6f;
    o.phase = Hash01(seed, 100u + static_cast<unsigned>(ring)) * 2.0f * kPi;
    return o;
}

// Un punto de la orbita, en el mundo, alrededor de `center`.
XMVECTOR OrbitPoint(const XMVECTOR& center, const Orbit& o, float angle) {
    XMVECTOR p = XMVectorSet(o.radius * std::cos(angle), 0.0f, o.radius * std::sin(angle), 0.0f);
    p = XMVector3TransformNormal(p, XMMatrixRotationX(o.tilt) * XMMatrixRotationY(o.node));
    return XMVectorAdd(center, p);
}

} // namespace

class WorldFx : public IScript {
public:
    void OnStart() override {
        m_earth = FindByPath(SceneService::MutableRoots(), kEarthPath);
        m_moon = FindByPath(SceneService::MutableRoots(), kMoonPath);
        m_sun = FindByPath(SceneService::MutableRoots(), kSunPath);
    }

    void OnUpdate(float dt) override {
        m_time += dt;
        const GameState& game = Session();
        if (Ui().menu) return; // con el menu abierto no se dibujan las instalaciones
        if (!m_earth || !m_moon || !m_sun || game.TotalOwned() <= 0) return;
        // En el mapa no se dibujan: lo construido se ve de cerca, en el cuerpo al que pertenece.
        if (Ui().mapView) return;
        const GameObject* camObj = FindActiveCamera(SceneService::Roots());
        if (!camObj || !camObj->camera) return;
        Cam cam;
        cam.w = UiService::Width();
        cam.h = UiService::Height();
        if (cam.w <= 0.0f || cam.h <= 0.0f) return;
        cam.view = ComputeGameCameraView(*camObj);
        cam.proj = ComputeGameCameraProjection(*camObj->camera, SceneService::Type(), cam.w / cam.h);

        // El cuerpo que se mira: lo construido alli se dibuja a su alrededor y encima de el.
        GameObject* focusObj = Ui().view == BodyId::Moon ? m_moon : Ui().view == BodyId::Sun ? m_sun : m_earth;
        // Del Sol solo hace falta DONDE esta (para saber que lado de un cuerpo es de noche): con la camara
        // del lado opuesto queda detras de ella y no se puede proyectar, y eso no es motivo para no dibujar.
        const Body focus = MakeBody(cam, *focusObj);
        if (!focus.ok) return;
        const XMVECTOR sunCenter = XMVector3TransformCoord(XMVectorZero(), ComputeWorldMatrix(*m_sun));
        UiDrawList& ui = UiService::Frame();

        for (const InstallationDef& d : game.installations().All()) {
            if (d.body != Ui().view) continue;
            const int owned = game.Owned(d.id);
            if (owned <= 0) continue;
            const int shown = std::min(owned, d.visibleCap);
            const unsigned seed = HashString(d.id);
            switch (d.look) {
                case InstallationLook::Orbit:
                case InstallationLook::Sun: DrawOrbit(ui, cam, focus, d, owned, shown, seed, focus); break;
                case InstallationLook::Surface: DrawSurface(ui, cam, focus, sunCenter, d, owned, shown, seed, *focusObj); break;
                case InstallationLook::Far: DrawFar(ui, cam, focus, d, owned, shown, seed); break;
            }
        }
    }

private:
    // Satelites (alrededor de la Tierra) o espejos y forjas (alrededor del Sol).
    void DrawOrbit(UiDrawList& ui, const Cam& cam, const Body& around, const InstallationDef& d, int owned, int shown, unsigned seed, const Body& occluder) const {
        const int rings = std::clamp(1 + shown / 8, 1, 6);
        const float base = around.radius * 1.3f;
        const UiColor c = ColorOf(d.color);

        // Con 25 o mas, las orbitas se dibujan: se ve la estructura, no solo los puntos.
        if (owned >= 25) {
            for (int k = 0; k < rings; ++k) {
                const Orbit o = OrbitOf(seed, k, base);
                float px = 0, py = 0, pz = 0;
                bool havePrev = false;
                for (int i = 0; i <= 56; ++i) {
                    const float a = 2.0f * kPi * static_cast<float>(i) / 56.0f;
                    float sx, sy, vz;
                    const bool ok = cam.Project(OrbitPoint(around.center, o, a), sx, sy, vz);
                    if (ok && havePrev && !Occluded(occluder, sx, sy, vz) && !Occluded(occluder, px, py, pz))
                        ui.AddLine(px, py, sx, sy, ColorOf(d.color, 0.16f), 1.0f);
                    havePrev = ok;
                    px = sx; py = sy; pz = vz;
                }
            }
        }

        for (int j = 0; j < shown; ++j) {
            const int ring = j % rings;
            const int inRing = (shown - ring + rings - 1) / rings;
            const int idx = j / rings;
            const Orbit o = OrbitOf(seed, ring, base);
            const float angle = o.phase + o.speed * m_time + 2.0f * kPi * static_cast<float>(idx) / static_cast<float>(std::max(1, inRing));

            // La estela: tres puntos detras, cada vez mas tenues.
            for (int t = 3; t >= 1; --t) {
                float sx, sy, vz;
                if (!cam.Project(OrbitPoint(around.center, o, angle - o.speed * 0.07f * static_cast<float>(t)), sx, sy, vz)) continue;
                if (Occluded(occluder, sx, sy, vz)) continue;
                Dot(ui, sx, sy, 1.0f, ColorOf(d.color, 0.45f - 0.12f * static_cast<float>(t)));
            }
            float sx, sy, vz;
            if (!cam.Project(OrbitPoint(around.center, o, angle), sx, sy, vz)) continue;
            if (Occluded(occluder, sx, sy, vz)) continue;
            if (owned >= 10) Dot(ui, sx, sy, 5.0f, ColorOf(d.color, 0.18f)); // el brillo del hito
            Dot(ui, sx, sy, 3.0f, c);
            Dot(ui, sx, sy, 1.0f, UiColor::Bytes(255, 255, 255));
        }
    }

    // Luces sobre la Tierra. Fijas a la superficie: la matriz del nodo ya lleva el giro.
    void DrawSurface(UiDrawList& ui, const Cam& cam, const Body& earth, const XMVECTOR& sunCenter, const InstallationDef& d, int owned, int shown, unsigned seed,
                     const GameObject& bodyObj) const {
        const XMMATRIX m = ComputeWorldMatrix(bodyObj);
        const XMVECTOR vc = XMVector3TransformCoord(earth.center, cam.view);
        const XMVECTOR toSun = XMVector3Normalize(XMVectorSubtract(sunCenter, earth.center));
        for (int j = 0; j < shown; ++j) {
            // Latitud y longitud fijas por (tipo, numero): la que se compra primero queda donde esta.
            const float lat = (Hash01(seed, static_cast<unsigned>(j)) - 0.42f) * 2.1f;
            const float lon = Hash01(seed, 500u + static_cast<unsigned>(j)) * 2.0f * kPi;
            const XMVECTOR local = XMVectorSet(std::cos(lat) * std::cos(lon), std::sin(lat), std::cos(lat) * std::sin(lon), 0.0f);
            const XMVECTOR world = XMVector3TransformCoord(local, m);

            const XMVECTOR pv = XMVector3TransformCoord(world, cam.view);
            const XMVECTOR n = XMVectorSubtract(pv, vc);
            if (XMVectorGetX(XMVector3Dot(n, XMVectorNegate(pv))) <= 0.0f) continue; // del otro lado del planeta
            float sx, sy, vz;
            if (!cam.Project(world, sx, sy, vz)) continue;

            // Del lado de noche (lejos del Sol) la luz se ve; del de dia apenas.
            const XMVECTOR nw = XMVector3Normalize(XMVectorSubtract(world, earth.center));
            const float lit = XMVectorGetX(XMVector3Dot(nw, toSun)); // 1 = mediodia, -1 = medianoche
            const float night = std::clamp(0.5f - lit * 0.5f, 0.0f, 1.0f);
            const float twinkle = 0.75f + 0.25f * std::sin(m_time * 2.2f + Hash01(seed, 900u + static_cast<unsigned>(j)) * 6.28f);
            const float a = (0.28f + 0.72f * night) * twinkle;
            if (owned >= 10) Dot(ui, sx, sy, 4.0f, ColorOf(d.color, 0.14f * a)); // brillo del hito
            Dot(ui, sx, sy, 2.0f, ColorOf(d.color, a));
        }
    }

    // Puntos que salen de la Tierra hacia el borde de la pantalla y se apagan.
    void DrawFar(UiDrawList& ui, const Cam& cam, const Body& earth, const InstallationDef& d, int owned, int shown, unsigned seed) const {
        (void)owned;
        const UiRect vis = UiService::VisibleRect();
        for (int j = 0; j < shown; ++j) {
            const float dir = Hash01(seed, static_cast<unsigned>(j)) * 2.0f * kPi;
            const float period = 7.0f + 6.0f * Hash01(seed, 300u + static_cast<unsigned>(j));
            const float u = std::fmod(m_time / period + Hash01(seed, 700u + static_cast<unsigned>(j)), 1.0f);
            // Hasta el borde mas lejano en esa direccion.
            const float reach = std::max(vis.width, vis.height) * 0.6f;
            const float r = earth.sr * 1.2f + u * u * reach;
            const float x = earth.sx + std::cos(dir) * r, y = earth.sy + std::sin(dir) * r * 0.62f;
            if (!vis.Contains(x, y)) continue;
            const float a = u < 0.15f ? u / 0.15f : 1.0f - (u - 0.15f) / 0.85f;
            Dot(ui, x, y, 2.0f, ColorOf(d.color, a));
            Dot(ui, x - std::cos(dir) * 3.0f, y - std::sin(dir) * 1.9f, 1.0f, ColorOf(d.color, a * 0.4f));
        }
        (void)cam;
    }

    GameObject* m_earth = nullptr;
    GameObject* m_moon = nullptr;
    GameObject* m_sun = nullptr;
    float m_time = 0.0f;
};

STRACE_REGISTER_SCRIPT(WorldFx)
