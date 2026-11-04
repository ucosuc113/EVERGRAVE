// CameraRig: la camara del juego. Gira alrededor del cuerpo que se mira, o se aleja al MAPA
// del sistema, y de paso decide que fue un clic y que un arrastre.
//
// DOS MODOS (Ui().view / Ui().mapView):
//  - Cuerpo (Tierra, Luna, Sol): la camara orbita ese cuerpo. Se arrastra con el boton
//    izquierdo para girar, la rueda acerca y aleja, y si nadie la toca un rato gira despacio
//    sola. El angulo se mide respecto a la direccion al Sol (0 = del lado del dia), asi
//    "la cara iluminada" no depende de en que punto de su orbita este el planeta.
//  - Mapa: desde arriba casi del todo (75 grados, no 90: se sigue viendo que es un espacio con
//    volumen) y lo bastante lejos para que quepa el sistema entero. Todo se ve chico pero se
//    puede tocar: un clic cerca de un cuerpo lo elige, y la camara vuela hasta el.
//
// Al EMPEZAR la persona esta en la Tierra y no puede salir (el mapa esta cerrado): lo abre
// una tecnologia (`UnlockBody` "map"), y cada cuerpo, otra.
//
// EL CLIC Y EL ARRASTRE COMPARTEN EL BOTON. Antes el clic recolectaba al APRETAR; ahora, para
// poder arrastrar sin recolectar, se decide al SOLTAR: si el puntero casi no se movio fue un
// clic (`Ui().worldClick`, que lee SpaceWorld), y si se movio fue un arrastre. Un arrastre que
// empieza sobre un panel no cuenta: eso es de la interfaz.
//
// Corre justo despues de GameRoot, antes del HUD y los paneles: lo que dibuja aqui (las etiquetas
// del mapa) queda DEBAJO de la interfaz. Tambien mueve el cielo (`Space/Stars`) con la camara:
// las estrellas estan a 80 unidades del origen y la camara ya no se queda cerca de el.

#include "../Services/Controls.h"
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

constexpr const char* kCameraPath = "Space/OrbitPivot/Camara";
constexpr const char* kStarsPath = "Space/Stars";
constexpr const char* kEarthPath = "Space/OrbitPivot/EarthAnchor/Tilt/Earth";
constexpr const char* kMoonPath = "Space/OrbitPivot/EarthAnchor/Moon";
constexpr const char* kSunPath = "Space/OrbitPivot/Sun";

constexpr float kRad = 3.14159265f / 180.0f;
constexpr float kDragSlop = 4.0f;
constexpr float kIdleBeforeSpin = 2.5f;   // segundos
constexpr float kSpinDegPerSec = 1.4f;
constexpr float kMapDist = 96.0f, kMapPitch = 75.0f;
constexpr float kPickRadius = 18.0f;      // pixeles de juego: el blanco de un cuerpo en el mapa

// La diferencia mas corta entre dos angulos, en grados.
float AngleDelta(float from, float to) {
    float d = std::fmod(to - from, 360.0f);
    if (d > 180.0f) d -= 360.0f;
    if (d < -180.0f) d += 360.0f;
    return d;
}

struct Pose {
    XMFLOAT3 center{0, 0, 0};
    float dist = 13.0f;
    float yaw = 0.0f;   // grados
    float pitch = 35.0f;
};

} // namespace

class CameraRig : public IScript {
public:
    void OnStart() override {
        m_cam = FindByPath(SceneService::MutableRoots(), kCameraPath);
        m_stars = FindByPath(SceneService::MutableRoots(), kStarsPath);
        m_earth = FindByPath(SceneService::MutableRoots(), kEarthPath);
        m_moon = FindByPath(SceneService::MutableRoots(), kMoonPath);
        m_sun = FindByPath(SceneService::MutableRoots(), kSunPath);
        m_snap = true;
    }

