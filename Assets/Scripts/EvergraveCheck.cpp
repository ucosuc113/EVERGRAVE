// LAS PRUEBAS QUE NECESITAN UN FRAME: lo que Tests/core_tests.cpp no puede
// comprobar porque depende del motor corriendo. Se ven con
//
//   EVERGRAVE_CHECK=1  Editor.exe EVERGRAVE.strproj --play --play-seconds 5 --frames 2000 --game-size 1280x720
//
// (--game-size no es opcional: con el panel acoplado el ancho interno cambia
// con el aspecto y el Sol, que esta a la derecha, puede quedar fuera de cuadro.)
//
// y se buscan en el log las lineas con OK o FALLA. El dominio (costos,
// requisitos, efectos, interacciones, guardado) ya esta probado sin motor; aqui
// se prueba lo que une las piezas: la escena, el pixelado, la camara, el clic
// del mundo, el reloj y el movimiento de la Tierra.
//
// SOLO CON EVERGRAVE_CHECK=1: la prueba toca la partida (le da puntos y la
// reinicia al final) y este objeto esta en la escena que se juega; sin la
// compuerta, quien jugara perderia su progreso al segundo de empezar.

#include "Services/Session.h"
#include "Services/UseFeedback.h"

#include "Engine/Components/Rendering/SubViewport.h"
#include "Engine/Diagnostics/Log.h"
#include "Engine/Gameplay/GameTime.h"
#include "Engine/Rendering/Camera.h"
#include "Engine/Scene/ComponentQuery.h"
#include "Engine/Scene/Raycast.h"
#include "Engine/Scene/SceneQuery.h"
#include "Engine/Scene/SceneService.h"
#include "Engine/Scene/SceneTransform.h"
#include "Engine/Scripting/IScript.h"
#include "Engine/Scripting/ScriptRegistry.h"
#include "Engine/UI/UiService.h"

#include <DirectXMath.h>

#include <cmath>
#include <cstdlib>
#include <string>

using namespace strace;
using namespace evergrave;

namespace {

void Expect(bool ok, const std::wstring& what, const std::wstring& got = L"") {
    Log::Write(ok ? Log::Level::Info : Log::Level::Error,
               std::wstring(L"[evg] ") + (ok ? L"OK    " : L"FALLA ") + what + (got.empty() ? L"" : L"   (" + got + L")"));
}

std::wstring Num(double v) { return std::to_wstring(v); }
std::wstring Wide(const std::string& s) { return std::wstring(s.begin(), s.end()); }

bool Enabled() {
    static const bool on = [] {
        char buf[8] = {};
        size_t len = 0;
        return getenv_s(&len, buf, sizeof(buf), "EVERGRAVE_CHECK") == 0 && len > 0 && buf[0] == '1';
    }();
    return on;
}

constexpr const char* kEarth = "Space/OrbitPivot/EarthAnchor/Tilt/Earth";
constexpr const char* kSun = "Space/OrbitPivot/Sun";
constexpr const char* kAnchor = "Space/OrbitPivot/EarthAnchor";
constexpr const char* kMoon = "Space/OrbitPivot/EarthAnchor/Moon";

// Proyecta un punto del mundo a pixeles de juego con la matriz de la camara.
bool ToScreen(const GameObject& cam, const DirectX::XMFLOAT3& wp, float& px, float& py, DirectX::XMMATRIX& viewProj) {
    const float w = UiService::Width(), h = UiService::Height();
    viewProj = ComputeGameCameraView(cam) * ComputeGameCameraProjection(*cam.camera, SceneService::Type(), w / h);
    DirectX::XMFLOAT3 ndc;
    DirectX::XMStoreFloat3(&ndc, DirectX::XMVector3TransformCoord(DirectX::XMLoadFloat3(&wp), viewProj));
    px = (ndc.x * 0.5f + 0.5f) * w;
    py = (1.0f - (ndc.y * 0.5f + 0.5f)) * h;
    return px > 0 && px < w && py > 0 && py < h;
}

// Un rayo por el centro de `obj` tiene que devolver `obj` (o una pieza suya).
void ExpectPickable(const GameObject& cam, const GameObject& obj, const char* prefix, const std::wstring& what) {
    float px = 0, py = 0;
    DirectX::XMMATRIX vp;
    const bool onScreen = ToScreen(cam, ComputeWorldPosition(obj), px, py, vp); // antes de armar el texto: el orden de los argumentos no esta definido
    Expect(onScreen, what + L" cae dentro de la pantalla", Num(px) + L"," + Num(py));
    Ray ray;
    RaycastHit hit;
    const bool shot = ScreenPointToRay(px, py, UiService::Width(), UiService::Height(), vp, ray) && SceneService::Raycast(ray, hit) && hit.object;
    const std::string path = shot ? PathOf(*hit.object) : std::string();
    Expect(shot && path.rfind(prefix, 0) == 0, L"un rayo por el centro de " + what + L" lo acierta", shot ? Wide(path) : L"no acerto nada");
}

} // namespace

