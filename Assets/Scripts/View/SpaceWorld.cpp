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
// La orbita es CONCEPTUAL: la camara, la Tierra y el Sol son hijos de un pivote
// que gira, asi que Tierra y Sol se ven siempre iguales y lo que cambia es el
// fondo - las estrellas, que no giran con el pivote, derivan como lo harian
// vistas desde un planeta que da la vuelta. (Una orbita con la camara quieta
// sacaria a la Tierra de cuadro en un minuto, o dejaria al Sol detras de la
// camara la mitad del tiempo.)
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

constexpr const char* kPivotPath = "Space/OrbitPivot";
constexpr const char* kEarthPath = "Space/OrbitPivot/EarthAnchor/Tilt/Earth";
constexpr const char* kSunPath = "Space/OrbitPivot/Sun";

// Grados por segundo. La orbita completa tarda 7,5 minutos y el dia terrestre 45 s:
// lo bastante lento para no marear, lo bastante rapido para verse vivo.
constexpr float kOrbitDegPerSec = 0.8f;
constexpr float kSpinDegPerSec = 8.0f;

bool StartsWith(const std::string& s, const char* prefix) { return s.rfind(prefix, 0) == 0; }

float Wrap360(float a) { return a >= 360.0f ? a - 360.0f : a; }

} // namespace

class SpaceWorld : public IScript {
public:
    void OnStart() override {
        m_pivot = FindByPath(SceneService::MutableRoots(), kPivotPath);
        m_earth = FindByPath(SceneService::MutableRoots(), kEarthPath);
        m_sun = FindByPath(SceneService::MutableRoots(), kSunPath);
        if (!m_pivot || !m_earth || !m_sun) Log::Warn(L"EVERGRAVE: la escena no tiene Space/OrbitPivot con Earth y Sun");
        if (m_earth) m_earthBaseScale = m_earth->transform.scale[0];
        if (m_sun) m_sunBaseScale = m_sun->transform.scale[0];
    }

    void OnUpdate(float dt) override {
        Animate(dt);
        HandlePointer();
    }

private:
    void Animate(float dt) {
        if (m_pivot) m_pivot->transform.rotation[1] = Wrap360(m_pivot->transform.rotation[1] + kOrbitDegPerSec * dt);
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
        if (StartsWith(path, kSunPath)) { out = InteractionTarget::Sun; return true; }
        return false;
    }

    void HandlePointer() {
        GameState& game = Session();
        InputActions& in = InputService::Actions();

        InteractionTarget target;
        if (!Pick(target)) return;
        const auto unlocked = [&](const InteractionId& id) { return game.IsUnlocked(id); };
        const InteractionDef* def = game.interactions().ForTarget(target, unlocked);
        if (!def) return;

        const float cx = in.CursorX(), cy = in.CursorY();
        DrawHoverLabel(*def, game, cx, cy);

        if (!in.Pressed("ev_click")) return;
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

    GameObject* m_pivot = nullptr;
    GameObject* m_earth = nullptr;
    GameObject* m_sun = nullptr;
    float m_earthBaseScale = 1.0f, m_sunBaseScale = 1.0f;
    float m_earthPulse = 0.0f, m_sunPulse = 0.0f;
};

STRACE_REGISTER_SCRIPT(SpaceWorld)
