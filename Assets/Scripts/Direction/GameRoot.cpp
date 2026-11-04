// GameRoot: el reloj de la partida. Lo primero que corre cada frame.
//
// Hace tres cosas y ninguna es de interfaz ni de mundo:
//  - al entrar en Play, partida nueva (Core limpio, controles enlazados);
//  - cada frame, avanza la produccion pasiva con el delta del juego;
//  - las teclas globales: recolectar, guardar y cargar.
//
// EL ORDEN IMPORTA. Los OnUpdate corren en el orden de la Jerarquia, y este
// objeto va PRIMERO en la escena: aqui se apaga `pointerOverUi` y despues el HUD
// y el panel lo vuelven a encender si el cursor esta sobre ellos, y por ultimo
// el mundo lo lee. Si GameRoot corriera despues, el mundo veria el valor del
// frame anterior y un clic en un boton tambien recolectaria en el generador.

#include "../Services/Controls.h"
#include "../Services/Session.h"
#include "../Services/UseFeedback.h"
#include "../Core/SaveData.h"

#include "Engine/Diagnostics/Log.h"
#include "Engine/Input/Cursor.h"
#include "Engine/Text.h"
#include "Engine/UI/UiService.h"
#include "Engine/Scripting/IScript.h"
#include "Engine/Scripting/ScriptRegistry.h"

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

void SaveGame() {
    const fs::path path = SavePath();
    if (path.empty()) {
        Toast("Saving is only available on Windows");
        return;
    }
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
            return;
        }
    }
    fs::rename(tmp, path, ec);
    if (ec) {
        Toast("Could not write the save file");
        Log::Error(L"EVERGRAVE: no se pudo renombrar el guardado: " + Utf8ToWide(ec.message()));
        return;
    }
    Toast("Game saved");
    Log::Info(L"EVERGRAVE: partida guardada en " + path.wstring());
}

void LoadGame() {
    const fs::path path = SavePath();
    std::ifstream f{path, std::ios::binary};
    if (path.empty() || !f) {
        Toast("No saved game found");
        return;
    }
    std::stringstream ss;
    ss << f.rdbuf();
    const LoadReport rep = DeserializeSave(Session(), ss.str());
    switch (rep.result) {
        case LoadResult::Ok:
            Ui().selected.clear();
            Toast(rep.droppedTechs ? "Game loaded (some technologies were dropped)" : "Game loaded");
            Log::Info(L"EVERGRAVE: partida cargada de " + path.wstring());
            break;
        case LoadResult::WrongVersion: Toast("Save is from another version"); break;
        case LoadResult::BadFormat: Toast("Save file is damaged"); break;
    }
}

// Para pruebas: con EVERGRAVE_SAVE=<ruta> la partida arranca cargada de ese archivo (el mismo
// formato que F5), en vez de vacia. Es como se mira el final del juego sin jugar treinta horas:
// un guardado con todo investigado y mil unidades de cada tipo. Sin la variable no hace nada.
// Con EVERGRAVE_CAMYAW / EVERGRAVE_CAMPITCH=<grados> la camara arranca con ese angulo (para mirar un lado concreto).
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
            char* yaw = nullptr;
            size_t n = 0;
            if (_dupenv_s(&yaw, &n, "EVERGRAVE_CAMYAW") == 0 && yaw) {
                Ui().camYaw = static_cast<float>(std::atof(yaw));
                std::free(yaw);
            }
            char* pitch = nullptr;
            if (_dupenv_s(&pitch, &n, "EVERGRAVE_CAMPITCH") == 0 && pitch) {
                Ui().camPitch = static_cast<float>(std::atof(pitch));
                std::free(pitch);
            }
        }
#endif
        BindControls();
        Cursor::SetMode(CursorMode::Visible);
        Log::Info(L"EVERGRAVE: partida nueva (" + std::to_wstring(Session().tree().All().size()) + L" tecnologias)");
    }

    void OnUpdate(float dt) override {
        UiState& ui = Ui();
        ui.pointerOverUiLast = ui.pointerOverUi;
        ui.pointerOverUi = false;
        ui.worldClick = false;
        if (ui.toastTime > 0.0f) ui.toastTime -= dt;

        GameState& game = Session();
        game.Tick(dt);

        InputActions& in = InputService::Actions();
        if (in.Pressed("ev_gather")) {
            const UiRect vis = UiService::VisibleRect();
            UseWithFeedback(kGatherId, vis.x + vis.width * 0.5f, vis.y + vis.height * 0.5f);
        }
        if (in.Pressed("ev_save")) SaveGame();
        if (in.Pressed("ev_load")) LoadGame();
    }
};

STRACE_REGISTER_SCRIPT(GameRoot)
