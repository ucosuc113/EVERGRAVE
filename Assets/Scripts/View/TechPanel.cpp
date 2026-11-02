// TechPanel: el panel del arbol de tecnologias. Dibuja lo que hay en el arbol de
// la sesion y le pide UNA cosa: investigar la tecnologia elegida.
//
// No contiene reglas. Que una tecnologia este bloqueada, cueste tanto o se pueda
// comprar lo contesta Core/Tech.h; aqui solo se pregunta y se pinta. Por eso un
// boton "INVESTIGAR" no puede cobrar de mas ni saltarse un requisito: llama a
// `GameState::Research`, que valida.
//
// Que se ve, por nodo: nombre, costo y estado (investigada / disponible y
// alcanza / disponible y no alcanza / bloqueada). Que se ve del elegido (panel
// de la derecha): descripcion, costo contra saldo, cada requisito y condicion
// con su casilla, y los efectos. Las flechas muestran las dependencias.

#include "Format.h"
#include "TechLayout.h"
#include "UiKit.h"
#include "../Services/Session.h"

#include "Engine/Scripting/IScript.h"
#include "Engine/Scripting/ScriptRegistry.h"

#include <algorithm>
#include <cmath>
#include <string>

using namespace strace;
using namespace evergrave;

namespace {

constexpr float kBarH = 28.0f;
constexpr float kDetailsW = 190.0f;
constexpr float kScrollStep = 24.0f;

struct NodeStyle {
    UiColor fill, edge, name, cost;
    std::string costText;
};

NodeStyle StyleOf(const TechDef& def, TechState state, bool affordable) {
    NodeStyle s;
    switch (state) {
        case TechState::Researched:
            s = {kit::RedDark(), kit::Red(), kit::Text(), kit::TextSoft(), "RESEARCHED"};
            break;
        case TechState::Available:
            s = {kit::Field(), affordable ? kit::Text() : kit::Dim(), affordable ? kit::Text() : kit::TextSoft(),
                 affordable ? kit::Text() : kit::Red(), "COST " + FormatExact(def.cost.empty() ? 0.0 : def.cost[0].amount)};
            break;
        case TechState::Locked:
            s = {UiColor::Bytes(14, 14, 17), kit::Faint(), UiColor::Bytes(120, 120, 134), UiColor::Bytes(100, 100, 114),
                 "COST " + FormatExact(def.cost.empty() ? 0.0 : def.cost[0].amount)};
            break;
    }
    return s;
}

} // namespace

