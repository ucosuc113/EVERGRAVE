// GameRoot: el reloj de la partida. Lo primero que corre cada frame.
//
// Hace tres cosas y ninguna es de interfaz ni de mundo:
//  - al entrar en Play, partida nueva (Core limpio, controles enlazados);
//  - cada frame, avanza la produccion pasiva con el delta del juego;
//  - la tecla global de recolectar, y el GUARDADO AUTOMATICO (no hay guardar ni cargar a mano).
//
// EL ORDEN IMPORTA. Los OnUpdate corren en el orden de la Jerarquia, y este
// objeto va PRIMERO en la escena: aqui se apaga `pointerOverUi` y despues el HUD
// y el panel lo vuelven a encender si el cursor esta sobre ellos, y por ultimo
// el mundo lo lee. Si GameRoot corriera despues, el mundo veria el valor del
// frame anterior y un clic en un boton tambien recolectaria en el generador.

#include "../Services/Cine.h"
#include "../Services/Controls.h"
#include "../Services/Music.h"
#include "../Services/Session.h"
#include "../Services/UseFeedback.h"
#include "../Core/SaveData.h"

#include "Engine/Diagnostics/Log.h"
#include "Engine/Components/Rendering/SubViewport.h"
#include "Engine/Input/Cursor.h"
#include "Engine/Scene/ComponentQuery.h"
#include "Engine/Scene/SceneQuery.h"
#include "Engine/Scene/SceneService.h"
#include "Engine/Text.h"
#include "Engine/UI/UiService.h"
#include "Engine/Scripting/IScript.h"
#include "Engine/Scripting/ScriptRegistry.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>

using namespace strace;
using namespace evergrave;
namespace fs = std::filesystem;

