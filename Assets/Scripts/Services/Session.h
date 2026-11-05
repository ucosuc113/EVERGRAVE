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

struct UiState {
    // EL MENU PRINCIPAL (Menu/ en la escena, View/MainMenu.cpp). Mientras esta abierto el juego espera
    // detras: no corre la produccion, el HUD y los paneles no dibujan y el mundo no recibe clics. Empieza
    // en false para que una prueba que reinicia la interfaz (ResetUi) no lo vuelva a abrir; lo abre
    // GameRoot al empezar el Play.
    bool menu = false;
    bool hasSave = false;   // hay una partida guardada que continuar (GameRoot lo mira al empezar)
    int menuRequest = 0;    // lo que el menu le pide a GameRoot, que es quien sabe guardar y cargar: 0 nada, 1 continuar, 2 nueva

    bool treeOpen = false;
    TechId selected;         // vacio = nada elegido
    float scrollY = 0.0f;    // desplazamiento del arbol

    // LA VISTA: en que cuerpo se mira y si se esta en el mapa del sistema. Empieza en la Tierra y
    // ahi se queda hasta que se abra el mapa. Presentacion: no se guarda ni el juego lo lee.
    BodyId view = BodyId::Earth;
    bool mapView = false;
    float camYaw = 55.0f;       // grados respecto a la direccion al Sol: 0 = del lado del dia
    float camPitch = 35.0f;     // grados sobre el plano del sistema
    float camZoom = 1.0f;
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

    // El mapa de tecnologias. Todo presentacion: nada de esto se guarda ni toca el Core.
    bool treeNeedsCenter = true;  // al abrir, el mapa se centra en la ultima tecnologia que se ve
    bool treeDrag = false;        // hay un arrastre en curso dentro del lienzo
    float treeDragDist = 0.0f;    // cuanto se movio el puntero: poco = fue un clic, no un arrastre
    float treeClock = 0.0f;       // segundos con el mapa abierto: el pulso de lo que se puede comprar
    bool treeSeeded = false;      // ya se vio el mapa una vez en esta partida
    // Cuanto va de la aparicion de cada nodo revelado, de 0 (recien aparece) a 1.
    // Los que ya estaban al abrir por primera vez entran en 1: no se animan.
    std::unordered_map<TechId, float> treeReveal;
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
    // Tierra: 55 (el lado iluminado, el Sol a un lado); Luna: 135 desde la Tierra (la Tierra de lado en cuadro).
    u.camYaw = b == BodyId::Sun ? 0.0f : b == BodyId::Moon ? 135.0f : 55.0f;
    u.camPitch = 35.0f;
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
