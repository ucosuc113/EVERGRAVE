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

#include "../Services/Cine.h"
#include "../Services/Controls.h"
#include "../Services/Session.h"
#include "../Services/UseFeedback.h"
#include "UiKit.h"

#include "Engine/Diagnostics/Log.h"
#include "Engine/Rendering/Camera.h"
#include "Engine/Rendering/Material.h"
#include "Engine/Rendering/MaterialCache.h"
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
#include <vector>

using namespace strace;
using namespace evergrave;

namespace {

constexpr const char* kAnchorPath = "Space/OrbitPivot/EarthAnchor";
constexpr const char* kMoonPath = "Space/OrbitPivot/EarthAnchor/Moon";
constexpr const char* kLightPath = "Space/OrbitPivot/Sunlight";
constexpr const char* kEarthPath = "Space/OrbitPivot/EarthAnchor/Tilt/Earth";
constexpr const char* kSunPath = "Space/OrbitPivot/Sun";
constexpr const char* kAtmospherePath = "Space/OrbitPivot/EarthAnchor/Tilt/Earth/Atmosphere";

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
        m_atmosphere = FindByPath(SceneService::MutableRoots(), kAtmospherePath);
        m_light = FindByPath(SceneService::MutableRoots(), kLightPath);
        if (!m_anchor || !m_earth || !m_moon || !m_sun) Log::Warn(L"EVERGRAVE: la escena no tiene Sol, Tierra y Luna en Space/OrbitPivot");
        if (m_earth) m_earthBaseScale = m_earth->transform.scale[0];
        if (m_sun) m_sunBaseScale = m_sun->transform.scale[0];
    }

    void OnUpdate(float dt) override {
        Animate(dt);
        if (Cine().active) {
            DrawCineOverlay(); // al final del frame y de los paneles: las barras y el negro tapan hasta lo que dibujan los efectos
            return;
        }
        HandlePointer();
    }