namespace {

// Donde se guarda. Fuera del proyecto y de Build/: la partida de quien juega no
// es un asset, y un Build no tiene carpeta propia donde escribir. Solo Windows
// por ahora; en las otras plataformas devuelve vacio y guardar lo dice.
fs::path SavePath() {
#ifdef _WIN32
    char* buf = nullptr;
    size_t len = 0;
    if (_dupenv_s(&buf, &len, "LOCALAPPDATA") != 0 || !buf) return {};
    fs::path p = fs::path(buf) / "EVERGRAVE" / "save.txt";
    std::free(buf);
    return p;
#else
    return {};
#endif
}

// Cada cuantos segundos de juego se guarda solo. Corto a proposito: lo que se pierde en un cierre brusco es, como
// mucho, esto (y al cerrar se guarda otra vez, ver OnDestroy).
constexpr float kAutosaveSeconds = 30.0f;

// Guarda la partida. Solo AVISA si falla: el exito lo muestra el HUD con un "SAVED" discreto (`saveFlash`), porque
// un aviso grande cada 30 s es lo que cansa. Devuelve si salio.
bool SaveGame() {
    const fs::path path = SavePath();
    if (path.empty()) return false; // fuera de Windows no hay donde (por ahora): sin guardado, sin aviso cada 30 s
    std::error_code ec;
    fs::create_directories(path.parent_path(), ec);
    // A un temporal y despues renombrar: un corte a mitad de escritura no deja
    // un guardado a medias donde estaba el bueno.
    const fs::path tmp = path.wstring() + L".tmp";
    {
        std::ofstream f{tmp, std::ios::binary};
        f << SerializeSave(Session());
        if (!f) {
            Toast("Could not write the save file");
            Log::Error(L"EVERGRAVE: no se pudo escribir " + tmp.wstring());
            return false;
        }
    }
    fs::rename(tmp, path, ec);
    if (ec) {
        Toast("Could not write the save file");
        Log::Error(L"EVERGRAVE: no se pudo renombrar el guardado: " + Utf8ToWide(ec.message()));
        return false;
    }
    Ui().saveFlash = 2.0f;
    Log::Info(L"EVERGRAVE: partida guardada en " + path.wstring());
    return true;
}

// Empezar una partida nueva con un guardado puesto: el autoguardado lo va a pisar a los 30 s, sin que nadie
// lo decida, asi que la anterior se aparta (una sola copia: save.previous.txt).
void BackUpSave() {
    const fs::path path = SavePath();
    std::error_code ec;
    if (path.empty() || !fs::exists(path, ec)) return;
    fs::copy_file(path, path.parent_path() / "save.previous.txt", fs::copy_options::overwrite_existing, ec);
    if (ec) Log::Warn(L"EVERGRAVE: no se pudo apartar el guardado anterior: " + Utf8ToWide(ec.message()));
}

bool LoadGame() {
    const fs::path path = SavePath();
    std::ifstream f{path, std::ios::binary};
    if (path.empty() || !f) {
        Toast("No saved game found");
        return false;
    }
    std::stringstream ss;
    ss << f.rdbuf();
    const LoadReport rep = DeserializeSave(Session(), ss.str());
    switch (rep.result) {
        case LoadResult::Ok:
            Ui().selected.clear();
            if (rep.droppedTechs) Toast("Game loaded (some technologies were dropped)");
            Log::Info(L"EVERGRAVE: partida cargada de " + path.wstring());
            break;
        case LoadResult::WrongVersion: Toast("Save is from another version"); break;
        case LoadResult::BadFormat: Toast("Save file is damaged"); break;
    }
    return rep.result == LoadResult::Ok;
}

bool HasSavedGame() {
    const fs::path path = SavePath();
    std::error_code ec;
    return !path.empty() && fs::exists(path, ec);
}

// El menu principal NO se abre en una corrida de prueba (EVERGRAVE_CHECK=1), ni cuando se pide una partida de
// prueba (EVERGRAVE_SAVE) ni con EVERGRAVE_SKIP_MENU=1: las pruebas y las capturas del juego empiezan jugando.
bool MenuWanted() {
#ifdef _WIN32
    for (const char* name : {"EVERGRAVE_CHECK", "EVERGRAVE_SAVE", "EVERGRAVE_SKIP_MENU"}) {
        char* buf = nullptr;
        size_t len = 0;
        const bool set = _dupenv_s(&buf, &len, name) == 0 && buf && buf[0] != '\0' && buf[0] != '0';
        std::free(buf);
        if (set) return false;
    }
#endif
    return true;
}

// Para pruebas: con EVERGRAVE_SAVE=<ruta> la partida arranca cargada de ese archivo (el mismo
// formato que F5), en vez de vacia. Es como se mira el final del juego sin jugar treinta horas:
// un guardado con todo investigado y mil unidades de cada tipo. Sin la variable no hace nada.
// Con EVERGRAVE_CAMYAW / EVERGRAVE_CAMPITCH=<grados> la camara arranca con ese angulo (para mirar un lado concreto), y con
// EVERGRAVE_VIEW=moon|sun|map en esa vista.
void LoadTestSave() {
#ifdef _WIN32
    char* buf = nullptr;
    size_t len = 0;
    if (_dupenv_s(&buf, &len, "EVERGRAVE_SAVE") != 0 || !buf) return;
    const fs::path path = buf;
    std::free(buf);
    std::ifstream f{path, std::ios::binary};
    if (!f) {
        Log::Warn(L"EVERGRAVE: EVERGRAVE_SAVE apunta a un archivo que no se puede abrir: " + path.wstring());
        return;
    }
    std::stringstream ss;
    ss << f.rdbuf();
    const LoadReport rep = DeserializeSave(Session(), ss.str());
    Log::Info(L"EVERGRAVE: partida de prueba cargada de " + path.wstring() + (rep.result == LoadResult::Ok ? L"" : L" (FALLO)"));
    // La cinematica filma estructuras sueltas (los anillos de Dyson) sin su cadena de requisitos: el cargado normal las descartaria
    // (ver `TechTree::RestoreResearched`), asi que aqui se fuerzan las `tech=` del archivo, tal cual estan.
    if (rep.result == LoadResult::Ok && std::getenv("EVERGRAVE_CINE")) {
        std::vector<TechId> ids;
        std::istringstream lines(ss.str());
        std::string line;
        while (std::getline(lines, line)) {
            while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();
            if (line.rfind("tech=", 0) == 0) ids.push_back(line.substr(5));
        }
        Session().MutableTree().ForceResearched(ids);
        Session().Recompute();
        Log::Info(L"EVERGRAVE: cinematica: " + std::to_wstring(ids.size()) + L" tecnologias forzadas");
    }
#endif
}

} // namespace