class EvergraveCheck : public IScript {
public:
    void OnUpdate(float) override {
        if (!Enabled()) return;
        ++m_frame;
        if (m_stage == 0 && m_frame >= 5) {
            CheckScene();
            CheckUnlockFlow();
            BeginProductionTest();
            m_stage = 1;
        } else if (m_stage == 1 && GameTime::Elapsed() - m_t0 >= 1.0) {
            FinishProductionTest();
            m_stage = 2;
        }
    }

private:
    void CheckScene() {
        const auto& roots = SceneService::Roots();
        const GameObject* earth = FindByPath(roots, kEarth);
        const GameObject* sun = FindByPath(roots, kSun);
        const GameObject* sunlight = FindByPath(roots, "Space/OrbitPivot/Sunlight");
        const GameObject* cam = FindActiveCamera(roots);
        Expect(earth && !earth->meshPath.empty(), L"la escena tiene la Tierra, con malla propia");
        Expect(sun != nullptr, L"la escena tiene el Sol");
        // La luz del Sol es direccional y SpaceWorld la apunta cada frame del Sol a la Tierra (orbitas de verdad).
        Expect(sunlight && sunlight->light && sunlight->light->type == LightType::Directional, L"el Sol ilumina (luz direccional)");
        Expect(cam && cam->camera, L"hay una camara activa");

        GameObject* pv = FindByPath(SceneService::MutableRoots(), "PixelViewport");
        const SubViewport* sv = pv ? GetComponent<SubViewport>(*pv) : nullptr;
        Expect(sv && sv->pixelate && sv->baseHeight == 360, L"PixelViewport: 360 filas con reescalado entero", sv ? Num(sv->baseHeight) : L"sin componente");
        Expect(std::fabs(UiService::Height() - 360.0f) < 0.5f, L"la UI se dibuja a 360 filas internas", Num(UiService::Height()));

        // El area visible: no vacia, dentro de la imagen, y los HUD se anclan a ella.
        const UiRect vis = UiService::VisibleRect();
        Expect(vis.width > 0 && vis.height > 0 && vis.x >= 0 && vis.y >= 0 && vis.Right() <= UiService::Width() + 0.5f &&
                   vis.Bottom() <= UiService::Height() + 0.5f,
               L"UiService::VisibleRect cabe dentro de la imagen y no esta vacia",
               Num(vis.x) + L"," + Num(vis.y) + L" " + Num(vis.width) + L"x" + Num(vis.height));
        if (!earth || !sun || !cam || !cam->camera) return;

        // Mira desde arriba, inclinada. La camara la mueve CameraRig (por defecto 35 grados sobre el plano del sistema;
        // el mapa, 75), asi que se mide en el MUNDO y con un margen.
        const DirectX::XMFLOAT3 fwd = ComputeWorldForward(*cam);
        const float pitch = std::asin(-fwd.y) * 180.0f / 3.14159265f;
        Expect(pitch > 15.0f && pitch < 80.0f, L"la camara mira desde arriba, inclinada", Num(pitch) + L" grados");

        float px = 0, py = 0;
        DirectX::XMMATRIX vp;
        ToScreen(*cam, ComputeWorldPosition(*earth), px, py, vp);
        Expect(std::fabs(px - UiService::Width() * 0.5f) < 20.0f && std::fabs(py - UiService::Height() * 0.5f) < 20.0f,
               L"la Tierra esta en el centro de la imagen", Num(px) + L"," + Num(py));

        ExpectPickable(*cam, *earth, kEarth, L"la Tierra");

        // El sistema: el Sol al centro, la Tierra a 30 unidades de el y la Luna a 8 de la Tierra. En la vista de la Tierra el
        // Sol casi nunca esta en cuadro (se ve desde la vista del Sol o el mapa), asi que se comprueba DONDE esta, no si se ve.
        const DirectX::XMFLOAT3 sp = ComputeWorldPosition(*sun), ep = ComputeWorldPosition(*earth);
        // Mirando la Tierra el Sol se ve LEJOS (SpaceWorld lo aleja): tiene que estar sobre la linea de la Tierra al centro del
        // sistema, mas alla de el, y bastante mas lejos que los 30 de su sitio real.
        {
            const float len = std::sqrt(ep.x * ep.x + ep.z * ep.z);
            const float along = ((sp.x - ep.x) * (-ep.x) + (sp.z - ep.z) * (-ep.z)) / len;           // cuanto se aleja de la Tierra hacia el centro
            const float off = std::fabs((sp.x - ep.x) * (-ep.z) + (sp.z - ep.z) * ep.x) / len;      // cuanto se desvia de esa linea
            Expect(off < 0.5f && along > 100.0f, L"mirando la Tierra el Sol se ve lejos, sobre la linea al centro del sistema",
                   Num(along) + L" lejos, " + Num(off) + L" de desvio");
        }
        Expect(std::fabs(std::sqrt(ep.x * ep.x + ep.z * ep.z) - 30.0f) < 0.5f, L"la Tierra orbita a 30 unidades del Sol", Num(std::sqrt(ep.x * ep.x + ep.z * ep.z)));
        const GameObject* moon = FindByPath(roots, kMoon);
        Expect(moon && !moon->meshPath.empty(), L"la escena tiene la Luna, con malla propia");
        if (moon) {
            const DirectX::XMFLOAT3 mp = ComputeWorldPosition(*moon);
            const float d = std::sqrt((mp.x - ep.x) * (mp.x - ep.x) + (mp.z - ep.z) * (mp.z - ep.z));
            Expect(std::fabs(d - 8.0f) < 0.2f, L"la Luna orbita a 8 unidades de la Tierra", Num(d));
        }
        Expect(!Session().IsMapUnlocked() && !Session().IsBodyUnlocked(BodyId::Moon) && Ui().view == BodyId::Earth && !Ui().mapView,
               L"la partida empieza en la Tierra, con el mapa y la Luna cerrados");

        m_earthSpin0 = earth->transform.rotation[1];
        const GameObject* anchor = FindByPath(roots, kAnchor);
        const GameObject* moon0 = FindByPath(roots, kMoon);
        m_orbit0 = anchor ? std::atan2(anchor->transform.position[2], anchor->transform.position[0]) * 180.0f / 3.14159265f : 0.0f;
        m_month0 = moon0 ? std::atan2(moon0->transform.position[2], moon0->transform.position[0]) * 180.0f / 3.14159265f : 0.0f;
    }

