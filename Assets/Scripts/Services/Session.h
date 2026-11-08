#pragma once

// LA SESION: el unico GameState de la partida y el estado de la interfaz que los
// scripts comparten.
//
// Los scripts de View/ y Direction/ son objetos sueltos de la escena que no se
// conocen entre si; esto es el punto de encuentro (el `GameWorld` de AN0MALY,
// en chico). Es un singleton a proposito: un `Scripts.dll` es un solo juego, y
// pasar el estado por campos de Inspector entre cinco scripts seria cableado
// que se rompe al renombrar un objeto.
//
// Dos cosas separadas, y no por orden:
//  - `Session()`  el juego. Core/, sin motor. Lo que se guarda.
//  - `Ui()`       lo que solo importa a la pantalla (panel abierto, seleccion,
//                 aviso). NO se guarda y el juego no lo lee jamas.
// Que el estado del juego no dependa de la interfaz es lo que deja cambiar o
// reemplazar el panel sin tocar una regla.

#include "../Core/GameState.h"
#include "../Data/GameContent.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace evergrave {

inline GameState& Session() {
    static GameState s = NewGame();
    return s;
}

// Partida nueva. Se llama al entrar en Play: el `Scripts.dll` sigue cargado
// entre una prueba y la siguiente, y un singleton conserva lo de la anterior.
inline void ResetSession() { Session() = NewGame(); }

// EL ENCUADRE POR DEFECTO DE LA TIERRA. No es "mirar el planeta de frente": es una toma. El yaw (respecto a la direccion al Sol)
// deja el Sol detras y a un lado, asi el planeta se ve mitad de dia y mitad de noche con el terminador a la vista y el filo del
// aire encendido, y no como un disco parejo (con 105 quedaba una media luna fina y el planeta casi entero a oscuras); el pitch es bajo (casi de lado), que es como se filma un planeta.
constexpr float kEarthYaw = 72.0f;
constexpr float kEarthPitch = 18.0f;

// Cuanto dura el titulo entero: aparece, se queda ~3 s y se abre. TitleCard.h reparte este tiempo.
constexpr float kTitleSeconds = 4.8f;

struct UiState {
    // EL MENU PRINCIPAL (Menu/ en la escena, View/MainMenu.cpp). Mientras esta abierto el juego espera
    // detras: no corre la produccion, el HUD y los paneles no dibujan y el mundo no recibe clics. Empieza
    // en false para que una prueba que reinicia la interfaz (ResetUi) no lo vuelva a abrir; lo abre
    // GameRoot al empezar el Play.
    bool menu = false;
    // EL TITULO ANTES DEL MENU ("for my precious moon", View/TitleCard.h): una pantalla negra con una frase que dura
    // kTitleSeconds y se abre como un telon sobre el menu. La musica ya suena desde aqui. Mientras dura, el menu esta
    // ahi detras pero no responde. Lo enciende GameRoot al empezar (solo si el menu se va a abrir: una corrida de
    // prueba no lo ve) y lo apaga cuando el telon termina.
    bool titleActive = false;
    float titleT = 0.0f;
    // LA CINEMATICA (Services/Cine.h, EVERGRAVE_CINE): el juego no se juega, se filma. Oculta HUD y paneles y le quita la
    // camara a la persona. Lo pone GameRoot; nada mas lo escribe.
    bool cine = false;
    float saveFlash = 0.0f; // segundos que quedan del "SAVED" del HUD tras un guardado automatico
    bool hasSave = false;   // hay una partida guardada que continuar (GameRoot lo mira al empezar)
    int menuRequest = 0;    // lo que el menu le pide a GameRoot, que es quien sabe guardar y cargar: 0 nada, 1 continuar, 2 nueva

    bool treeOpen = false;
    // El frame en que un CLIC abrio o cerro el panel del arbol. El boton RESEARCH del HUD y el CLOSE del panel estan UNO
    // ENCIMA DEL OTRO (arriba a la derecha, casi el mismo rectangulo) y los dos leen el mismo clic: el que corre primero
    // abre o cierra, el que corre despues ve el panel en su estado NUEVO, el cursor sigue sobre su boton y el clic sigue
    // valido ese mismo frame - lo deshace. El panel parpadeaba un frame y quedaba como estaba. Con esto, un clic cambia el
    // panel UNA vez, venga del boton que venga y corra en el orden que corra. (La T no tenia el problema: no es un clic.)
    unsigned long long treeToggledFrame = ~0ull;
    TechId selected;         // vacio = nada elegido
    float scrollY = 0.0f;    // desplazamiento de la lista de tecnologias