class GameRoot : public IScript {
public:
    void OnStart() override {
        ResetSession();
        ResetUi();
        LoadTestSave();
#ifdef _WIN32
        {
            // EVERGRAVE_VIEW=moon|sun|map arranca en esa vista (si la partida la tiene abierta): una corrida automatica no
            // puede apretar 2, 3 o M, y sin esto la vista de la Luna no se podia capturar. Antes que el angulo: elegir la
            // vista pone el de esa vista, y EVERGRAVE_CAMYAW lo pisa despues si tambien viene.
            size_t n = 0;
            char* view = nullptr;
            if (_dupenv_s(&view, &n, "EVERGRAVE_VIEW") == 0 && view) {
                const std::string v = view;
                std::free(view);
                if (v == "moon") GoToBody(BodyId::Moon);
                else if (v == "sun") GoToBody(BodyId::Sun);
                else if (v == "map") GoToMap();
            }
            // EVERGRAVE_TREE=1 abre el arbol de tecnologias: es como se captura (una corrida automatica no aprieta T).
            char* tree = nullptr;
            if (_dupenv_s(&tree, &n, "EVERGRAVE_TREE") == 0 && tree) {
                if (tree[0] != '\0' && tree[0] != '0') Ui().treeOpen = true;
                if (tree[0] == '2') Ui().treeShowDone = true; // y con lo investigado desplegado
                std::free(tree);
            }
            char* yaw = nullptr;
            if (_dupenv_s(&yaw, &n, "EVERGRAVE_CAMYAW") == 0 && yaw) {
                Ui().camYaw = static_cast<float>(std::atof(yaw));
                std::free(yaw);
            }
            char* pitch = nullptr;
            if (_dupenv_s(&pitch, &n, "EVERGRAVE_CAMPITCH") == 0 && pitch) {
                Ui().camPitch = static_cast<float>(std::atof(pitch));
                std::free(pitch);
            }
            // EVERGRAVE_CAMZOOM=<0.55..1.9> fija el zoom de la camara (menos = mas cerca), y EVERGRAVE_PHOTO=1 esconde el HUD y la tienda: una
            // FOTO del juego de verdad (la vista de la Luna, la Tierra...), sin interfaz y sin pasar por la cinematica.
            char* zoom = nullptr;
            if (_dupenv_s(&zoom, &n, "EVERGRAVE_CAMZOOM") == 0 && zoom) {
                Ui().camZoom = std::clamp(static_cast<float>(std::atof(zoom)), 0.55f, 1.9f);
                std::free(zoom);
            }
            char* photo = nullptr;
            if (_dupenv_s(&photo, &n, "EVERGRAVE_PHOTO") == 0 && photo) {
                if (photo[0] != '\0' && photo[0] != '0') {
                    Ui().cine = true; // el mismo interruptor que oculta HUD y paneles al filmar: aqui sin guion ni camara de cinematica
                    Ui().shopOpen = false;
                }
                std::free(photo);
            }
        }
#endif
        BindControls();
        Ui().hasSave = HasSavedGame();
        Ui().menu = MenuWanted();
        // Solo una corrida NORMAL guarda: las pruebas, las capturas (EVERGRAVE_SAVE, EVERGRAVE_SKIP_MENU) y la
        // cinematica juegan con partidas armadas a mano y pisarian la de verdad.
        m_autosave = Ui().menu;
        // EVERGRAVE_AUTOSAVE=<segundos>: fuerza el guardado automatico en una corrida automatica, con ese intervalo, para
        // probarlo. Escribe en %LOCALAPPDATA%\EVERGRAVE\save.txt: quien lo use redirige LOCALAPPDATA a una carpeta suelta.
#ifdef _WIN32
        {
            char* a = nullptr;
            size_t n = 0;
            if (_dupenv_s(&a, &n, "EVERGRAVE_AUTOSAVE") == 0 && a) {
                const float secs = static_cast<float>(std::atof(a));
                std::free(a);
                if (secs > 0.0f) {
                    m_autosave = true;
                    m_interval = secs;
                }
            }
        }
#endif
        // LA CINEMATICA (EVERGRAVE_CINE): sin menu, sin interfaz, y a 540 filas internas en vez de 360 (x1,5: un pixel de juego
        // son 2x2 de un video de 1080p, exacto, pero con mas detalle que el juego). Solo cambia la copia de Play de la escena.
        CineInit();
        if (Cine().active) {
            m_autosave = false;
            Ui().menu = false;
            Ui().cine = true;
            Ui().shopOpen = false;
            if (GameObject* px = FindByPath(SceneService::MutableRoots(), "PixelViewport"))
                if (SubViewport* sv = GetComponent<SubViewport>(*px)) {
                    sv->baseHeight = 540;
                    sv->baseWidth = 960;
                }
            Log::Info(L"EVERGRAVE: CINEMATICA desde el segundo " + std::to_wstring(Cine().t) + L" (" + std::to_wstring(CineLength()) + L" s)");
        }
        // LA MUSICA (Services/Music.h): suena desde el primer frame - la pantalla del titulo ya la tiene - y no se corta
        // nunca. Con menu empieza solo con la melodia y los acompanantes entran al darle a jugar; sin menu (una partida
        // directa) ya con la cancion entera. Filmando no suena: la cinematica es solo imagen.
        if (!Cine().active) MusicStart(!Ui().menu);
        // El titulo solo si el menu se va a abrir de verdad.
        Ui().titleActive = Ui().menu;
        Ui().titleT = 0.0f;
        Cursor::SetMode(CursorMode::Visible);
        Log::Info(L"EVERGRAVE: partida nueva (" + std::to_wstring(Session().tree().All().size()) + L" tecnologias)");
    }

