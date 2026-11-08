// MainMenu: lo que el menu principal tiene de CODIGO, que es poco a proposito.
//
// LA FORMA VIVE EN LA JERARQUIA. `Menu/Canvas` es la interfaz entera como datos de la escena: el titulo y el
// AN0MALESS son UiImage (con su brillo y su opacidad como campos del Inspector), cada opcion es un UiButton con
// su UiLabel y un `Marcador` (un UiPanel de 4x4), y los tres satelites son objetos 3D bajo la Tierra
// (`Space/OrbitPivot/EarthAnchor/Satelites`). Para mover una opcion diez pixeles o cambiar un texto se toca la
// escena y no se recompila nada.
//
// Aqui queda solo lo que una escena no sabe hacer:
//  - CUAL opcion esta elegida (el raton la elige al pasar por encima, las flechas la mueven) y como se ve
//    (blanca la elegida, gris el resto, el marcador rojo a su lado);
//  - QUE PASA al aceptar: continuar y nueva partida se piden a GameRoot (es quien sabe guardar y cargar) y salir
//    cierra el juego;
//  - el MOVIMIENTO: los tres satelites girando alrededor de la Tierra.
//
// El menu es un ESTADO de la partida (`Ui().menu`), no otra escena: el motor todavia no cambia de escena desde un
// script, y asi la Tierra del menu es la misma del juego - al empezar la camara baja de un encuadre al otro.

#include "../Services/Controls.h"
#include "../Services/Session.h"
#include "TitleCard.h"

#include "Engine/Components/UI/UiComponents.h"
#include "Engine/Diagnostics/Log.h"
#include "Engine/Scene/ComponentQuery.h"
#include "Engine/Scene/GameObject.h"
#include "Engine/Scene/SceneQuery.h"
#include "Engine/Scene/SceneService.h"
#include "Engine/Scene/SceneTransform.h"
#include "Engine/Scripting/IScript.h"
#include "Engine/Scripting/ScriptRegistry.h"
#include "Engine/UI/SceneUi.h"

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
// Scripts.dll se enlaza solo contra EngineCore.lib y lo basico del CRT: PostQuitMessage vive en user32.
#if defined(_MSC_VER)
#pragma comment(lib, "user32.lib")
#endif
#endif

#include <DirectXMath.h>

#include <algorithm>
#include <cmath>
#include <string>

using namespace strace;
using namespace evergrave;
using DirectX::XMFLOAT3;

namespace {

constexpr const char* kCanvasPath = "Menu/Canvas";
constexpr const char* kCameraPath = "Space/OrbitPivot/Camara";
constexpr const char* kMoonPath = "Space/OrbitPivot/EarthAnchor/Moon";
constexpr const char* kGroupPath = "Space/OrbitPivot/EarthAnchor/Satelites";
constexpr float kRad = 3.14159265f / 180.0f;

enum Option { kContinue, kNew, kSettings, kQuit, kOptionCount };
constexpr const char* kOptionNames[kOptionCount] = {"Continuar", "Nueva", "Ajustes", "Salir"};

// Un satelite: radio de la orbita en unidades del mundo (la Tierra mide 1.8 de radio), grados por segundo (el
// signo es el sentido), inclinacion y fase. La camara del menu mira la Tierra desde casi el plano de su ecuador
// y solo ve lo que queda ARRIBA de su centro: una orbita plana quedaria bajo el cuadro, asi que la sonda y la
// estacion van por orbitas muy inclinadas (casi polares) y suben por delante del planeta. La Luna, mas suave.
struct SatOrbit {
    const char* name;
    float radius;
    float degPerSec;
    float inclination;
    float phase;
    float spin;  // grados por segundo del propio satelite
};
constexpr SatOrbit kOrbits[3] = {
    {"Sonda", 2.6f, 36.0f, 56.0f, 25.0f, 40.0f},
    {"Estacion", 3.2f, -19.0f, -38.0f, 140.0f, 12.0f},
    {"Luna", 4.4f, 9.0f, 22.0f, 250.0f, 3.0f},
};

bool RunningInEditor() {
#if defined(_WIN32)
    static int cached = -1;
    if (cached < 0) {
        wchar_t path[MAX_PATH] = {};
        GetModuleFileNameW(nullptr, path, MAX_PATH);
        std::wstring name(path);
        const size_t slash = name.find_last_of(L"\\/");
        if (slash != std::wstring::npos) name = name.substr(slash + 1);
        for (wchar_t& c : name) c = static_cast<wchar_t>(towlower(c));
        cached = name == L"editor.exe" ? 1 : 0;
    }
    return cached == 1;
#else
    return false;
#endif
}

void QuitGame() {
    if (RunningInEditor()) {
        Log::Info(L"EVERGRAVE: SALIR en el Editor no cierra nada (deten el Play). En un Build cierra el juego.");
        return;
    }
#if defined(_WIN32)
    Log::Info(L"EVERGRAVE: salir del juego.");
    PostQuitMessage(0);
#else
    Log::Warn(L"EVERGRAVE: esta plataforma todavia no deja que el juego se cierre solo.");
#endif
}

} // namespace