    // El Sol bloqueado no rinde y dice que investigar; la Tierra si rinde.
    void CheckUnlockFlow() {
        GameState& g = Session();
        Expect(g.IsUnlocked("gather") && !g.IsUnlocked("solar_collector"), L"al empezar: la Tierra abierta, el Sol bloqueado");
        const double before = g.wallet().Get(ResourceId::Research);
        Expect(UseWithFeedback("solar_collector", 0, 0) == UseResult::Locked, L"clicar el Sol bloqueado lo rechaza");
        Expect(g.wallet().Get(ResourceId::Research) == before, L"y no gana puntos");
        Expect(Ui().toastTime > 0.0f && Ui().toast.find("Solar Collectors") != std::string::npos, L"y dice que tecnologia hay que investigar", Wide(Ui().toast));
        Expect(UseWithFeedback("gather", 0, 0) == UseResult::Ok && g.wallet().Get(ResourceId::Research) > before, L"clicar la Tierra gana puntos reales");
    }

    // Produccion pasiva medida contra el reloj del motor.
    void BeginProductionTest() {
        GameState& g = Session();
        g.MutableWallet().Add(ResourceId::Research, 1000.0);
        g.Research("instrumentation");
        g.Research("solar_collection");
        Expect(UseWithFeedback("solar_collector", 0, 0) == UseResult::Ok, L"con Solar Collectors, el Sol rinde");
        g.Research("steam_generators");
        Expect(g.PassiveRate() > 0.0, L"con Steam Generators hay produccion pasiva", Num(g.PassiveRate()));
        m_t0 = GameTime::Elapsed();
        m_sessionT0 = g.Elapsed();
        m_balance0 = g.wallet().Get(ResourceId::Research);
    }

