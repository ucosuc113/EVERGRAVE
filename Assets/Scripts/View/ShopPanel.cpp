// ShopPanel: la tienda. Una columna a la izquierda, abierta desde el primer segundo, con
// lo que se puede comprar una y otra vez: cada fila es un tipo de instalacion, con
// cuantas se tienen, lo que cuesta la siguiente y lo que da. Es el "Cookie Clicker" del
// juego: el arbol de tecnologias abre tipos nuevos y aqui se compran.
//
// Como el resto de paneles, no contiene reglas: pregunta a `GameState` (cuanto cuesta,
// si esta abierta, cuanto da) y le pide UNA cosa, comprar. Es `BuyInstallation` quien
// valida y cobra; un boton "comprar" aqui no puede cobrar de mas ni abrir lo cerrado.
//
// Que se ve, y como se va abriendo:
//  - las abiertas (su tecnologia esta investigada), para comprar;
//  - la SIGUIENTE, apagada, diciendo que tecnologia la abre: una fila cuya tecnologia ya
//    aparece en el mapa. Asi la tienda no es una lista de dieciocho cosas que no se
//    pueden tocar, sino lo que hay a la mano y lo que viene.
//
// Arriba, el modo de compra (x1, x10, x100, o todo lo que alcance): el costo de cada fila
// es el de ESE numero de unidades, que es lo que se cobra.
//
// Al pasar el puntero por una fila sale su ficha a la derecha: que es, cuanto da cada
// una, cuanto produce el total y cuanto falta para el proximo hito.

#include "Format.h"
#include "UiKit.h"
#include "../Services/Session.h"

#include "Engine/Scripting/IScript.h"
#include "Engine/Scripting/ScriptRegistry.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>
#include <unordered_set>
#include <vector>

using namespace strace;
using namespace evergrave;

namespace {

constexpr float kBarH = 28.0f;
constexpr float kShopW = 172.0f;
constexpr float kRowH = 38.0f;
constexpr float kHeaderH = 36.0f;
constexpr float kScrollStep = 38.0f;

UiColor ColorOf(std::uint32_t rgb, float a = 1.0f) {
    return UiColor::Bytes(static_cast<int>((rgb >> 16) & 0xFF), static_cast<int>((rgb >> 8) & 0xFF), static_cast<int>(rgb & 0xFF),
                          static_cast<int>(255.0f * std::clamp(a, 0.0f, 1.0f)));
}

const char* kModeLabel[] = {"x1", "x10", "x100", "MAX"};
constexpr int kModeCount[] = {1, 10, 100, 0};

struct Row {
    const InstallationDef* def = nullptr;
    bool unlocked = false;
};

} // namespace