class MainMenu : public IScript {
public:
    void OnStart() override {
        GameObject* canvas = FindByPath(SceneService::MutableRoots(), kCanvasPath);
        m_canvas = canvas;
        for (int i = 0; i < kOptionCount; ++i) {
            const std::string path = std::string(kCanvasPath) + "/" + kOptionNames[i];
            Item& it = m_items[i];
            it.node = FindByPath(SceneService::MutableRoots(), path);
            if (!it.node) continue;
            it.button = GetComponent<UiButton>(*it.node);
            it.label = GetComponent<UiLabel>(*it.node);
            it.marker = FindByPath(SceneService::MutableRoots(), path + "/Marcador");
        }
        m_camera = FindByPath(SceneService::MutableRoots(), kCameraPath);
        m_moon = FindByPath(SceneService::MutableRoots(), kMoonPath);
        m_group = FindByPath(SceneService::MutableRoots(), kGroupPath);
        for (int i = 0; i < 3; ++i) m_sats[i] = m_group ? FindByPath(SceneService::MutableRoots(), std::string(kGroupPath) + "/" + kOrbits[i].name) : nullptr;
        if (!m_canvas || !m_group) Log::Warn(L"EVERGRAVE: la escena no tiene Menu/Canvas o Space/.../Satelites");
        m_sel = Ui().hasSave ? kContinue : kNew;
        m_shown = false;
        m_first = true;
    }

    void OnUpdate(float dt) override {
        UiState& state = Ui();
        // Mostrar u ocultar: el Canvas, los satelites, y la Luna de verdad (que a su distancia entraria al cuadro
        // como un cuarto satelite).
        // El PRIMER frame siempre reconcilia: la escena trae el menu encendido, y en una corrida de prueba (con partida
        // cargada) el estado nace cerrado - sin esto el titulo se quedaba pintado encima del juego, porque `menu`
        // y `m_shown` valian lo mismo (false) y nadie apagaba el Canvas.
        if (m_first || state.menu != m_shown) {
            m_first = false;
            m_shown = state.menu;
            if (m_canvas) m_canvas->active = state.menu;
            if (m_group) m_group->active = state.menu;
            if (m_moon) m_moon->active = !state.menu;
            if (state.menu) m_sel = state.hasSave ? kContinue : kNew;
        }
        if (!state.menu) return;

        m_time += dt;
        if (!state.titleActive) Select(); // detras del titulo el menu se ve pero no responde
        Style();
        Orbit(dt);
        DrawTitleCard(); // al final del frame: encima de todo lo del menu
    }

private:
    struct Item {
        GameObject* node = nullptr;
        GameObject* marker = nullptr;
        UiButton* button = nullptr;
        UiLabel* label = nullptr;
    };

    bool Enabled(int i) const {
        if (i == kContinue) return Ui().hasSave;
        if (i == kSettings) return false; // todavia no hay opciones que cambiar
        return true;
    }