    void FinishProductionTest() {
        GameState& g = Session();
        const double engineDt = GameTime::Elapsed() - m_t0;
        const double sessionDt = g.Elapsed() - m_sessionT0;
        // Si GameRoot ticara dos veces por frame, sessionDt seria ~2x; si no, ~0.
        Expect(std::fabs(sessionDt - engineDt) < 0.05, L"GameRoot avanza la sesion al ritmo del reloj del motor", Num(sessionDt) + L" vs " + Num(engineDt));
        const double gained = g.wallet().Get(ResourceId::Research) - m_balance0;
        Expect(std::fabs(gained - g.PassiveRate() * sessionDt) < 1e-6, L"lo producido es tasa x tiempo", Num(gained) + L" vs " + Num(g.PassiveRate() * sessionDt));

        const GameObject* earth = FindByPath(SceneService::Roots(), kEarth);
        const GameObject* anchor = FindByPath(SceneService::Roots(), kAnchor);
        const GameObject* moon = FindByPath(SceneService::Roots(), kMoon);
        Expect(earth && std::fabs(earth->transform.rotation[1] - m_earthSpin0) > 1.0f, L"la Tierra rota", earth ? Num(earth->transform.rotation[1] - m_earthSpin0) : L"-");
        const float orbit = anchor ? std::atan2(anchor->transform.position[2], anchor->transform.position[0]) * 180.0f / 3.14159265f : 0.0f;
        Expect(anchor && std::fabs(orbit - m_orbit0) > 0.3f, L"la Tierra orbita alrededor del Sol", Num(orbit - m_orbit0));
        const float month = moon ? std::atan2(moon->transform.position[2], moon->transform.position[0]) * 180.0f / 3.14159265f : 0.0f;
        Expect(moon && std::fabs(month - m_month0) > 2.0f, L"la Luna orbita alrededor de la Tierra", Num(month - m_month0));
        ResetSession();
        ResetUi();
        Log::Info(L"[evg] fin de las pruebas con frame");
    }

    int m_frame = 0;
    int m_stage = 0;
    float m_earthSpin0 = 0.0f, m_orbit0 = 0.0f, m_month0 = 0.0f;
    double m_t0 = 0.0, m_sessionT0 = 0.0, m_balance0 = 0.0;
};

STRACE_REGISTER_SCRIPT(EvergraveCheck)
