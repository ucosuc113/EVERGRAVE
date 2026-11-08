#pragma once

// EL BUZON ENTRE LOS EFECTOS Y LA ESCENA. WorldFx/EarthFx deciden QUE hay que mostrar (un laser que dispara, un haz de una estacion
// optica) con toda la logica de las tecnologias; Fleet (View/Fleet.cpp) sabe CÓMO se muestra en la escena (un cilindro aditivo de un
// pool, que recibe el brillo y el desenfoque de verdad). Este archivo es lo unico que comparten: una lista de pedidos que se vacia
// cada frame y la bandera "los objetos de la flota existen en la escena".
//
// Cada .cpp de scripts es su propia unidad de compilacion, asi que no pueden compartir una variable de archivo: una funcion `inline`
// con estatica local es la misma en todas (igual que `FxClock`).
//
// Un pedido son DOS PUNTOS DEL MUNDO: lo demas (donde cae, que lo tapa el planeta, el desenfoque) lo resuelve la escena.

#include <vector>

namespace evergrave {

struct WorldBeam {
    float a[3];   // desde (mundo)
    float b[3];   // hasta (mundo)
    float width;  // diametro en metros del mundo, para la camara de la toma por defecto
    float rgb[3]; // color 0..1
    float power;  // intensidad del brillo (0..2)
};

struct FxBusState {
    std::vector<WorldBeam> beams;
    // true si la escena trae el nodo `Fleet` (lo pone Fleet al empezar). Con el, lo que Fleet dibuja ya NO se dibuja como UI.
    bool fleetInScene = false;
    // Lo mismo para `SunWorks` (los anillos de Dyson y la nube de colectores): con el, SunFx deja de dibujarlos como puntos de UI.
    bool sunWorksInScene = false;
};

inline FxBusState& FxBus() {
    static FxBusState s;
    return s;
}

} // namespace evergrave
