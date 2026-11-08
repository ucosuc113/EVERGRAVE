#pragma once

// LAS PIEZAS DE LA INTERFAZ: la paleta, el boton y el texto que se parte en
// lineas. Ninguna sabe de tecnologias ni de recursos: el HUD y el panel las
// usan, y un panel nuevo (otra era del juego) tambien.
//
// El texto es la fuente de pixeles del motor (5x7, ASCII, escala ENTERA). Todo
// se mide y se dibuja en pixeles del juego (UiService::Width/Height), que con
// SubViewport son los internos: un pixel dibujado es un pixel del arte.

#include "../Services/Controls.h"

#include "Engine/UI/PixelFont.h"
#include "Engine/UI/UiDrawList.h"
#include "Engine/UI/UiService.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <string_view>

namespace evergrave::kit {

using namespace strace;

// Paleta: negro, el rojo del logo y blanco; grises para lo apagado.
// Los neutros llevan un poco de azul y NO llegan al negro ni al blanco puros (7/11/2026): negro puro con
// blanco puro es el contraste mas alto que hay, y un panel que se mira minutos cansa. Una pizca de azul
// en el fondo y el texto a ~85% hacen lo mismo de legible con la vista mas tranquila; el rojo del logo
// sigue siendo el acento.
inline UiColor Panel() { return UiColor::Bytes(14, 18, 28, 245); }
inline UiColor Bar() { return UiColor::Bytes(8, 11, 18, 235); }
inline UiColor Field() { return UiColor::Bytes(22, 28, 41); }
inline UiColor Line() { return UiColor::Bytes(50, 60, 80); }
inline UiColor Text() { return UiColor::Bytes(214, 221, 233); }
inline UiColor TextSoft() { return UiColor::Bytes(172, 182, 200); }
inline UiColor Dim() { return UiColor::Bytes(118, 130, 152); }
inline UiColor Faint() { return UiColor::Bytes(72, 82, 102); }
inline UiColor Red() { return UiColor::Bytes(224, 44, 44); }
inline UiColor RedBright() { return UiColor::Bytes(255, 80, 80); }
inline UiColor RedDark() { return UiColor::Bytes(132, 24, 24); }

// "Listo" y "falta": el verde es lo que se puede hacer AHORA (alcanza el saldo) y el ambar lo que todavia no.
// El rojo del logo queda para las acciones y la seleccion: si tambien fuera "no alcanza", un panel lleno de
// costos sin pagar seria un panel rojo, y es lo que cansaba de mirar.
inline UiColor Go() { return UiColor::Bytes(116, 204, 160); }
inline UiColor GoDark() { return UiColor::Bytes(20, 42, 40); }
inline UiColor Warn() { return UiColor::Bytes(214, 172, 104); }

inline UiRect Rect(float x, float y, float w, float h) { return UiRect{x, y, w, h}; }

// Un boton. Devuelve true el frame en que se hizo clic sobre el, estando
// habilitado. Deshabilitado se dibuja apagado y NO responde, pero sigue siendo
// del panel: el clic no se cuela al mundo (eso lo decide pointerOverUi).
//
// `primary` es la accion que se quiere que se vea (investigar, el pulso laser, cerrar): rojo, pero apagado
// (rojo del logo al 100% como relleno de un boton ancho era lo mas fuerte de la pantalla, siempre). Los
// demas botones (abrir/cerrar la tienda...) son neutros y solo se encienden al pasar el puntero.
inline void Card(UiDrawList& ui, const UiRect& r, const UiColor& fill, const UiColor& edge); // mas abajo

inline bool Button(UiDrawList& ui, const UiRect& r, std::string_view label, bool enabled, const Pointer& p, bool primary = true) {
    const bool hot = enabled && p.Over(r.x, r.y, r.width, r.height);
    UiColor fill, edge, ink;
    if (!enabled) {
        fill = UiColor::Bytes(20, 26, 38);
        edge = Line().WithAlpha(0.6f);
        ink = Faint();
    } else if (primary) {
        fill = hot ? UiColor::Bytes(158, 52, 58) : UiColor::Bytes(112, 34, 42);
        edge = hot ? RedBright() : UiColor::Bytes(196, 62, 66);
        ink = Text();
    } else {
        fill = hot ? UiColor::Bytes(40, 52, 76) : UiColor::Bytes(26, 34, 50);
        edge = hot ? TextSoft() : Line();
        ink = hot ? Text() : TextSoft();
    }
    Card(ui, r, fill, edge);
    ui.AddTextCentered(r, label, ink, 1);
    return hot && p.clicked;
}

// Escribe `text` partido para caber en `width` y devuelve la Y donde sigue.
// Una linea ocupa kGlyphLineHeight (8): el 1 de interlineado sobre el alto del
// glifo es lo que la fuente trae, no un ajuste de aca.
inline float Wrapped(UiDrawList& ui, float x, float y, float width, const std::string& text, const UiColor& color) {
    for (const std::string& line : WrapText(text, static_cast<int>(width), 1)) {
        ui.AddText(x, y, line, color, 1);
        y += static_cast<float>(kGlyphLineHeight);
    }
    return y;
}

// Una gema: un cuadrado de lado par con las esquinas cortadas. Es lo que hace que un nodo o
// un icono no parezca un boton de formulario; el arbol y la tienda dibujan las mismas.
inline void Gem(UiDrawList& ui, float cx, float cy, float size, const UiColor& fill, const UiColor& edge) {
    const float w = std::max(6.0f, std::floor(size * 0.5f) * 2.0f);
    const float x = cx - w * 0.5f, y = cy - w * 0.5f;
    ui.AddRect(Rect(x + 2, y, w - 4, w), fill);
    ui.AddRect(Rect(x, y + 2, w, w - 4), fill);
    ui.AddRect(Rect(x + 1, y + 1, w - 2, w - 2), fill);
    ui.AddRect(Rect(x + 2, y, w - 4, 1), edge);
    ui.AddRect(Rect(x + 2, y + w - 1, w - 4, 1), edge);
    ui.AddRect(Rect(x, y + 2, 1, w - 4), edge);
    ui.AddRect(Rect(x + w - 1, y + 2, 1, w - 4), edge);
    ui.AddRect(Rect(x + 1, y + 1, 1, 1), edge);
    ui.AddRect(Rect(x + w - 2, y + 1, 1, 1), edge);
    ui.AddRect(Rect(x + 1, y + w - 2, 1, 1), edge);
    ui.AddRect(Rect(x + w - 2, y + w - 2, 1, 1), edge);
}

// Una tarjeta: un rectangulo con las esquinas cortadas un pixel (la misma idea que la gema, para que
// nada se vea como un boton de formulario). `edge` con alfa 0 = sin borde.
inline void Card(UiDrawList& ui, const UiRect& r, const UiColor& fill, const UiColor& edge) {
    ui.AddRect(Rect(r.x + 1, r.y, r.width - 2, r.height), fill);
    ui.AddRect(Rect(r.x, r.y + 1, r.width, r.height - 2), fill);
    if (edge.a <= 0.0f) return;
    ui.AddRect(Rect(r.x + 1, r.y, r.width - 2, 1), edge);
    ui.AddRect(Rect(r.x + 1, r.y + r.height - 1, r.width - 2, 1), edge);
    ui.AddRect(Rect(r.x, r.y + 1, 1, r.height - 2), edge);
    ui.AddRect(Rect(r.x + r.width - 1, r.y + 1, 1, r.height - 2), edge);
}

// Un degradado vertical de `top` a `bottom` en franjas de `step` px: un fondo plano es un bloque, y uno
// con un poco de caida se lee como un cielo y no como un agujero.
inline void VGradient(UiDrawList& ui, const UiRect& r, const UiColor& top, const UiColor& bottom, float step = 8.0f) {
    for (float y = 0.0f; y < r.height; y += step) {
        const float t = std::clamp((y + step * 0.5f) / std::max(1.0f, r.height), 0.0f, 1.0f);
        const UiColor c{top.r + (bottom.r - top.r) * t, top.g + (bottom.g - top.g) * t, top.b + (bottom.b - top.b) * t, top.a + (bottom.a - top.a) * t};
        ui.AddRect(Rect(r.x, r.y + y, r.width, std::min(step, r.height - y)), c);
    }
}

inline float TextW(std::string_view t, int scale = 1) { return static_cast<float>(MeasureTextWidth(t, scale)); }

} // namespace evergrave::kit