class ShopPanel : public IScript {
public:
    void OnUpdate(float dt) override {
        if (Ui().menu) return; // el menu principal esta abierto
        UiState& state = Ui();
        GameState& game = Session();
        InputActions& in = InputService::Actions();

        if (in.Pressed("ev_shop")) state.shopOpen = !state.shopOpen;
        state.shopFlash = std::max(0.0f, state.shopFlash - dt * 4.0f);
        // El arbol ocupa la pantalla entera: con el abierto, la tienda se queda quieta detras.
        // En el mapa tampoco: no se construye desde lejos, se elige a donde ir.
        if (state.treeOpen || !state.shopOpen || state.mapView) return;

        UiDrawList& ui = UiService::Frame();
        const UiRect vis = UiService::VisibleRect();
        const Pointer p = ReadPointer();
        const TechTree& tree = game.tree();

        // A la IZQUIERDA: la derecha es del Sol (esta arriba a la derecha, medio detras de la barra, y se le
        // hace clic). Una columna de tienda encima lo taparia.
        const UiRect shop = kit::Rect(vis.x + 6.0f, vis.y + kBarH + 4.0f, kShopW, vis.height - 2.0f * kBarH - 8.0f);
        const UiRect list = kit::Rect(shop.x + 2, shop.y + kHeaderH, shop.width - 4, shop.height - kHeaderH - 3);
        ui.AddRect(shop, kit::Panel());
        ui.AddRectOutline(shop, kit::Line(), 1.0f);
        if (p.Over(shop.x, shop.y, shop.width, shop.height)) state.pointerOverUi = true;

        // --- Cabecera: titulo, total de unidades y modo de compra ---
        ui.AddText(shop.x + 7, shop.y + 5, std::string("SHOP - ") + InfoOf(state.view).name, kit::Text(), 1);
        const std::string total = "x" + std::to_string(game.OwnedOn(state.view));
        ui.AddText(shop.Right() - 7.0f - kit::TextW(total), shop.y + 5, total, kit::Dim(), 1);
        const float bw = (shop.width - 14.0f - 3.0f * 3.0f) / 4.0f;
        for (int m = 0; m < 4; ++m) {
            const UiRect r = kit::Rect(shop.x + 7 + m * (bw + 3.0f), shop.y + 17, bw, 14);
            const bool sel = state.shopBuyMode == m;
            const bool hot = p.Over(r.x, r.y, r.width, r.height);
            ui.AddRect(r, sel ? kit::RedDark() : kit::Field());
            ui.AddRectOutline(r, sel ? kit::Red() : hot ? kit::Text() : kit::Line(), 1.0f);
            ui.AddTextCentered(r, kModeLabel[m], sel ? kit::Text() : kit::Dim(), 1);
            if (hot && p.clicked) state.shopBuyMode = m;
        }

        // --- Que filas hay: las abiertas y las que vienen ---
        const std::unordered_set<TechId> seen = tree.RevealedIds();
        std::vector<Row> rows;
        for (const InstallationDef& d : game.installations().All()) {
            if (d.body != state.view) continue; // la tienda es de cada cuerpo; el arbol, no
            const bool unlocked = game.IsInstallationUnlocked(d.id);
            if (unlocked || seen.count(d.unlockedBy)) rows.push_back({&d, unlocked});
        }

        const float contentH = static_cast<float>(rows.size()) * kRowH;
        const bool overList = p.Over(list.x, list.y, list.width, list.height);
        if (overList) state.shopScroll -= in.WheelDelta() * kScrollStep;
        state.shopScroll = std::clamp(state.shopScroll, 0.0f, std::max(0.0f, contentH - list.height));

        if (rows.empty()) {
            kit::Wrapped(ui, shop.x + 8, list.y + 8, shop.width - 16, "Nothing to build here yet. Research technologies to unlock things.", kit::Dim());
            return;
        }

        // --- Las filas ---
        const Wallet& wallet = game.wallet();
        const Row* hover = nullptr;
        float hoverY = 0.0f;
        ui.PushClipRect(list);
        for (std::size_t i = 0; i < rows.size(); ++i) {
            const InstallationDef& d = *rows[i].def;
            const float y = list.y + static_cast<float>(i) * kRowH - state.shopScroll;
            if (y + kRowH < list.y || y > list.Bottom()) continue;
            const UiRect r = kit::Rect(list.x + 2, y + 1, list.width - 4, kRowH - 2);
            const bool hot = overList && p.Over(r.x, r.y, r.width, r.height);
            if (hot) {
                hover = &rows[i];
                hoverY = y;
            }

            // Cuantas se compran y lo que cuestan: el modo, acotado a lo que alcance en MAX.
            const int mode = state.shopBuyMode;
            const int n = kModeCount[mode] > 0 ? kModeCount[mode] : std::max(1, game.MaxAffordable(d.id));
            const double cost = game.CostOfNext(d.id, n);
            const bool unlocked = rows[i].unlocked;
            const bool canBuy = unlocked && wallet.CanAfford({{ResourceId::Research, cost}});

            const UiColor tint = ColorOf(d.color);
            ui.AddRect(r, hot && unlocked ? kit::Field() : UiColor::Bytes(13, 13, 18));
            ui.AddRectOutline(r, canBuy ? ColorOf(d.color, hot ? 0.95f : 0.55f) : hot && unlocked ? kit::Dim() : kit::Faint().WithAlpha(0.55f), 1.0f);
            if (state.shopFlash > 0.0f && state.shopFlashId == d.id)
                ui.AddRect(r, UiColor::Bytes(255, 255, 255, static_cast<int>(70.0f * state.shopFlash)));

            // Icono: una gema del color de la instalacion; apagada si todavia no se abre.
            kit::Gem(ui, r.x + 14, r.y + r.height * 0.5f, 16, unlocked ? ColorOf(d.color, 0.25f) : UiColor::Bytes(12, 12, 15),
                     unlocked ? tint : kit::Faint());

            const float textX = r.x + 28.0f;
            const std::string count = unlocked ? std::to_string(game.Owned(d.id)) : "";
            const float countW = count.empty() ? 0.0f : kit::TextW(count, 2);
            if (!count.empty()) ui.AddText(r.Right() - 5.0f - countW, r.y + 3, count, game.Owned(d.id) > 0 ? kit::TextSoft() : kit::Faint(), 2);

            const std::vector<std::string> lines = WrapText(d.name, static_cast<int>(r.width - 28.0f - countW - 6.0f), 1);
            for (std::size_t k = 0; k < lines.size() && k < 2; ++k)
                ui.AddText(textX, r.y + 4 + static_cast<float>(k) * kGlyphLineHeight, lines[k], unlocked ? kit::Text() : kit::Dim(), 1);

            if (unlocked) {
                ui.AddText(textX, r.Bottom() - 11.0f, FormatExact(cost), canBuy ? kit::Text() : kit::Red(), 1);
                const std::string rate = "+" + FormatRate(game.OutputOfNext(d.id));
                ui.AddText(r.Right() - 5.0f - kit::TextW(rate), r.Bottom() - 11.0f, rate, kit::Dim(), 1);
            } else {
                const TechDef* need = tree.Find(d.unlockedBy);
                // Solo el nombre de la tecnologia: "NEEDS" ya no cabia con nombres largos.
                const std::string msg = "> " + (need ? need->name : d.unlockedBy);
                ui.AddText(textX, r.Bottom() - 11.0f, WrapText(msg, static_cast<int>(r.width - 32.0f), 1).front(), kit::Red().WithAlpha(0.75f), 1);
            }

            // Comprar.
            if (hot && p.clicked && unlocked) {
                if (game.BuyInstallation(d.id, n) == BuyResult::Ok) {
                    state.floaters.push_back({"+" + std::to_string(n), p.x, p.y - 8.0f, 0.0f});
                    state.shopFlash = 1.0f;
                    state.shopFlashId = d.id;
                } else {
                    Toast("Need " + FormatExact(cost - wallet.Get(ResourceId::Research)) + " more", 1.4f);
                }
            }
        }
        ui.PopClipRect();

        // --- El indicador de scroll, si la lista es mas larga que el panel ---
        if (contentH > list.height) {
            const float frac = list.height / contentH;
            const float pos = state.shopScroll / (contentH - list.height);
            const float th = std::max(12.0f, list.height * frac);
            ui.AddRect(kit::Rect(shop.Right() - 3.0f, list.y + (list.height - th) * pos, 2, th), kit::Line());
        }

        if (hover && hover->unlocked) DrawCard(ui, vis, shop, *hover->def, hoverY);
        else if (hover) DrawLockedCard(ui, vis, shop, *hover->def, hoverY);
    }

private:
    // La ficha de una fila abierta: a la derecha del panel, a la altura de la fila.
    void DrawCard(UiDrawList& ui, const UiRect& vis, const UiRect& shop, const InstallationDef& d, float rowY) const {
        const GameState& game = Session();
        const int owned = game.Owned(d.id);
        const float w = 196.0f;
        std::vector<std::string> lines;
        lines.push_back(d.name);
        const std::vector<std::string> desc = WrapText(d.description, static_cast<int>(w - 14.0f), 1);
        lines.insert(lines.end(), desc.begin(), desc.end());

        const double mine = game.InstallationOutput(d.id);
        const double all = game.PassiveRate();
        const double each = d.output * game.installations().MilestoneMultiplier(std::max(1, owned)) * game.modifiers().yieldMultiplier;
        char b[96];
        const float lineH = static_cast<float>(kGlyphLineHeight);
        float h = 8.0f + static_cast<float>(lines.size()) * lineH + 4.0f;
        h += 4.0f * lineH + 4.0f; // each / producing / next milestone / cost line

        const float x = std::min(shop.Right() + 6.0f, vis.Right() - w - 4.0f);
        const float y = std::clamp(rowY, vis.y + kBarH + 4.0f, vis.Bottom() - kBarH - 4.0f - h);
        ui.AddRect(kit::Rect(x, y, w, h), UiColor::Bytes(4, 4, 7, 248));
        ui.AddRectOutline(kit::Rect(x, y, w, h), ColorOf(d.color), 1.0f);

        float ty = y + 6.0f;
        for (std::size_t i = 0; i < lines.size(); ++i, ty += lineH)
            ui.AddText(x + 7, ty, lines[i], i == 0 ? kit::Text() : kit::TextSoft(), 1);
        ty += 4.0f;

        ui.AddText(x + 7, ty, "EACH   +" + FormatRate(each), kit::Text(), 1);
        ty += lineH;
        std::snprintf(b, sizeof b, "%.1f%%", all > 0.0 ? 100.0 * mine / all : 0.0);
        ui.AddText(x + 7, ty, "OWNED  " + std::to_string(owned) + "  =  " + FormatRate(mine) + "  (" + b + ")", kit::TextSoft(), 1);
        ty += lineH;
        const int next = game.installations().NextMilestone(owned);
        if (next > 0) {
            std::snprintf(b, sizeof b, "NEXT MILESTONE  %d (x%g)", next, game.installations().Milestones().empty() ? 1.0 : MilestoneAt(game, next));
            ui.AddText(x + 7, ty, b, kit::Dim(), 1);
        } else {
            ui.AddText(x + 7, ty, "ALL MILESTONES REACHED", kit::Red(), 1);
        }
        ty += lineH;
        ui.AddText(x + 7, ty, "ERA " + std::to_string(d.era), kit::Faint(), 1);
    }

