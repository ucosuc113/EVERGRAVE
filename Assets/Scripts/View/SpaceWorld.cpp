// SpaceWorld: el entorno principal. La Tierra vista desde el espacio, el Sol, y
// los dos elementos del mundo a los que se les hace clic.
//
// EL MUNDO NO CONTIENE REGLAS NI PROGRESO. Lo que hace es:
//  - ANIMAR (rotacion de la Tierra, orbita alrededor del Sol): solo atmosfera, no
//    depende de la partida y no la cambia;
//  - TRADUCIR un clic: del cursor al objeto que el rayo acierta, de ese objeto a
//    su `InteractionTarget` (Tierra o Sol) y de ahi a la interaccion que el Core
//    tenga para ese elemento. Quien decide si el clic produce algo - esta
//    desbloqueada, esta en espera - es `GameState::Use`, no esta clase.
//
// Por eso el Sol "esta ahi" desde el primer frame y no hace nada hasta que se
// investiga Solar Collectors: no hay una casilla "activo" en la escena que
// alguien encienda, hay una interaccion que el Core rechaza como Locked, y el
// puntero lo dice (etiqueta junto al cursor) en vez de ignorar el clic.
//
// LAS ORBITAS SON DE VERDAD (a escala de dibujo, no astronomica): el Sol esta en el centro, la
// Tierra gira a su alrededor y la Luna alrededor de la Tierra. Lo que cambia entre una vista y
// otra no es el mundo sino la camara (View/CameraRig.cpp), que orbita el cuerpo que se mira o
// se aleja al mapa. La luz del Sol es direccional y se apunta cada frame de el a la Tierra, asi
// la mitad de dia y la de noche siguen a la orbita y la Luna tiene sus fases.
//
// Los objetos se buscan por ruta (Space/OrbitPivot/...) y no por campos de
// Inspector: son nombres que esta escena define y que esta clase conoce.

#include "../Services/Controls.h"
#include "../Services/Session.h"
#include "../Services/UseFeedback.h"
#include "UiKit.h"

#include "Engine/Diagnostics/Log.h"
#include "Engine/Rendering/Camera.h"
#include "Engine/Scene/GameObject.h"
#include "Engine/Scene/Raycast.h"
#include "Engine/Scene/SceneQuery.h"
#include "Engine/Scene/SceneService.h"
#include "Engine/Scripting/IScript.h"
#include "Engine/Scripting/ScriptRegistry.h"
#include "Engine/UI/UiService.h"

#include <DirectXMath.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

using namespace strace;
using namespace evergrave;

namespace {

constexpr const char* kAnchorPath = "Space/OrbitPivot/EarthAnchor";
constexpr const char* kMoonPath = "Space/OrbitPivot/EarthAnchor/Moon";
constexpr const char* kLightPath = "Space/OrbitPivot/Sunlight";
constexpr const char* kEarthPath = "Space/OrbitPivot/EarthAnchor/Tilt/Earth";
constexpr const char* kSunPath = "Space/OrbitPivot/Sun";

// Grados por segundo. La orbita de la Tierra tarda 7,5 minutos, el dia terrestre 45 s y el mes de
// la Luna 45 s tambien: lento para no marear, rapido para verse vivo.
constexpr float kOrbitDegPerSec = 0.8f;
constexpr float kSpinDegPerSec = 8.0f;
constexpr float kMoonDegPerSec = 8.0f;
constexpr float kEarthOrbitRadius = 30.0f; // del Sol a la Tierra
constexpr float kMoonOrbitRadius = 8.0f;    // de la Tierra a la Luna
constexpr float kRad = 3.14159265f / 180.0f;
// A que distancia de la Tierra se VE el Sol cuando se mira la Tierra o la Luna. A escala de dibujo el Sol esta a 30
// unidades y mide 5: desde la Tierra llena un cuarto del cielo y, al girar la camara, rompe la ilusion de que es
// una estrella lejana. En el mapa y en la vista del Sol se queda donde esta (30 del centro); en las otras dos se aleja
// por la MISMA linea (misma direccion: la luz y el lado de dia no cambian) hasta que se ve chico.
constexpr float kSunFarDistance = 150.0f;

bool StartsWith(const std::string& s, const char* prefix) { return s.rfind(prefix, 0) == 0; }

float Wrap360(float a) { return a >= 360.0f ? a - 360.0f : a; }

} // namespace

