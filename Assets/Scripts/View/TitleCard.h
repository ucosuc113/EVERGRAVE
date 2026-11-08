#pragma once

// EL TITULO ANTES DEL MENU: "for my precious moon" en una pantalla negra, y luego un telon que se abre sobre el
// menu. Todo el tiempo (UiState::titleT) lo cuenta GameRoot; esto solo dibuja ese instante.
//
//   0.0 - 1.0   la frase aparece (y una linea fina se extiende debajo)
//   1.0 - 3.2   se queda quieta: es lo que se lee
//   3.2 - 4.0   la frase se desvanece subiendo un poco
//   3.6 - 4.8   el telon: la pantalla negra se parte en dos mitades que salen por arriba y por abajo, y deja ver el
//               menu (la Tierra y los satelites) ya en su sitio
//
// Se dibuja como UI inmediata, que va ENCIMA de la interfaz de la escena (el Canvas del menu), asi que el menu esta
// encendido detras todo el rato y el telon solo lo descubre: no hay un corte entre "titulo" y "menu".
// La fuente es la de 5x7 del motor (solo ASCII), a escala entera como todo el juego.

#include "../Services/Session.h"
#include "UiKit.h"

#include "Engine/UI/UiService.h"

#include <algorithm>
#include <cmath>
#include <string_view>

namespace evergrave {

namespace titlecard {
constexpr float kFadeIn = 1.0f;
constexpr float kHoldUntil = 3.2f;
constexpr float kFadeOutEnd = 4.0f;
constexpr float kCurtainStart = 3.6f;
constexpr const char* kText = "for my precious moon";
constexpr int kScale = 2;
constexpr float kTracking = 5.0f; // aire extra entre letras: una frase de una linea, que respire

inline float Clamp01(float x) { return std::clamp(x, 0.0f, 1.0f); }
inline float EaseInOut(float k) { return k < 0.5f ? 4.0f * k * k * k : 1.0f - std::pow(-2.0f * k + 2.0f, 3.0f) * 0.5f; }
inline float EaseOut(float k) { return 1.0f - (1.0f - k) * (1.0f - k); }

// Ancho de la frase con el aire entre letras.
inline float TrackedWidth(std::string_view t) {
    float w = 0.0f;
    for (const char c : t) w += kit::TextW(std::string_view(&c, 1), kScale) + kTracking;
    return w - kTracking;
}
inline void Tracked(UiDrawList& ui, float x, float y, std::string_view t, const UiColor& color) {
    for (const char c : t) {
        ui.AddText(std::round(x), std::round(y), std::string_view(&c, 1), color, kScale);
        x += kit::TextW(std::string_view(&c, 1), kScale) + kTracking;
    }
}
} // namespace titlecard

inline void DrawTitleCard() {
    using namespace titlecard;
    UiState& state = Ui();
    if (!state.titleActive) return;
    const float t = state.titleT;
    UiDrawList& ui = UiService::Frame();
    const float W = static_cast<float>(UiService::Width()), H = static_cast<float>(UiService::Height());

    // El texto: aparece, se queda, se va hacia arriba.
    const float inK = EaseOut(Clamp01(t / kFadeIn));
    const float outK = EaseInOut(Clamp01((t - kHoldUntil) / (kFadeOutEnd - kHoldUntil)));
    const float alpha = inK * (1.0f - outK);
    const UiRect vis = UiService::VisibleRect();
    const float cx = vis.x + vis.width * 0.5f, cy = vis.y + vis.height * 0.5f;
    const float lift = outK * 10.0f;

    // El telon: dos mitades negras que se alejan del centro. Antes de que empiece, cubren todo.
    const float curtain = EaseInOut(Clamp01((t - kCurtainStart) / (kTitleSeconds - kCurtainStart)));
    const float half = H * 0.5f;
    const float slide = curtain * (half + 2.0f);
    const UiColor black = UiColor::Bytes(0, 0, 0);
    if (curtain < 1.0f) {
        ui.AddRect(UiRect{-2.0f, -2.0f - slide, W + 4.0f, half + 2.0f}, black);          // la mitad de arriba sube
        ui.AddRect(UiRect{-2.0f, half + slide, W + 4.0f, half + 2.0f}, black);            // la de abajo baja
    }

    if (alpha > 0.002f) {
        const float w = TrackedWidth(kText);
        const float h = static_cast<float>(MeasureTextHeight(kText, kScale));
        const uint8_t a = static_cast<uint8_t>(std::round(255.0f * alpha));
        // Mientras el telon se abre, la frase ya casi no esta; si aun queda algo, se recorta con el centro (la
        // rendija) para que no se vea flotar sobre el menu.
        Tracked(ui, cx - w * 0.5f, cy - h * 0.5f - lift, kText, UiColor::Bytes(214, 221, 233, a));
        // La linea fina de debajo: crece desde el centro con la aparicion y se queda del ancho de la frase.
        const float lw = w * inK;
        ui.AddRect(UiRect{std::round(cx - lw * 0.5f), std::round(cy + h * 0.5f + 12.0f - lift), std::round(lw), 1.0f},
                   UiColor::Bytes(118, 130, 152, static_cast<uint8_t>(std::round(150.0f * alpha))));
    }
}

} // namespace evergrave