    static double MilestoneAt(const GameState& game, int count) {
        for (const Milestone& m : game.installations().Milestones())
            if (m.count == count) return m.multiplier;
        return 1.0;
    }

    void DrawLockedCard(UiDrawList& ui, const UiRect& vis, const UiRect& shop, const InstallationDef& d, float rowY) const {
        const GameState& game = Session();
        const TechDef* need = game.tree().Find(d.unlockedBy);
        const float w = 176.0f;
        const std::vector<std::string> lines = WrapText("Research " + (need ? need->name : d.unlockedBy) + " to unlock " + d.name + ".", static_cast<int>(w - 14.0f), 1);
        const float h = 10.0f + static_cast<float>(lines.size()) * static_cast<float>(kGlyphLineHeight);
        const float x = std::min(shop.Right() + 6.0f, vis.Right() - w - 4.0f);
        const float y = std::clamp(rowY, vis.y + kBarH + 4.0f, vis.Bottom() - kBarH - 4.0f - h);
        ui.AddRect(kit::Rect(x, y, w, h), UiColor::Bytes(4, 4, 7, 248));
        ui.AddRectOutline(kit::Rect(x, y, w, h), kit::Faint(), 1.0f);
        float ty = y + 5.0f;
        for (const std::string& l : lines) {
            ui.AddText(x + 7, ty, l, kit::Dim(), 1);
            ty += static_cast<float>(kGlyphLineHeight);
        }
    }
};

STRACE_REGISTER_SCRIPT(ShopPanel)
