#pragma once

// EL RELOJ DE LOS EFECTOS. WorldFx lo avanza (y lo CONGELA con EVERGRAVE_FXTIME, para que una captura se pueda repetir); Fleet lo lee.
//
// Existe porque los satelites son ahora objetos de la escena (Fleet) y sus pulsos de relay siguen siendo UI (WorldFx): los dos tienen
// que calcular la posicion de cada satelite con el MISMO tiempo, o el pulso nace donde el satelite ya no esta. Cada .cpp de scripts es
// su propia unidad de compilacion, asi que no pueden compartir una variable de archivo: una funcion `inline` con estatica local es la
// misma en todas.

namespace evergrave {

struct FxClockState {
    float time = 0.0f;
};

inline FxClockState& FxClock() {
    static FxClockState s;
    return s;
}

} // namespace evergrave
