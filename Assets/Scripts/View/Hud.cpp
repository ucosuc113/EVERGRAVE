// Hud: las dos barras que siempre estan - el saldo arriba, recolectar abajo - y
// el aviso de una linea. Solo LEE la sesion y le pide dos cosas: recolectar y
// abrir o cerrar el panel. No decide nada de la partida.
//
// Las barras dejan libre el centro de la pantalla a proposito: el panel del
// arbol se abre entre ellas, asi que recolectar y ver el saldo siguen a mano
// con el arbol abierto (la produccion no se detiene mientras se lee).

#include "UiKit.h"
#include "Format.h"
#include "../Services/Session.h"
#include "../Services/UseFeedback.h"

#include "Engine/Scripting/IScript.h"
#include "Engine/Scripting/ScriptRegistry.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

using namespace strace;
using namespace evergrave;

namespace {
constexpr float kBarH = 28.0f;
}

class Hud : public IScript {
public:
    void OnUpdate(float dt) override {
        UiDrawList& ui = UiService::Frame();
        const float W = UiService::Width();
        // El HUD se ancla al AREA VISIBLE, no a los bordes de la imagen: con el
        // pixelado, una ventana donde la escala se redondea hacia arriba recorta
        // la imagen y las barras de los bordes se perderian. (VisibleRect es la
        // imagen entera cuando no hay recorte.)
        const UiRect vis = UiService::VisibleRect();
        const float L = vis.x, R = vis.Right(), T = vis.y, B = vis.Bottom(), cx = vis.x + vis.width * 0.5f;
        const Pointer p = ReadPointer();
        GameState& game = Session();
        UiState& state = Ui();

        // --- Barra de arriba: saldo, produccion, boton del arbol ---
        // El fondo de la barra ocupa todo el ancho; su contenido, el visible.
        const UiRect top = kit::Rect(0, T, W, kBarH);
        ui.AddRect(top, kit::Bar());
        ui.AddRect(kit::Rect(0, T + kBarH - 1, W, 1), kit::Line());

        const std::string balance = FormatAmount(game.wallet().Get(ResourceId::Research));
        ui.AddText(L + 8, T + 7, balance, kit::Text(), 2);
        const float tx = L + 8.0f + kit::TextW(balance, 2) + 8.0f;
        ui.AddText(tx, T + 6, InfoOf(ResourceId::Research).name, kit::Dim(), 1);
        ui.AddText(tx, T + 16, FormatRate(game.PassiveRate()) + "   gather +" + FormatAmount(game.GatherAmount()), kit::TextSoft(), 1);

        const UiRect shopBtn = kit::Rect(R - 128.0f - 96.0f, T + 4, 88, 20);
        if (kit::Button(ui, shopBtn, state.shopOpen ? "HIDE SHOP [B]" : "SHOP [B]", true, p)) state.shopOpen = !state.shopOpen;
        const UiRect treeBtn = kit::Rect(R - 128.0f, T + 4, 120, 20);
        if (kit::Button(ui, treeBtn, state.treeOpen ? "CLOSE TREE [T]" : "TECH TREE [T]", true, p)) state.treeOpen = !state.treeOpen;

        // --- El selector de vista: de un vistazo, donde se esta y a donde se puede ir ---
        // Tierra, Luna, Sol y Mapa, y el Dyson al final (reservado: aun no existe). Los cerrados se ven
        // desde el primer segundo, apagados: la persona sabe que hay mas, y al pasar el puntero, que
        // tecnologia lo abre.
        {
            struct Slot { const char* label; BodyId body; bool isMap; };
            const Slot slots[] = {{"EARTH", BodyId::Earth, false}, {"MOON", BodyId::Moon, false}, {"SUN", BodyId::Sun, false},
                                  {"MAP", BodyId::Earth, true}, {"DYSON", BodyId::Dyson, false}};
            const float bw = 50.0f, gap = 3.0f;
            const float total = 5 * bw + 4 * gap;
            float x = std::floor(cx - total * 0.5f);
            const float y = T + kBarH + 4.0f;
            const Slot* hoverSlot = nullptr;
            float hoverX = 0.0f;
            for (const Slot& s : slots) {
                const UiRect r = kit::Rect(x, y, bw, 16);
                const bool open = s.isMap ? game.IsMapUnlocked() : game.IsBodyUnlocked(s.body);
                const bool selected = s.isMap ? state.mapView : (!state.mapView && state.view == s.body);
                const bool hot = p.Over(r.x, r.y, r.width, r.height);
                ui.AddRect(r, selected ? kit::RedDark() : kit::Field());
                ui.AddRectOutline(r, selected ? kit::Red() : hot && open ? kit::Text() : open ? kit::Line() : kit::Faint().WithAlpha(0.6f), 1.0f);
                ui.AddTextCentered(r, s.label, open ? (selected ? kit::Text() : kit::TextSoft()) : kit::Faint(), 1);
                if (hot) {
                    state.pointerOverUi = true;
                    hoverSlot = &s;
                    hoverX = r.x;
                    if (open && p.clicked) {
                        if (s.isMap) {
                            if (state.mapView) GoToBody(state.view);
                            else GoToMap();
                        } else {
                            GoToBody(s.body);
                        }
                    }
                }
                x += bw + gap;
            }
            if (hoverSlot) {
                const bool open = hoverSlot->isMap ? game.IsMapUnlocked() : game.IsBodyUnlocked(hoverSlot->body);
                std::string msg;
                if (open) msg = hoverSlot->isMap ? "Zoom out to the whole system" : std::string("Go to the ") + hoverSlot->label;
                else {
                    const std::string key = hoverSlot->isMap ? kMapKey : InfoOf(hoverSlot->body).key;
                    const std::string who = UnlockerName(game.tree(), key);
                    msg = who.empty() ? "Not designed yet" : "Research " + who + " to unlock";
                }
                const float w = kit::TextW(msg) + 12.0f;
                const UiRect tip = kit::Rect(std::floor(std::clamp(hoverX + bw * 0.5f - w * 0.5f, L + 4.0f, R - w - 4.0f)), y + 20.0f, w, 14);
                ui.AddRect(tip, UiColor::Bytes(4, 4, 7, 245));
                ui.AddRectOutline(tip, kit::Faint(), 1.0f);
                ui.AddTextCentered(tip, msg, open ? kit::Text() : kit::Dim(), 1);
            }
        }

        // --- Barra de abajo: recolectar, atajos, progreso ---
        const float by = B - kBarH;
        ui.AddRect(kit::Rect(0, by, W, kBarH), kit::Bar());
        ui.AddRect(kit::Rect(0, by, W, 1), kit::Line());

        // Los botones son las interacciones de tipo Button que YA se desbloquearon:
        // una tecnologia nueva agrega uno aqui sin tocar este archivo. Mientras no
        // hay ninguno, la barra dice cual es la unica accion que hay.
        std::vector<const InteractionDef*> buttons;
        for (const InteractionDef& d : game.interactions().All())
            if (d.target == InteractionTarget::Button && game.IsUnlocked(d.id)) buttons.push_back(&d);

        if (buttons.empty()) {
            const std::string hint = "CLICK THE EARTH TO GATHER DATA";
            ui.AddText(std::floor(cx - kit::TextW(hint) * 0.5f), by + 10, hint, kit::TextSoft(), 1);
        } else {
            const float bw = 150.0f, gap = 8.0f;
            float x = std::floor(cx - (buttons.size() * bw + (buttons.size() - 1) * gap) * 0.5f);
            for (const InteractionDef* d : buttons) {
                const double left = game.CooldownLeft(d->id);
                const UiRect r = kit::Rect(x, by + 4, bw, 20);
                char wait[24];
                std::snprintf(wait, sizeof wait, " %.1fs", left);
                const std::string label = d->name + (left > 0.0 ? wait : "");
                if (kit::Button(ui, r, label, left <= 0.0, p)) UseWithFeedback(d->id, r.x + bw * 0.5f, r.y);
                if (left > 0.0 && d->cooldown > 0.0)
                    ui.AddRect(kit::Rect(r.x + 1, r.Bottom() - 3, (bw - 2) * static_cast<float>(1.0 - left / d->cooldown), 2), kit::Red());
                x += bw + gap;
            }
        }

        ui.AddText(L + 8, by + 10, "F5 SAVE  F9 LOAD", kit::Dim(), 1);
        const std::string progress = "TECH " + std::to_string(game.tree().ResearchedCount()) + "/" + std::to_string(game.tree().All().size());
        ui.AddText(R - 8.0f - kit::TextW(progress), by + 10, progress, kit::Dim(), 1);

        // --- Aviso ---
        if (state.toastTime > 0.0f && !state.toast.empty()) {
            const float a = state.toastTime < 0.5f ? state.toastTime / 0.5f : 1.0f;
            const float w = kit::TextW(state.toast) + 16.0f;
            const UiRect r = kit::Rect(std::floor(cx - w * 0.5f), by - 22.0f, w, 16);
            ui.AddRect(r, kit::Bar().WithAlpha(0.9f * a));
            ui.AddRectOutline(r, kit::Red().WithAlpha(a), 1.0f);
            ui.AddTextCentered(r, state.toast, kit::Text().WithAlpha(a), 1);
        }

        // --- Numeros que suben donde se hizo clic ---
        std::vector<UiState::Floater>& fl = state.floaters;
        for (UiState::Floater& f : fl) f.age += dt;
        fl.erase(std::remove_if(fl.begin(), fl.end(), [](const UiState::Floater& f) { return f.age > 1.1f; }), fl.end());
        for (const UiState::Floater& f : fl) {
            const float a = f.age < 0.7f ? 1.0f : 1.0f - (f.age - 0.7f) / 0.4f;
            const float rise = std::floor(f.age * 26.0f);
            ui.AddText(std::floor(f.x - kit::TextW(f.text, 2) * 0.5f) + 1, std::floor(f.y - rise) + 1, f.text, UiColor::Bytes(0, 0, 0).WithAlpha(a), 2);
            ui.AddText(std::floor(f.x - kit::TextW(f.text, 2) * 0.5f), std::floor(f.y - rise), f.text, kit::Text().WithAlpha(a), 2);
        }

        // El mundo no tiene que reaccionar a un clic que cae sobre una barra.
        if (p.Over(0, T, W, kBarH) || p.Over(0, by, W, kBarH)) state.pointerOverUi = true;
    }
};

STRACE_REGISTER_SCRIPT(Hud)