    void OnUpdate(float dt) override {
        if (!m_cam || !m_earth || !m_moon || !m_sun) return;
        UiState& state = Ui();
        GameState& game = Session();
        InputActions& in = InputService::Actions();

        // --- Teclado: cambiar de vista ---
        if (in.Pressed("ev_view_earth")) GoToBody(BodyId::Earth);
        if (in.Pressed("ev_view_moon") && !GoToBody(BodyId::Moon)) Toast("The Moon is not open yet", 1.6f);
        if (in.Pressed("ev_view_sun") && !GoToBody(BodyId::Sun)) Toast("The Sun is not open yet", 1.6f);
        if (in.Pressed("ev_map")) {
            if (state.mapView) GoToBody(state.view);
            else if (!GoToMap()) Toast("The system map is not open yet", 1.6f);
        }

        HandleMouse(state, in, dt);

        // --- A donde mira la camara ---
        // El Sol "de verdad" esta siempre en el centro del sistema; el que se ve se aleja (SpaceWorld) cuando se mira la
        // Tierra o la Luna, asi que para apuntar la camara y marcar el mapa se usa el punto fijo.
        const XMFLOAT3 earth = ComputeWorldPosition(*m_earth), moon = ComputeWorldPosition(*m_moon), sun(0, 0, 0);
        Pose target;
        if (state.mapView) {
            target.center = XMFLOAT3(0, 0, 0);
            target.dist = kMapDist;
            target.pitch = kMapPitch;
            target.yaw = 0.0f;
        } else {
            const XMFLOAT3& c = state.view == BodyId::Moon ? moon : state.view == BodyId::Sun ? sun : earth;
            target.center = c;
            target.dist = (state.view == BodyId::Moon ? 5.5f : state.view == BodyId::Sun ? 36.0f : 16.0f) * state.camZoom;
            target.pitch = state.camPitch;
            // El angulo se mide respecto a la direccion al Sol (para el Sol mismo, desde +z) y, para la Luna,
            // respecto a la direccion a la TIERRA: asi, con el desvio por defecto, la Tierra queda de lado en
            // pantalla en vez de escondida detras de la Luna o a la espalda de la camara.
            float ref = 0.0f;
            if (state.view == BodyId::Moon) ref = std::atan2(earth.x - c.x, earth.z - c.z) / kRad;
            else if (state.view != BodyId::Sun) ref = std::atan2(-c.x, -c.z) / kRad;
            target.yaw = ref + state.camYaw;
        }

        if (m_snap) {
            m_pose = target;
            m_snap = false;
        } else {
            // Suavizado exponencial: la camara llega en un segundo o dos sin arrancar ni frenar de golpe.
            const float k = 1.0f - std::exp(-3.2f * dt);
            m_pose.center.x += (target.center.x - m_pose.center.x) * k;
            m_pose.center.y += (target.center.y - m_pose.center.y) * k;
            m_pose.center.z += (target.center.z - m_pose.center.z) * k;
            m_pose.dist += (target.dist - m_pose.dist) * k;
            m_pose.pitch += (target.pitch - m_pose.pitch) * k;
            m_pose.yaw += AngleDelta(m_pose.yaw, target.yaw) * k;
        }

        // --- Ponerla ---
        const float p = m_pose.pitch * kRad, a = m_pose.yaw * kRad;
        const XMFLOAT3 pos(m_pose.center.x + m_pose.dist * std::cos(p) * std::sin(a), m_pose.center.y + m_pose.dist * std::sin(p),
                           m_pose.center.z + m_pose.dist * std::cos(p) * std::cos(a));
        m_cam->transform.position[0] = pos.x;
        m_cam->transform.position[1] = pos.y;
        m_cam->transform.position[2] = pos.z;
        // Mira al centro: el angulo en Y hacia el, y el de arriba/abajo por la altura.
        m_cam->transform.rotation[0] = m_pose.pitch;
        m_cam->transform.rotation[1] = std::atan2(m_pose.center.x - pos.x, m_pose.center.z - pos.z) / kRad;
        m_cam->transform.rotation[2] = 0.0f;
        if (m_stars) {
            m_stars->transform.position[0] = pos.x;
            m_stars->transform.position[1] = pos.y;
            m_stars->transform.position[2] = pos.z;
        }

        if (state.mapView) DrawMap(game, state, in, earth, moon, sun);
    }

private:
    // Clic, arrastre y rueda sobre el mundo.
    void HandleMouse(UiState& state, InputActions& in, float dt) {
        const bool freeOfUi = !state.pointerOverUiLast && in.CursorInside();
        if (freeOfUi && in.Pressed("ev_click")) {
            state.camDragging = true;
            state.camDragDist = 0.0f;
        }
        if (state.camDragging) {
            if (in.Held("ev_click")) {
                const float dx = in.CursorDeltaX(), dy = in.CursorDeltaY();
                state.camDragDist += std::fabs(dx) + std::fabs(dy);
                // En el mapa no se gira (esta fijo a proposito: si se mueve, tocar un cuerpo chico se vuelve adivinar).
                if (state.camDragDist >= kDragSlop && !state.mapView) {
                    state.camYaw -= dx * 0.4f;
                    state.camPitch = std::clamp(state.camPitch + dy * 0.3f, 12.0f, 80.0f);
                    state.camIdle = 0.0f;
                }
            }
            if (in.Released("ev_click")) {
                state.camDragging = false;
                if (state.camDragDist < kDragSlop) state.worldClick = true;
            }
        }
        if (freeOfUi && !state.mapView && in.WheelDelta() != 0.0f) {
            state.camZoom = std::clamp(state.camZoom * (1.0f - in.WheelDelta() * 0.1f), 0.55f, 1.9f);
            state.camIdle = 0.0f;
        }
        // Quieta un rato: gira sola, despacio.
        state.camIdle += dt;
        if (!state.mapView && !state.camDragging && state.camIdle > kIdleBeforeSpin) state.camYaw += kSpinDegPerSec * dt;
    }

