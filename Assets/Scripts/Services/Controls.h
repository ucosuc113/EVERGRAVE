#pragma once

// LOS CONTROLES: que tecla o boton hace que, y el puntero ya leido.
//
// Se enlazan una vez (GameRoot::OnStart) con nombres propios `ev_*`. No se usan
// las acciones que el motor trae (`interact`, `jump`...) porque esas las puede
// reenlazar cualquier otro codigo, y un menu que cambia "interact" no tiene por
// que mover el boton de recolectar.

#include "Engine/Input/InputService.h"

namespace evergrave {

using namespace strace;

inline void BindControls() {
    InputActions& in = InputService::Actions();
    auto bindKey = [&](const char* name, Key key) {
        in.ClearAction(name);
        in.BindKey(name, key);
    };
    bindKey("ev_tree", Key::T);
    bindKey("ev_gather", Key::Space);
    bindKey("ev_save", Key::F5);
    bindKey("ev_load", Key::F9);
    bindKey("ev_close", Key::Escape);
    bindKey("ev_scroll_up", Key::Up);
    bindKey("ev_scroll_down", Key::Down);
    in.ClearAction("ev_click");
    in.BindMouseButton("ev_click", MouseButton::Left);
}

// El puntero de ESTE frame, en pixeles de juego (con SubViewport, los internos:
// los mismos en que se dibuja la UI, asi no hay una cuenta distinta para clicar).
struct Pointer {
    float x = 0.0f, y = 0.0f;
    bool inside = false;  // sobre la imagen del juego
    bool clicked = false; // el boton izquierdo se APRETO este frame (flanco)
    bool Over(float rx, float ry, float rw, float rh) const {
        return inside && x >= rx && x < rx + rw && y >= ry && y < ry + rh;
    }
};

inline Pointer ReadPointer() {
    InputActions& in = InputService::Actions();
    Pointer p;
    p.x = in.CursorX();
    p.y = in.CursorY();
    p.inside = in.CursorInside();
    p.clicked = p.inside && in.Pressed("ev_click");
    return p;
}

} // namespace evergrave
