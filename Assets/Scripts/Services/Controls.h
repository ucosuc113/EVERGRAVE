#pragma once

// LOS CONTROLES: que tecla o boton hace que, y el puntero ya leido.
//
// Se enlazan una vez (GameRoot::OnStart) con nombres propios `ev_*`. No se usan
// las acciones que el motor trae (`interact`, `jump`...) porque esas las puede
// reenlazar cualquier otro codigo, y un menu que cambia "interact" no tiene por
// que mover el boton de recolectar.

#include "Touch.h"

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
    bindKey("ev_shop", Key::B);
    bindKey("ev_view_earth", Key::Digit1);
    bindKey("ev_view_moon", Key::Digit2);
    bindKey("ev_view_sun", Key::Digit3);
    bindKey("ev_map", Key::M);
    bindKey("ev_gather", Key::Space);
    bindKey("ev_close", Key::Escape);
    bindKey("ev_scroll_up", Key::Up);
    bindKey("ev_scroll_down", Key::Down);
    bindKey("ev_research", Key::Enter); // investigar lo elegido en el panel de tecnologias
    bindKey("ev_tree_left", Key::Left); // moverse de a una tarjeta en la rejilla de ese panel
    bindKey("ev_tree_right", Key::Right);
    // El menu principal: dos teclas para cada lado y dos para aceptar. Esc no cierra nada alli: es un menu, no un panel.
    bindKey("ev_menu_prev", Key::Left);
    in.BindKey("ev_menu_prev", Key::A);
    in.BindKey("ev_menu_prev", Key::Up);
    bindKey("ev_menu_next", Key::Right);
    in.BindKey("ev_menu_next", Key::D);
    in.BindKey("ev_menu_next", Key::Down);
    bindKey("ev_menu_ok", Key::Enter);
    in.BindKey("ev_menu_ok", Key::Space);
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
    if (TouchMode()) {
        // Con el dedo: el clic es un TOQUE, al soltar y sin haberse movido (empezar a arrastrar una lista sobre un boton
        // de comprar no lo aprieta); y no hay hover, el puntero solo esta mientras el dedo esta puesto (o el frame del
        // toque, para que el boton que se toco responda).
        TouchUpdate();
        const TouchGesture& g = Touch();
        p.clicked = p.inside && g.tap;
        p.inside = p.inside && (g.held || g.tap);
    }
    return p;
}

} // namespace evergrave