class SpaceWorld : public IScript {
public:
    void OnStart() override {
        m_anchor = FindByPath(SceneService::MutableRoots(), kAnchorPath);
        m_earth = FindByPath(SceneService::MutableRoots(), kEarthPath);
        m_moon = FindByPath(SceneService::MutableRoots(), kMoonPath);
        m_sun = FindByPath(SceneService::MutableRoots(), kSunPath);
        m_light = FindByPath(SceneService::MutableRoots(), kLightPath);
        if (!m_anchor || !m_earth || !m_moon || !m_sun) Log::Warn(L"EVERGRAVE: la escena no tiene Sol, Tierra y Luna en Space/OrbitPivot");
        if (m_earth) m_earthBaseScale = m_earth->transform.scale[0];
        if (m_sun) m_sunBaseScale = m_sun->transform.scale[0];
    }

    void OnUpdate(float dt) override {
        Animate(dt);
        HandlePointer();
    }

private:
    void Animate(float dt) {
        // La Tierra en su orbita (y el Sol al centro); la Luna en la suya.
        m_orbit = Wrap360(m_orbit + kOrbitDegPerSec * dt);
        m_month = Wrap360(m_month + kMoonDegPerSec * dt);
        if (m_anchor) {
            m_anchor->transform.position[0] = kEarthOrbitRadius * std::cos(m_orbit * kRad);
            m_anchor->transform.position[1] = 0.0f;
            m_anchor->transform.position[2] = kEarthOrbitRadius * std::sin(m_orbit * kRad);
        }
        if (m_moon) {
            m_moon->transform.position[0] = kMoonOrbitRadius * std::cos(m_month * kRad);
            m_moon->transform.position[2] = kMoonOrbitRadius * std::sin(m_month * kRad);
            m_moon->transform.rotation[1] = -m_month; // siempre la misma cara a la Tierra
        }
        // El Sol que se VE: en su sitio para el mapa y la vista del Sol, lejos para la Tierra y la Luna. Se mueve con
        // suavidad, al ritmo de la camara, para que al cambiar de vista se aleje o se acerque en vez de saltar.
        if (m_sun && m_anchor) {
            const bool closeUp = Ui().mapView || Ui().view == BodyId::Sun;
            m_sunFar += ((closeUp ? 0.0f : 1.0f) - m_sunFar) * (1.0f - std::exp(-3.2f * dt));
            const float ex = m_anchor->transform.position[0], ez = m_anchor->transform.position[2];
            const float len = std::max(0.001f, std::sqrt(ex * ex + ez * ez));
            // Desde la Tierra hacia el centro del sistema, y mas alla.
            const float px = ex + (-ex / len) * kSunFarDistance, pz = ez + (-ez / len) * kSunFarDistance;
            m_sun->transform.position[0] = px * m_sunFar;
            m_sun->transform.position[1] = 0.0f;
            m_sun->transform.position[2] = pz * m_sunFar;
        }
        // La luz: del Sol (el origen) hacia la Tierra.
        if (m_light && m_anchor) {
            const float ex = m_anchor->transform.position[0], ez = m_anchor->transform.position[2];
            m_light->transform.rotation[0] = 0.0f;
            m_light->transform.rotation[1] = std::atan2(ex, ez) / kRad;
        }
        if (m_earth) m_earth->transform.rotation[1] = Wrap360(m_earth->transform.rotation[1] + kSpinDegPerSec * dt);

        // Un golpe de escala al usar una interaccion: respuesta del clic, no
        // progreso. Decae solo.
        m_earthPulse = std::max(0.0f, m_earthPulse - dt * 5.0f);
        m_sunPulse = std::max(0.0f, m_sunPulse - dt * 5.0f);
        if (m_earth) SetUniformScale(*m_earth, m_earthBaseScale * (1.0f + 0.04f * m_earthPulse));
        if (m_sun) SetUniformScale(*m_sun, m_sunBaseScale * (1.0f + 0.03f * m_sunPulse));
    }

    static void SetUniformScale(GameObject& o, float s) { o.transform.scale[0] = o.transform.scale[1] = o.transform.scale[2] = s; }