    // LA VISTA: en que cuerpo se mira y si se esta en el mapa del sistema. Empieza en la Tierra y
    // ahi se queda hasta que se abra el mapa. Presentacion: no se guarda ni el juego lo lee.
    BodyId view = BodyId::Earth;
    bool mapView = false;
    float camYaw = kEarthYaw;     // grados respecto a la direccion al Sol: 0 = del lado del dia
    float camPitch = kEarthPitch; // grados sobre el plano del sistema
    float camZoom = 1.0f;
    bool camYawFromEarth = false; // vista de la Luna: camYaw todavia es relativo a la Tierra (CameraRig lo fija al entrar)
    float camIdle = 0.0f;       // segundos sin tocar la camara: pasado un rato, gira sola
    bool camDragging = false;   // hay un arrastre en curso sobre el mundo
    float camDragDist = 0.0f;   // cuanto se movio el puntero: poco = fue un clic, no un arrastre
    bool worldClick = false;    // este frame hubo un clic limpio (sin arrastre) sobre el mundo
    bool pointerOverUiLast = false; // el valor de pointerOverUi del frame anterior (el de este aun no se calculo)

    // La tienda. Presentacion: nada de esto se guarda. Empieza ABIERTA: es lo primero que se ve.
    bool shopOpen = true;
    int shopBuyMode = 0;       // 0 = x1, 1 = x10, 2 = x100, 3 = el maximo que alcance
    float shopScroll = 0.0f;
    float shopFlash = 0.0f;    // un destello corto cuando se compra o no alcanza
    std::string shopFlashId;

    // El panel de investigacion (View/TechPanel.cpp). Todo presentacion: nada de esto se guarda ni toca el Core.
    std::string treeBranch;       // rama filtrada ("" = todas)
    bool treeShowDone = false;    // la seccion de lo ya investigado, desplegada
    bool treeFocus = true;        // pedir que la lista muestre la tecnologia elegida (al abrir, al saltar a un requisito)
    float treeDetailScroll = 0.0f;
    float treeDetailH = 0.0f;     // alto del contenido del detalle del cuadro anterior (para acotar su desplazamiento)
    float treeListH = 0.0f;       // alto del contenido de la lista del cuadro anterior
    float treeClock = 0.0f;       // segundos con el panel abierto
    TechId treeLastClick;         // para el doble clic: investigar de un tiro
    float treeLastClickAt = -10.0f;
    bool pointerOverUi = false; // lo recalcula cada frame quien dibuja; el mundo lo lee

    // Numeros que suben y se apagan donde se hizo clic ("+8.0"). Solo
    // presentacion: lo ganado ya esta en el Core cuando se crean.
    struct Floater {
        std::string text;
        float x = 0, y = 0, age = 0;
    };
    std::vector<Floater> floaters;

    // Un aviso de una linea (guardado, error de compra...) y cuanto le queda.
    std::string toast;
    float toastTime = 0.0f;
};

inline UiState& Ui() {
    static UiState s;
    return s;
}

inline void ResetUi() { Ui() = UiState{}; }

// Cambiar de cuerpo o de vista. Devuelven si se pudo: la Tierra siempre, y lo demas solo si
// una tecnologia ya lo abrio (el Core decide, no la interfaz).
inline void ResetCameraFor(BodyId b) {
    UiState& u = Ui();
    // Tierra: 55 (el lado iluminado, el Sol a un lado); Luna: 132 desde la Tierra (con 135 quedaba detras de la Luna; con
    // 132 asoma a un lado). El de la Luna es relativo a la Tierra solo para entrar: CameraRig lo vuelve fijo en el mundo.
    // La Luna tiene la MISMA toma que la Tierra (yaw respecto al Sol): con el yaw fijo en el mundo que tenia, la fase que se veia dependia de EN QUE
    // MOMENTO del mes se entraba a la vista (a veces una raya de luz, a veces la mitad), y una toma no puede depender del azar.
    u.camYaw = b == BodyId::Sun ? 0.0f : kEarthYaw;
    u.camYawFromEarth = false;
    u.camPitch = b == BodyId::Sun ? 35.0f : kEarthPitch;
    u.camZoom = 1.0f;
    u.camIdle = 0.0f;
}
inline bool GoToBody(BodyId b) {
    if (!Session().IsBodyUnlocked(b)) return false;
    UiState& u = Ui();
    if (u.view != b || u.mapView) ResetCameraFor(b);
    u.view = b;
    u.mapView = false;
    return true;
}
inline bool GoToMap() {
    if (!Session().IsMapUnlocked()) return false;
    Ui().mapView = true;
    return true;
}

inline void Toast(const std::string& text, float seconds = 2.5f) {
    Ui().toast = text;
    Ui().toastTime = seconds;
}

} // namespace evergrave
