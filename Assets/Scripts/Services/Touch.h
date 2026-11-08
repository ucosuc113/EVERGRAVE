#pragma once

// EL DEDO: lo que hace falta para jugar EVERGRAVE en un telefono.
//
// En Android (y en Web, desde un telefono) el primer dedo ya es el cursor y el boton izquierdo del raton, y los demas
// llegan como posiciones (`TouchCount/TouchX/TouchY`). Asi que casi todo el juego YA se toca: cada accion tiene su
// boton en pantalla (SHOP, RESEARCH, EARTH/MOON/SUN/MAP, CLOSE, la recoleccion) y tocar la Tierra recolecta. Lo que
// faltaba es lo que en el PC hacen la rueda y el apretar-al-instante:
//
//   - el ZOOM (la rueda)             -> pellizco con dos dedos (CameraRig)
//   - desplazar las listas (rueda)   -> arrastrar con un dedo (tienda, arbol, detalle)
//   - el clic, que en el PC es al APRETAR: con el dedo, empezar a arrastrar una lista sobre un boton de comprar lo
//     habria comprado antes de saber que era un arrastre. Con el dedo un clic es un TOQUE y se informa al SOLTAR, y solo
//     si el dedo casi no se movio (`Pointer::clicked` lo hace solo en este modo: ningun panel cambia).
//   - el hover: en un telefono no existe; el "puntero" solo esta mientras el dedo esta puesto.
//
// En cuanto se apoya un SEGUNDO dedo, lo de un dedo se cancela hasta que se levanten todos: ni un toque al soltar ni
// un giro de la camara con el dedo que queda (el cursor salta a el, y eso no es un arrastre de nadie).
//
// El modo se activa solo: siempre en Android, y en cualquier otra plataforma en cuanto llega el PRIMER dedo (una web
// abierta en un telefono; con raton no llega ninguno y nada cambia). En el Editor se prueba con EVERGRAVE_TOUCH=1: el
// raton hace de dedo.
//
// El umbral de arrastre se mide en fraccion del ALTO de la imagen y no en pixeles: un telefono de 1080 filas y uno de
// 720 tienen que sentir lo mismo con el mismo dedo.

#include "Engine/Gameplay/GameTime.h"
#include "Engine/Input/InputService.h"
#include "Engine/UI/UiDrawList.h"
#include "Engine/UI/UiService.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>

namespace evergrave {

// true en Android; en el resto, con EVERGRAVE_TOUCH=1 o en cuanto se ve un dedo.
inline bool TouchMode() {
    using namespace strace;
#if defined(__ANDROID__)
    return true;
#else
    static const bool forced = [] {
#if defined(_MSC_VER)
        char buf[8] = {};
        size_t len = 0;
        return getenv_s(&len, buf, sizeof(buf), "EVERGRAVE_TOUCH") == 0 && len > 0 && buf[0] == '1';
#else
        const char* v = std::getenv("EVERGRAVE_TOUCH");
        return v && v[0] == '1';
#endif
    }();
    static bool seen = false;
    if (!seen && InputService::Actions().TouchCount() > 0) seen = true;
    return forced || seen;
#endif
}

struct TouchGesture {
    uint64_t frame = ~0ull;  // el frame de la ultima actualizacion: Update es idempotente dentro de un frame
    bool held = false, wasHeld = false;
    bool began = false, tap = false;     // `tap`: el dedo se levanto este frame sin haberse movido (un clic)
    bool dragging = false;               // el dedo se movio mas que el umbral: ya no es un toque
    bool cancelled = false;              // hubo dos dedos: nada de un dedo cuenta hasta que se levanten todos
    int fingers = 0;
    float startX = 0, startY = 0, lastX = 0, lastY = 0;
    float dx = 0, dy = 0;                // cuanto se movio el dedo este frame (solo si arrastra)
    float pinch = 1.0f;                  // distancia actual entre los dos dedos / la del frame anterior (1 = quieto)
    float prevDist = 0.0f;
    int prevFingers = 0;
    bool startedOnUi = false;
};

inline TouchGesture& Touch() {
    static TouchGesture g;
    return g;
}

constexpr float kTouchDragFraction = 0.022f; // del alto de la imagen: ~8 px a 360 filas

// Una vez por frame (GameRoot la llama primero; quien la llame antes en el mismo frame, hace lo mismo).
inline void TouchUpdate() {
    using namespace strace;
    TouchGesture& g = Touch();
    const uint64_t frame = GameTime::FrameCount();
    if (g.frame == frame) return;
    g.frame = frame;

    InputActions& in = InputService::Actions();
    g.fingers = in.TouchCount();
    g.held = in.Held("ev_click");
    g.began = g.held && !g.wasHeld;
    g.tap = false;
    g.dx = g.dy = 0.0f;
    g.pinch = 1.0f;

    const float x = in.CursorX(), y = in.CursorY();
    const float h = std::max(1.0f, UiService::Height());

    if (g.began) {
        g.startX = g.lastX = x;
        g.startY = g.lastY = y;
        g.dragging = false;
        g.cancelled = false;
    }
    if (g.fingers >= 2) {
        g.cancelled = true;
        const float ddx = in.TouchX(0) - in.TouchX(1), ddy = in.TouchY(0) - in.TouchY(1);
        const float dist = std::sqrt(ddx * ddx + ddy * ddy);
        // Solo entre dos frames con la MISMA cuenta de dedos: si uno entra o sale, los indices se corren y la
        // distancia saltaria de un par de dedos a otro.
        if (g.prevFingers == g.fingers && g.prevDist > 1.0f && dist > 1.0f) g.pinch = dist / g.prevDist;
        g.prevDist = dist;
    } else {
        g.prevDist = 0.0f;
    }
    g.prevFingers = g.fingers;

    if (g.held && !g.cancelled) {
        const float tx = x - g.startX, ty = y - g.startY;
        if (!g.dragging && std::sqrt(tx * tx + ty * ty) > kTouchDragFraction * h) g.dragging = true;
        if (g.dragging && !g.began) {
            g.dx = x - g.lastX;
            g.dy = y - g.lastY;
        }
        g.lastX = x;
        g.lastY = y;
    }
    if (!g.held && g.wasHeld && !g.dragging && !g.cancelled) g.tap = true;
    g.wasHeld = g.held;
}

// Cuanto hay que desplazar una lista este frame: lo que arrastra el dedo, si el arrastre EMPEZO dentro de `area`.
// 0 con raton o sin arrastre.
inline float TouchScrollY(const strace::UiRect& area) {
    if (!TouchMode()) return 0.0f;
    const TouchGesture& g = Touch();
    if (!g.held || !g.dragging || g.cancelled) return 0.0f;
    if (g.startX < area.x || g.startX >= area.x + area.width || g.startY < area.y || g.startY >= area.y + area.height) return 0.0f;
    return g.dy;
}

} // namespace evergrave