    void OnUpdate(float dt) override {
        MusicUpdate(dt);
        if (TouchMode()) TouchUpdate();
        UiState& ui = Ui();
        if (ui.titleActive) {
            ui.titleT += dt;
            if (ui.titleT >= kTitleSeconds) ui.titleActive = false;
        }
        ui.pointerOverUiLast = ui.pointerOverUi;
        ui.pointerOverUi = false;
        ui.worldClick = false;
        if (ui.toastTime > 0.0f) ui.toastTime -= dt;
        if (ui.saveFlash > 0.0f) ui.saveFlash -= dt;

        // Filmando: el reloj es el de la toma y la partida no corre (ni teclas, ni guardados).
        if (Cine().active) {
            CineAdvance(dt);
            ui.view = Cine().f.view;
            ui.mapView = false;
            // LO QUE SE CONSTRUYE (Cine.h, `CineTechSteps`): cuando cambia la etapa se rehace entero el conjunto de investigadas y la cuenta de
            // satelites. Entero, y no "agregar lo nuevo": asi saltar a cualquier segundo (EVERGRAVE_CINE=30) da el mismo mundo que llegar
            // ahi desde el principio. La flota y los anillos lo leen en este mismo frame.
            const int stage = CineTechStage(Cine().t);
            if (stage != m_cineStage) {
                m_cineStage = stage;
                std::vector<TechId> ids;
                int satellites = 0;
                for (int k = 0; k < stage; ++k) {
                    const CineTechStep& st = CineTechSteps()[static_cast<std::size_t>(k)];
                    if (st.tech) ids.push_back(st.tech);
                    satellites = st.satellites;
                }
                GameState& g = Session();
                g.MutableTree().ClearResearched();
                g.MutableTree().ForceResearched(ids);
                g.Recompute();
                g.RestoreInstallations({{"satellite", satellites}});
            }
            return;
        }

        // El menu pide y esto responde: guardar y cargar son de aqui. Mientras esta abierto la partida espera.
        if (ui.menu) {
            if (ui.menuRequest == 1) {
                if (LoadGame()) ui.menu = false;
            } else if (ui.menuRequest == 2) {
                BackUpSave();
                ResetSession();
                ui.menu = false;
            }
            ui.menuRequest = 0;
            if (ui.menu) return;
            // Los acompanantes de la cancion entran (la melodia sigue igual): Services/Music.h.
            MusicToFull();
            // Al cerrarse el menu la camara vuela del encuadre del menu al del juego (CameraRig).
            ResetCameraFor(BodyId::Earth);
        }

        GameState& game = Session();
        game.Tick(dt);

        InputActions& in = InputService::Actions();
        if (in.Pressed("ev_gather")) {
            const UiRect vis = UiService::VisibleRect();
            UseWithFeedback(kGatherId, vis.x + vis.width * 0.5f, vis.y + vis.height * 0.5f);
        }

        // El guardado automatico. Nunca con el menu abierto (la sesion ahi es una partida nueva vacia: guardarla
        // pisaria la buena antes de que se elija "Continuar").
        m_sinceSave += dt;
        if (m_autosave && m_sinceSave >= m_interval) {
            m_sinceSave = 0.0f;
            SaveGame();
        }
    }

    // Al detener el Play o cerrar el juego: lo ultimo que paso desde el ultimo guardado no se pierde.
    void OnDestroy() override {
        if (m_autosave && !Ui().menu) SaveGame();
    }

private:
    bool m_autosave = false;
    float m_sinceSave = 0.0f;
    float m_interval = kAutosaveSeconds;
    int m_cineStage = -1; // la etapa del calendario de la cinematica ya aplicada (-1 = ninguna)
};

STRACE_REGISTER_SCRIPT(GameRoot)