private:
    void Animate(float dt) {
        // La Tierra en su orbita (y el Sol al centro); la Luna en la suya.
        m_orbit = Wrap360(m_orbit + kOrbitDegPerSec * dt);
        m_month = Wrap360(m_month + kMoonDegPerSec * dt);
        // FILMANDO: los angulos son funcion del tiempo de la toma (Services/Cine.h), no un acumulador: asi la fase de la Luna de cada
        // toma es la que se pidio, y un eclipse es un eclipse. (cos/sin no necesitan el angulo envuelto.)
        if (Cine().active) {
            m_orbit = Cine().f.orbitDeg;
            m_month = Cine().world.monthDeg;
        }
        if (m_anchor) {
            m_anchor->transform.position[0] = kEarthOrbitRadius * std::cos(m_orbit * kRad);
            m_anchor->transform.position[1] = 0.0f;
            m_anchor->transform.position[2] = kEarthOrbitRadius * std::sin(m_orbit * kRad);
        }
        if (m_moon) {
            m_moon->transform.position[0] = kMoonOrbitRadius * std::cos(m_month * kRad);
            m_moon->transform.position[2] = kMoonOrbitRadius * std::sin(m_month * kRad);
            m_moon->transform.rotation[1] = -m_month; // siempre la misma cara a la Tierra
            // Filmando, la Luna esta donde la toma diga (en el baile sale de su orbita y rodea al Sol): su posicion local es la del mundo
            // menos la del ancla de la Tierra (cuelga de ella sin giro), y su tamano crece un poco mientras baila.
            if (Cine().active && m_anchor) {
                const CineWorld& w = Cine().world;
                m_moon->transform.position[0] = w.moon.x - w.earth.x;
                m_moon->transform.position[1] = w.moon.y - w.earth.y;
                m_moon->transform.position[2] = w.moon.z - w.earth.z;
                if (m_moonBaseScale <= 0.0f) m_moonBaseScale = m_moon->transform.scale[0];
                SetUniformScale(*m_moon, m_moonBaseScale * w.moonScale);
            }
        }
        // El Sol que se VE: en su sitio para el mapa y la vista del Sol, lejos para la Tierra y la Luna. Se mueve con
        // suavidad, al ritmo de la camara, para que al cambiar de vista se aleje o se acerque en vez de saltar.
        if (m_sun && m_anchor && Cine().active) {
            // La distancia a la que se VE el Sol la pide la toma (un eclipse necesita un Sol del tamano de la Luna).
            m_sun->transform.position[0] = Cine().world.sun.x;
            m_sun->transform.position[1] = Cine().world.sun.y; // distinta de 0 en el eclipse: el Sol queda sobre el rayo camara -> Luna
            m_sun->transform.position[2] = Cine().world.sun.z;
            m_sunFar = Cine().f.k.sunDist > 0.0f ? 1.0f : 0.0f;
        } else if (m_sun && m_anchor) {
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
        if (m_earth && Cine().active) m_earth->transform.rotation[1] = std::fmod(Cine().f.spinDeg, 360.0f);

        // La Tierra DE FONDO: mirando la Luna, la Tierra esta detras y se apaga y enfria (Earth.hlsl, UserParams[1].x), para
        // que la profundidad se lea por el valor y las dos no se fundan. Suavizado al ritmo de la camara, asi el cambio
        // acompana al vuelo en vez de saltar. Es por RUTA del material: la unica Tierra que lo usa es esta.
        const bool moonView = !Ui().menu && !Ui().mapView && Ui().view == BodyId::Moon;
        m_earthBack += ((moonView ? 1.0f : 0.0f) - m_earthBack) * (1.0f - std::exp(-3.2f * dt));
        if (Cine().active) m_earthBack = moonView ? 1.0f : 0.0f; // un corte es un corte: la Tierra de fondo no se desliza
        // Va a la Tierra y a su capa de aire (Earth/Atmosphere): un halo encendido sobre una Tierra apagada se veria
        // como un aro suelto.
        if (MaterialCache* cache = SceneService::Materials())
            for (const GameObject* o : {static_cast<const GameObject*>(m_earth), static_cast<const GameObject*>(m_atmosphere)})
                if (o && !o->materialPath.empty())
                    if (Material* m = cache->GetMutable(o->materialPath)) m->userParams[1].x = m_earthBack;

        // Un golpe de escala al usar una interaccion: respuesta del clic, no
        // progreso. Decae solo.
        m_earthPulse = std::max(0.0f, m_earthPulse - dt * 5.0f);
        m_sunPulse = std::max(0.0f, m_sunPulse - dt * 5.0f);
        if (m_earth) SetUniformScale(*m_earth, m_earthBaseScale * (1.0f + 0.04f * m_earthPulse));
        if (m_sun) SetUniformScale(*m_sun, m_sunBaseScale * (1.0f + 0.03f * m_sunPulse) * (Cine().active ? Cine().f.k.sunScale : 1.0f));
    }

    static void SetUniformScale(GameObject& o, float s) { o.transform.scale[0] = o.transform.scale[1] = o.transform.scale[2] = s; }

    // LAS BARRAS, EL NEGRO Y LA DEDICATORIA de la cinematica. Todo en pixeles internos (960x540): el IMAX son dos barras negras a los
    // lados (enteras: una barra de medio pixel seria un borde gris), y el fundido es un rectangulo negro encima de todo.
    static void DrawCineOverlay() {
        const CineFrame& f = Cine().f;
        const float W = UiService::Width(), H = UiService::Height();
        if (W <= 0.0f || H <= 0.0f) return;
        UiDrawList& ui = UiService::Frame();
        const float visible = std::min(W, f.ratio * H);
        const float bar = std::floor((W - visible) * 0.5f);
        const UiColor black = UiColor::Bytes(0, 0, 0, 255);
        // El destello de los cortes donde algo nace: luz calida sobre la imagen (las barras van encima y no se borran).
        if (f.flash > 0.001f) ui.AddRect(UiRect{0.0f, 0.0f, W, H}, UiColor::Bytes(255, 244, 214, static_cast<int>(235.0f * std::min(1.0f, f.flash))));
        if (bar > 0.0f) {
            ui.AddRect(UiRect{0.0f, 0.0f, bar, H}, black);
            ui.AddRect(UiRect{W - bar, 0.0f, bar, H}, black);
            // El filo: mientras las barras se deslizan (y un momento despues) una linea tenue en su borde interior. Contra el negro del
            // espacio una barra negra no se ve, y el cambio de formato es justo lo que el corte tiene que mostrar.
            if (f.edge > 0.01f) {
                const UiColor edge = UiColor::Bytes(150, 168, 205, static_cast<int>(150.0f * f.edge));
                ui.AddRect(UiRect{bar, 0.0f, 1.0f, H}, edge);
                ui.AddRect(UiRect{W - bar - 1.0f, 0.0f, 1.0f, H}, edge);
            }
        }
        // El texto va centrado en lo que SE VE (entre las barras), no en la imagen entera.
        const float shownX = bar, shownW = W - 2.0f * bar;
        if (f.text > 0.01f && !Cine().text.empty()) {
            const UiRect r{shownX, std::floor(H * 0.80f), shownW, 20.0f};
            ui.AddTextCentered(r, Cine().text, UiColor::Bytes(255, 238, 216, static_cast<int>(255.0f * f.text)), 2);
        }
        // Subtitulos (EVERGRAVE_CINE_SUBS): una linea o dos, abajo, mas chicos que la dedicatoria.
        for (const CineSub& s : Cine().subs) {
            if (f.t < s.t0 || f.t > s.t1) continue;
            const float a = CineSubAlpha(s, f.t) * (1.0f - f.fade);
            if (a <= 0.01f) continue;
            float y = std::floor(H * 0.90f) - 5.0f;
            std::size_t from = 0;
            while (from <= s.text.size()) {
                const std::size_t nl = s.text.find('\n', from);
                const std::string line = s.text.substr(from, nl == std::string::npos ? std::string::npos : nl - from);
                ui.AddTextCentered(UiRect{shownX, y, shownW, 10.0f}, line, UiColor::Bytes(235, 232, 228, static_cast<int>(235.0f * a)), 1);
                y += 11.0f;
                if (nl == std::string::npos) break;
                from = nl + 1;
            }
        }
        if (f.fade > 0.001f) ui.AddRect(UiRect{0.0f, 0.0f, W, H}, UiColor::Bytes(0, 0, 0, static_cast<int>(255.0f * std::min(1.0f, f.fade))));
    }

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
        if (!ScreenPointToRay(in.CursorX(), in.CursorY(), w, h, viewProj, ray)) return false;

        // TODOS los aciertos, del mas cercano al mas lejano, y se queda el primero que sea un cuerpo. Con solo el mas
        // cercano, cada satelite, estacion, anillo o nave que se ha ido construyendo alrededor de la Tierra se
        // INTERPONIA: el rayo daba en el adorno, no era la Tierra, y el clic no hacia nada. Cuantas mas tecnologias,
        // mas adornos delante - de ahi que fallara justo al avanzar. Los adornos no son interactuables, asi que no
        // tienen por que comerse el clic.
        std::vector<RaycastHit> hits;
        SceneService::RaycastAll(ray, hits);
        for (const RaycastHit& hit : hits) {
            if (!hit.object) continue;
            const std::string path = PathOf(*hit.object);
            if (StartsWith(path, kEarthPath)) { out = InteractionTarget::Earth; return true; }
            // La Luna se recolecta como la Tierra: es otro mundo que se estudia.
            if (StartsWith(path, kMoonPath)) { out = InteractionTarget::Earth; return true; }
            if (StartsWith(path, kSunPath)) { out = InteractionTarget::Sun; return true; }
        }
        return false;
    }

    void HandlePointer() {
        GameState& game = Session();
        InputActions& in = InputService::Actions();

        // En el mapa un clic elige cuerpo (lo maneja CameraRig), y mientras se arrastra no se apunta. Con el
        // menu principal abierto el mundo no se toca.
        if (Ui().menu || Ui().mapView || Ui().camDragging || Ui().cine) return; // (cine: filmando o EVERGRAVE_PHOTO: sin etiquetas junto al cursor)
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
    GameObject* m_atmosphere = nullptr; // opcional: la capa de aire, para apagarla con la Tierra
    float m_earthBaseScale = 1.0f, m_sunBaseScale = 1.0f;
    float m_moonBaseScale = 0.0f; // la cinematica: el tamano de la Luna en la escena (se lee la primera vez)
    float m_earthPulse = 0.0f, m_sunPulse = 0.0f;
    float m_orbit = 0.0f, m_month = 0.0f;
    float m_earthBack = 0.0f; // 0 = la Tierra normal, 1 = de fondo (vista de la Luna)
    float m_sunFar = 1.0f; // 0 = el Sol en su sitio, 1 = lejos (se empieza en la Tierra)
};

STRACE_REGISTER_SCRIPT(SpaceWorld)