    // Que elemento esta bajo el cursor, por el rayo de la camara del juego.
    bool Pick(InteractionTarget& out) const {
        InputActions& in = InputService::Actions();
        if (!in.CursorInside() || Ui().pointerOverUi) return false;
        const GameObject* cam = FindActiveCamera(SceneService::Roots());
        if (!cam || !cam->camera) return false;
        const float w = UiService::Width(), h = UiService::Height();
        if (w <= 0.0f || h <= 0.0f) return false;

        // La misma matriz con que se dibuja el juego: si no, el clic agarra lo de al lado.
        const DirectX::XMMATRIX viewProj =
            ComputeGameCameraView(*cam) * ComputeGameCameraProjection(*cam->camera, SceneService::Type(), w / h);
        Ray ray;
        RaycastHit hit;
        if (!ScreenPointToRay(in.CursorX(), in.CursorY(), w, h, viewProj, ray)) return false;
        if (!SceneService::Raycast(ray, hit) || !hit.object) return false;

        const std::string path = PathOf(*hit.object);
        if (StartsWith(path, kEarthPath)) { out = InteractionTarget::Earth; return true; }
        // La Luna se recolecta como la Tierra: es otro mundo que se estudia.
        if (StartsWith(path, kMoonPath)) { out = InteractionTarget::Earth; return true; }
        if (StartsWith(path, kSunPath)) { out = InteractionTarget::Sun; return true; }
        return false;
    }

    void HandlePointer() {
        GameState& game = Session();
        InputActions& in = InputService::Actions();

        // En el mapa un clic elige cuerpo (lo maneja CameraRig), y mientras se arrastra no se apunta.
        if (Ui().mapView || Ui().camDragging) return;
        InteractionTarget target;
        if (!Pick(target)) return;
        const auto unlocked = [&](const InteractionId& id) { return game.IsUnlocked(id); };
        const InteractionDef* def = game.interactions().ForTarget(target, unlocked);
        if (!def) return;

        const float cx = in.CursorX(), cy = in.CursorY();
        DrawHoverLabel(*def, game, cx, cy);

        // Se recolecta al SOLTAR, y solo si no fue un arrastre (CameraRig lo decide).
        if (!Ui().worldClick) return;
        if (UseWithFeedback(def->id, cx, cy - 6.0f) == UseResult::Ok) (target == InteractionTarget::Earth ? m_earthPulse : m_sunPulse) = 1.0f;
    }

    // Etiqueta junto al cursor: que es, y si se puede usar. Es como el jugador
    // descubre que el Sol existe como interaccion y que le falta para abrirla.
    static void DrawHoverLabel(const InteractionDef& def, const GameState& game, float x, float y) {
        const bool open = game.IsUnlocked(def.id);
        const double left = game.CooldownLeft(def.id);
        const char* who = def.target == InteractionTarget::Sun ? "SUN" : "EARTH";
        char buf[96];
        if (!open) std::snprintf(buf, sizeof buf, "%s - LOCKED", who);
        else if (left > 0.0) std::snprintf(buf, sizeof buf, "%s - %s (%.1fs)", who, def.name.c_str(), left);
        else std::snprintf(buf, sizeof buf, "%s - %s +%s", who, def.name.c_str(), FormatAmount(game.YieldOf(def.id)).c_str());

        UiDrawList& ui = UiService::Frame();
        const std::string text = buf;
        const float w = kit::TextW(text) + 10.0f;
        // Dentro del area visible: con el recorte del pixelado, el borde de la
        // imagen puede estar fuera de la pantalla.
        const UiRect vis = UiService::VisibleRect();
        const float rx = std::min(x + 12.0f, vis.Right() - w - 4.0f);
        const float ry = std::min(y + 14.0f, vis.Bottom() - 18.0f);
        const UiRect r = kit::Rect(std::floor(rx), std::floor(ry), w, 14);
        ui.AddRect(r, kit::Bar().WithAlpha(0.92f));
        ui.AddRectOutline(r, open && left <= 0.0 ? kit::Red() : kit::Line(), 1.0f);
        ui.AddTextCentered(r, text, open && left <= 0.0 ? kit::Text() : kit::Dim(), 1);
    }

    GameObject* m_anchor = nullptr;
    GameObject* m_moon = nullptr;
    GameObject* m_light = nullptr;
    GameObject* m_earth = nullptr;
    GameObject* m_sun = nullptr;
    float m_earthBaseScale = 1.0f, m_sunBaseScale = 1.0f;
    float m_earthPulse = 0.0f, m_sunPulse = 0.0f;
    float m_orbit = 0.0f, m_month = 0.0f;
    float m_sunFar = 1.0f; // 0 = el Sol en su sitio, 1 = lejos (se empieza en la Tierra)
};

STRACE_REGISTER_SCRIPT(SpaceWorld)