class TechPanel : public IScript {
public:
    void OnUpdate(float) override {
        UiState& state = Ui();
        GameState& game = Session();
        InputActions& in = InputService::Actions();

        if (in.Pressed("ev_tree")) state.treeOpen = !state.treeOpen;
        if (in.Pressed("ev_close") && state.treeOpen) state.treeOpen = false;
        if (!state.treeOpen) return;

        UiDrawList& ui = UiService::Frame();
        const float W = UiService::Width(), H = UiService::Height();
        const Pointer p = ReadPointer();
        const TechTree& tree = game.tree();

        // --- Geometria: el panel entre las dos barras del HUD ---
        const UiRect panel = kit::Rect(8, kBarH + 4, W - 16, H - 2 * kBarH - 8);
        const UiRect canvas = kit::Rect(panel.x + 8, panel.y + 22, panel.width - 24 - kDetailsW, panel.height - 30);
        const UiRect details = kit::Rect(panel.Right() - kDetailsW - 8, panel.y + 22, kDetailsW, panel.height - 30);

        ui.AddRect(panel, kit::Panel());
        ui.AddRectOutline(panel, kit::Line(), 1.0f);
        ui.AddText(panel.x + 8, panel.y + 8, "TECHNOLOGY TREE", kit::Text(), 1);
        const std::string hint = "[T] / [ESC] close   [WHEEL] / [UP][DOWN] scroll";
        ui.AddText(panel.Right() - 8.0f - kit::TextW(hint), panel.y + 8, hint, kit::Faint(), 1);

        if (p.Over(panel.x, panel.y, panel.width, panel.height)) state.pointerOverUi = true;

        // --- Desplazamiento ---
        const TreeLayout layout = BuildTreeLayout(tree, canvas.width);
        const float maxScroll = std::max(0.0f, layout.contentHeight - canvas.height);
        if (p.Over(canvas.x, canvas.y, canvas.width, canvas.height)) state.scrollY -= in.WheelDelta() * kScrollStep;
        if (in.Pressed("ev_scroll_up")) state.scrollY -= kScrollStep;
        if (in.Pressed("ev_scroll_down")) state.scrollY += kScrollStep;
        state.scrollY = std::clamp(state.scrollY, 0.0f, maxScroll);

        // --- Seleccion: un clic en un nodo ---
        if (!state.selected.empty() && !tree.Find(state.selected)) state.selected.clear();
        if (p.clicked && canvas.Contains(p.x, p.y)) {
            if (const NodeLayout* hit = layout.HitTest(p.x - canvas.x, p.y - canvas.y + state.scrollY)) state.selected = hit->id;
        }

        // --- El arbol ---
        ui.AddRect(canvas, UiColor::Bytes(6, 6, 9));
        ui.PushClipRect(canvas);
        const float ox = canvas.x, oy = canvas.y - state.scrollY;

        for (const EdgeLayout& e : layout.edges) {
            const bool parentDone = tree.IsResearched(e.parent);
            const bool childDone = tree.IsResearched(e.child);
            const UiColor c = childDone ? kit::Red() : parentDone ? kit::Text() : kit::Faint();
            for (const Segment& s : e.seg) {
                if (s.x0 == s.x1 && s.y0 == s.y1) continue;
                ui.AddLine(ox + s.x0, oy + s.y0, ox + s.x1, oy + s.y1, c, 1.0f);
            }
        }

        for (const NodeLayout& n : layout.nodes) {
            const TechDef* def = tree.Find(n.id);
            if (!def) continue;
            const TechState st = tree.StateOf(n.id, game.wallet());
            const NodeStyle style = StyleOf(*def, st, game.wallet().CanAfford(def->cost));
            const UiRect r = kit::Rect(ox + n.rect.x, oy + n.rect.y, n.rect.w, n.rect.h);
            if (n.id == state.selected) ui.AddRectOutline(kit::Rect(r.x - 2, r.y - 2, r.width + 4, r.height + 4), kit::Text(), 1.0f);
            ui.AddRect(r, style.fill);
            ui.AddRectOutline(r, style.edge, 1.0f);

            // Hasta dos lineas de nombre; el resto del nodo lo ocupa el costo.
            const std::vector<std::string> lines = WrapText(def->name, static_cast<int>(r.width) - 8, 1);
            float ty = r.y + (lines.size() > 1 ? 4.0f : 8.0f);
            for (std::size_t i = 0; i < lines.size() && i < 2; ++i, ty += static_cast<float>(kGlyphLineHeight))
                ui.AddText(r.x + 4, ty, lines[i], style.name, 1);
            ui.AddText(r.x + 4, r.Bottom() - 11.0f, style.costText, style.cost, 1);
        }
        ui.PopClipRect();
        ui.AddRectOutline(canvas, kit::Line(), 1.0f);

        DrawDetails(ui, details, p);
    }

private:
    void DrawDetails(UiDrawList& ui, const UiRect& box, const Pointer& p) {
        UiState& state = Ui();
        GameState& game = Session();
        const TechTree& tree = game.tree();

        ui.AddRect(box, kit::Field());
        ui.AddRectOutline(box, kit::Line(), 1.0f);

        const TechDef* def = state.selected.empty() ? nullptr : tree.Find(state.selected);
        if (!def) {
            kit::Wrapped(ui, box.x + 8, box.y + 8, box.width - 16, "Select a technology to see what it costs and what it does.", kit::Dim());
            return;
        }

        // El texto se recorta por encima del boton: con un arbol de descripciones
        // largas, escribir encima del boton seria peor que cortar la ultima linea.
        ui.PushClipRect(kit::Rect(box.x, box.y, box.width, box.height - 34));
        const float x = box.x + 8, w = box.width - 16;
        const Wallet& wallet = game.wallet();
        const TechState st = tree.StateOf(def->id, wallet);
        float y = box.y + 8;

        y = kit::Wrapped(ui, x, y, w, def->name, kit::Text());
        ui.AddText(x, y, DescribeState(st), st == TechState::Researched ? kit::Red() : st == TechState::Available ? kit::Text() : kit::Dim(), 1);
        y += 14;
        y = kit::Wrapped(ui, x, y, w, def->description, kit::TextSoft()) + 6;

        ui.AddText(x, y, "COST", kit::Dim(), 1);
        y += 10;
        for (const ResourceAmount& c : def->cost) {
            const bool ok = wallet.Get(c.id) >= c.amount;
            y = kit::Wrapped(ui, x, y, w, FormatExact(c.amount) + " " + InfoOf(c.id).name + " (have " + FormatAmount(wallet.Get(c.id)) + ")",
                             ok || st == TechState::Researched ? kit::Text() : kit::Red());
        }
        y += 6;

        if (!def->prerequisites.empty() || !def->conditions.empty()) {
            ui.AddText(x, y, "REQUIRES", kit::Dim(), 1);
            y += 10;
            for (const TechId& pre : def->prerequisites) {
                const TechDef* pd = tree.Find(pre);
                const bool done = tree.IsResearched(pre);
                y = kit::Wrapped(ui, x, y, w, std::string(done ? "[x] " : "[ ] ") + (pd ? pd->name : pre), done ? kit::Text() : kit::Red());
            }
            for (const UnlockCondition& c : def->conditions) {
                const bool done = wallet.Lifetime(c.resource) >= c.lifetimeAtLeast;
                y = kit::Wrapped(ui, x, y, w,
                                 std::string(done ? "[x] " : "[ ] ") + DescribeCondition(c) + " (" + FormatAmount(wallet.Lifetime(c.resource)) + ")",
                                 done ? kit::Text() : kit::Red());
            }
            y += 6;
        }

        ui.AddText(x, y, "EFFECTS", kit::Dim(), 1);
        y += 10;
        for (const Effect& e : def->effects) y = kit::Wrapped(ui, x, y, w, DescribeEffect(e, &game.interactions()), kit::Text());
        ui.PopClipRect();

        // --- El boton: dice POR QUE no se puede cuando no se puede ---
        const ResearchResult can = tree.Check(def->id, wallet);
        std::string label = "RESEARCH";
        switch (can) {
            case ResearchResult::Ok: break;
            case ResearchResult::AlreadyResearched: label = "RESEARCHED"; break;
            case ResearchResult::Locked: label = "LOCKED"; break;
            case ResearchResult::CannotAfford: {
                const double missing = def->cost.empty() ? 0.0 : def->cost[0].amount - wallet.Get(def->cost[0].id);
                label = "NEED " + FormatExact(missing) + " MORE";
                break;
            }
            case ResearchResult::UnknownTech: label = "-"; break;
        }
        const UiRect btn = kit::Rect(box.x + 8, box.Bottom() - 28, box.width - 16, 20);
        if (kit::Button(ui, btn, label, can == ResearchResult::Ok, p)) {
            // Se vuelve a preguntar al Core: el boton solo lo habilita, no decide.
            if (game.Research(def->id) == ResearchResult::Ok) Toast("Researched: " + def->name);
        }
    }
};

STRACE_REGISTER_SCRIPT(TechPanel)
