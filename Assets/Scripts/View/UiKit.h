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
inline UiColor Panel() { return UiColor::Bytes(10, 10, 14, 245); }
inline UiColor Bar() { return UiColor::Bytes(4, 4, 6, 235); }
inline UiColor Field() { return UiColor::Bytes(20, 20, 25); }
inline UiColor Line() { return UiColor::Bytes(70, 70, 80); }
inline UiColor Text() { return UiColor::Bytes(235, 235, 235); }
inline UiColor TextSoft() { return UiColor::Bytes(190, 190, 200); }
inline UiColor Dim() { return UiColor::Bytes(135, 135, 148); }
inline UiColor Faint() { return UiColor::Bytes(80, 80, 92); }
inline UiColor Red() { return UiColor::Bytes(214, 40, 40); }
inline UiColor RedBright() { return UiColor::Bytes(255, 80, 80); }
inline UiColor RedDark() { return UiColor::Bytes(110, 20, 20); }

inline UiRect Rect(float x, float y, float w, float h) { return UiRect{x, y, w, h}; }

// Un boton. Devuelve true el frame en que se hizo clic sobre el, estando
// habilitado. Deshabilitado se dibuja apagado y NO responde, pero sigue siendo
// del panel: el clic no se cuela al mundo (eso lo decide pointerOverUi).
inline bool Button(UiDrawList& ui, const UiRect& r, std::string_view label, bool enabled, const Pointer& p) {
    const bool hot = enabled && p.Over(r.x, r.y, r.width, r.height);
    const UiColor fill = !enabled ? Field() : hot ? RedBright().WithAlpha(0.35f) : RedDark();
    const UiColor edge = !enabled ? Line() : hot ? RedBright() : Red();
    const UiColor ink = !enabled ? Faint() : Text();
    ui.AddRect(r, fill);
    ui.AddRectOutline(r, edge, 1.0f);
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

inline float TextW(std::string_view t, int scale = 1) { return static_cast<float>(MeasureTextWidth(t, scale)); }

} // namespace evergrave::kit