    int Step(int from, int dir) const {
        int i = from;
        for (int n = 0; n < kOptionCount; ++n) {
            i = (i + dir + kOptionCount) % kOptionCount;
            if (Enabled(i)) return i;
        }
        return from;
    }

    void Select() {
        InputActions& in = InputService::Actions();
        // El boton de Continuar se apaga solo si no hay partida; los otros ya vienen de la escena.
        if (m_items[kContinue].button) m_items[kContinue].button->interactable = Ui().hasSave;

        for (int i = 0; i < kOptionCount; ++i)
            if (m_items[i].button && m_items[i].button->hovered && Enabled(i)) m_sel = i;
        if (!Enabled(m_sel)) m_sel = Step(m_sel, 1);
        if (in.Pressed("ev_menu_next")) m_sel = Step(m_sel, 1);
        if (in.Pressed("ev_menu_prev")) m_sel = Step(m_sel, -1);

        bool accept = in.Pressed("ev_menu_ok");
        for (int i = 0; i < kOptionCount; ++i) {
            if (m_items[i].node && Enabled(i) && SceneUi::Clicked(*m_items[i].node)) {
                m_sel = i;
                accept = true;
            }
        }
        if (!accept || !Enabled(m_sel)) return;
        switch (m_sel) {
            case kContinue: Ui().menuRequest = 1; break;
            case kNew: Ui().menuRequest = 2; break;
            case kQuit: QuitGame(); break;
            default: break;
        }
    }

    // Blanca la elegida, gris el resto y mas oscura la que no se puede; el marcador rojo solo junto a la elegida.
    void Style() {
        for (int i = 0; i < kOptionCount; ++i) {
            const Item& it = m_items[i];
            if (it.label)
                it.label->color = i == m_sel ? UiColor::Bytes(235, 235, 235) : Enabled(i) ? UiColor::Bytes(138, 138, 152) : UiColor::Bytes(70, 70, 82);
            if (it.marker) it.marker->active = i == m_sel;
        }
    }

    // Tres satelites en orbita alrededor del centro de la Tierra (su padre). La orbita es un circulo en el plano que
    // forman la derecha de la pantalla y la profundidad, inclinado alrededor de la derecha: asi, desde la camara
    // del menu, es una elipse inclinada y no una raya.
    void Orbit(float dt) {
        if (!m_camera || !m_group) return;
        const GameObject* anchor = m_group->parent;
        if (!anchor) return;
        // Hacia donde esta la camara, en el plano horizontal, visto desde la Tierra.
        const XMFLOAT3 earth = ComputeWorldPosition(*anchor);
        float fx = m_camera->transform.position[0] - earth.x, fz = m_camera->transform.position[2] - earth.z;
        const float len = std::max(0.0001f, std::sqrt(fx * fx + fz * fz));
        fx /= len;
        fz /= len;
        const float rx = fz, rz = -fx; // la derecha de la pantalla

        for (int i = 0; i < 3; ++i) {
            GameObject* s = m_sats[i];
            if (!s) continue;
            const SatOrbit& o = kOrbits[i];
            const float a = (o.phase + o.degPerSec * m_time) * kRad, inc = o.inclination * kRad;
            const float c = std::cos(a), sn = std::sin(a), ci = std::cos(inc), si = std::sin(inc);
            // r * cos(a) + (f * cos(inc) + arriba * sin(inc)) * sin(a)
            s->transform.position[0] = o.radius * (c * rx + sn * ci * fx);
            s->transform.position[1] = o.radius * (sn * si);
            s->transform.position[2] = o.radius * (c * rz + sn * ci * fz);
            s->transform.rotation[1] = std::fmod(o.spin * m_time, 360.0f);
        }
        (void)dt;
    }

    Item m_items[kOptionCount];
    GameObject* m_canvas = nullptr;
    GameObject* m_camera = nullptr;
    GameObject* m_moon = nullptr;
    GameObject* m_group = nullptr;
    GameObject* m_sats[3] = {nullptr, nullptr, nullptr};
    int m_sel = kNew;
    float m_time = 0.0f;
    bool m_shown = false;
    bool m_first = true;
};

STRACE_REGISTER_SCRIPT(MainMenu)
