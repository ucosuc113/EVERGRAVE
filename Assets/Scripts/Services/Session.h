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
    bool treeOpen = false;
    TechId selected;         // vacio = nada elegido
    float scrollY = 0.0f;    // desplazamiento del arbol
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

inline void Toast(const std::string& text, float seconds = 2.5f) {
    Ui().toast = text;
    Ui().toastTime = seconds;
}

} // namespace evergrave