    // El mapa: las orbitas, un nombre junto a cada cuerpo, y el clic para elegir.
    void DrawMap(GameState& game, UiState& state, InputActions& in, const XMFLOAT3& earth, const XMFLOAT3& moon, const XMFLOAT3& sun) {
        const GameObject* camObj = FindActiveCamera(SceneService::Roots());
        if (!camObj || !camObj->camera) return;
        const float w = UiService::Width(), h = UiService::Height();
        if (w <= 0.0f || h <= 0.0f) return;
        const XMMATRIX view = ComputeGameCameraView(*camObj);
        const XMMATRIX proj = ComputeGameCameraProjection(*camObj->camera, SceneService::Type(), w / h);
        const auto project = [&](const XMFLOAT3& wp, float& sx, float& sy) {
            const XMVECTOR v = XMVector3TransformCoord(XMLoadFloat3(&wp), view);
            if (XMVectorGetZ(v) <= 0.05f) return false;
            const XMVECTOR c = XMVector3TransformCoord(v, proj);
            sx = (XMVectorGetX(c) * 0.5f + 0.5f) * w;
            sy = (0.5f - XMVectorGetY(c) * 0.5f) * h;
            return true;
        };

        UiDrawList& ui = UiService::Frame();
        // Las orbitas: la de la Tierra alrededor del Sol y la de la Luna alrededor de la Tierra.
        const auto ring = [&](const XMFLOAT3& center, float radius, int steps, const UiColor& c) {
            float px = 0, py = 0;
            bool have = false;
            for (int i = 0; i <= steps; ++i) {
                const float a = 2.0f * 3.14159265f * static_cast<float>(i) / static_cast<float>(steps);
                float sx, sy;
                const bool ok = project(XMFLOAT3(center.x + radius * std::cos(a), center.y, center.z + radius * std::sin(a)), sx, sy);
                if (ok && have) ui.AddLine(px, py, sx, sy, c, 1.0f);
                have = ok;
                px = sx;
                py = sy;
            }
        };
        ring(XMFLOAT3(0, 0, 0), std::sqrt(earth.x * earth.x + earth.z * earth.z), 96, UiColor::Bytes(120, 140, 190, 70));
        ring(earth, 8.0f, 40, UiColor::Bytes(170, 170, 190, 80));

        struct Marker { BodyId body; XMFLOAT3 pos; float sx, sy; bool ok; };
        Marker m[3] = {{BodyId::Sun, sun, 0, 0, false}, {BodyId::Earth, earth, 0, 0, false}, {BodyId::Moon, moon, 0, 0, false}};
        for (Marker& k : m) k.ok = project(k.pos, k.sx, k.sy);

        // El mas cercano al puntero, si esta dentro del blanco.
        int hover = -1;
        float best = kPickRadius * kPickRadius;
        if (in.CursorInside() && !state.pointerOverUiLast) {
            for (int i = 0; i < 3; ++i) {
                if (!m[i].ok) continue;
                const float dx = in.CursorX() - m[i].sx, dy = in.CursorY() - m[i].sy;
                if (dx * dx + dy * dy < best) {
                    best = dx * dx + dy * dy;
                    hover = i;
                }
            }
        }
        if (state.worldClick && hover >= 0) {
            const BodyId b = m[hover].body;
            if (!GoToBody(b)) Toast(std::string(InfoOf(b).name) + " is not open yet", 1.6f);
            state.worldClick = false;   // el clic fue de la camara, no del mundo
        }

        for (int i = 0; i < 3; ++i) {
            if (!m[i].ok) continue;
            const bool open = game.IsBodyUnlocked(m[i].body);
            const UiColor base = open ? kit::Text() : kit::Faint();
            std::string label = InfoOf(m[i].body).name;
            if (!open) label += " (LOCKED)";
            else if (const int n = game.OwnedOn(m[i].body); n > 0) label += "  x" + std::to_string(n);
            // El nombre junto al cuerpo, cada uno por su lado para que no se pisen: el del Sol arriba de su disco,
            // el de la Tierra a su izquierda y el de la Luna a su derecha.
            const float lw = kit::TextW(label);
            float lx = m[i].sx - lw * 0.5f, ly = m[i].sy - 18.0f;
            if (m[i].body == BodyId::Sun) ly = m[i].sy - 42.0f;
            if (m[i].body == BodyId::Earth) { lx = m[i].sx - lw - 12.0f; ly = m[i].sy - 4.0f; }
            if (m[i].body == BodyId::Moon) { lx = m[i].sx + 14.0f; ly = m[i].sy - 4.0f; }
            ui.AddText(std::floor(lx), std::floor(ly), label, i == hover ? kit::Text() : base.WithAlpha(0.85f), 1);
            if (i == hover) {
                const float r = 11.0f;
                ui.AddRectOutline(kit::Rect(std::floor(m[i].sx - r), std::floor(m[i].sy - r), r * 2, r * 2), open ? kit::Red() : kit::Dim(), 1.0f);
            }
        }
        ui.AddText(std::floor(UiService::VisibleRect().x + UiService::VisibleRect().width * 0.5f - kit::TextW("CLICK A BODY TO GO THERE") * 0.5f),
                   std::floor(UiService::VisibleRect().Bottom() - 50.0f), "CLICK A BODY TO GO THERE", kit::Dim(), 1);
    }

    GameObject* m_cam = nullptr;
    GameObject* m_stars = nullptr;
    GameObject* m_earth = nullptr;
    GameObject* m_moon = nullptr;
    GameObject* m_sun = nullptr;
    Pose m_pose;
    bool m_snap = true;
};

STRACE_REGISTER_SCRIPT(CameraRig)
