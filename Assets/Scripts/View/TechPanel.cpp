// TechPanel: el panel del arbol de tecnologias. Dibuja lo que hay en el arbol de
// la sesion y le pide UNA cosa: investigar la tecnologia elegida.
//
// No contiene reglas. Que una tecnologia este bloqueada, cueste tanto o se pueda
// comprar lo contesta Core/Tech.h; aqui solo se pregunta y se pinta. Por eso un
// boton "INVESTIGAR" no puede cobrar de mas ni saltarse un requisito: llama a
// `GameState::Research`, que valida.
//
// EL MAPA. El arbol es un mapa de carriles (View/TechLayout.h): cada rama tiene el
// suyo, de izquierda a derecha, y el mapa crece hacia abajo. Se va ABRIENDO a
// medida que se investiga (`TechTree::IsRevealed`): lo que no esta a un paso de lo
// que ya se tiene, no se ve. Cada nodo es una gema chica, del color de su rama y
// con un signo del efecto (+ clic, ~ pasivo, x multiplicador, > desbloquea): el
// nombre y el costo salen al pasar el puntero, y todo el detalle al elegirlo.
// Se arrastra con el boton izquierdo, o con la rueda y las flechas.
//
// Los colores de rama son de ESTE panel y no de la paleta del logo a proposito:
// con diez ramas en un mismo mapa, distinguirlas es lo que hace que se lea.

#include "Format.h"
#include "TechLayout.h"
#include "UiKit.h"
#include "../Services/Session.h"

#include "Engine/Scripting/IScript.h"
#include "Engine/Scripting/ScriptRegistry.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <unordered_set>

using namespace strace;
using namespace evergrave;

namespace {

constexpr float kBarH = 28.0f;
constexpr float kDetailsW = 190.0f;
constexpr float kHeaderH = 14.0f;     // la tira con los nombres de las ramas
constexpr float kScrollStep = 32.0f;  // una fila
constexpr float kDragSlop = 4.0f;     // menos que esto es un clic; mas, un arrastre
constexpr float kRevealSeconds = 0.45f;

UiColor Scale(const UiColor& c, float k) { return UiColor{c.r * k, c.g * k, c.b * k, c.a}; }
UiColor Alpha(const UiColor& c, float a) { return UiColor{c.r, c.g, c.b, c.a * a}; }

// El color de cada rama. Una que la hoja estrene sale gris hasta que se le elija uno.
UiColor BranchColor(const std::string& branch) {
    if (branch == "Instruments") return UiColor::Bytes(110, 165, 255);
    if (branch == "Materials") return UiColor::Bytes(228, 168, 66);
    if (branch == "Power") return UiColor::Bytes(244, 218, 84);
    if (branch == "Solar") return UiColor::Bytes(255, 134, 58);
    if (branch == "Optics") return UiColor::Bytes(84, 222, 232);
    if (branch == "Lasers") return UiColor::Bytes(255, 76, 110);
    if (branch == "Orbit") return UiColor::Bytes(172, 128, 255);
    if (branch == "Logistics") return UiColor::Bytes(98, 212, 120);
    if (branch == "Mining") return UiColor::Bytes(206, 124, 98);
    if (branch == "Industrial") return UiColor::Bytes(204, 204, 214);
    return UiColor::Bytes(160, 160, 170);
}

// Cuatro letras para la cabecera del carril.
std::string LaneTag(const std::string& branch) {
    std::string s = branch.substr(0, 4);
    for (char& c : s)
        if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
    return s;
}

const char* EraName(int era) {
    static const char* names[] = {"EARLY ENERGY", "LASER R&D", "INDUSTRIAL SCALE", "ADVANCED INSTALLATIONS",
                                  "SOLAR SYSTEM", "LUNAR SETTLEMENT", "LUNAR INDUSTRY", "SOLAR ENGINEERING"};
    return era >= 1 && era <= 8 ? names[era - 1] : "";
}

// El signo del primer efecto: lo unico que hay que saber de un vistazo.
const char* EffectGlyph(const TechDef& def) {
    if (def.effects.empty()) return "?";
    switch (def.effects.front().kind) {
        case EffectKind::ClickPowerAdd: return "+";
        case EffectKind::PassiveAdd: return "~";
        case EffectKind::YieldMultiplier: return "x";
        case EffectKind::UnlockInteraction: return ">";
    }
    return "?";
}

// El motor no recorta las lineas contra el recorte activo (un trozo que se sale del
// lienzo se dibuja igual), asi que se recortan aqui: contra arriba y abajo, que es por
// donde se sale el mapa al desplazarse.
void ClippedLine(UiDrawList& ui, const UiRect& view, float x0, float y0, float x1, float y1, const UiColor& c) {
    float t0 = 0.0f, t1 = 1.0f;
    const float dy = y1 - y0;
    if (std::fabs(dy) < 1e-6f) {
        if (y0 < view.y || y0 > view.Bottom()) return;
    } else {
        float ta = (view.y - y0) / dy, tb = (view.Bottom() - y0) / dy;
        if (ta > tb) std::swap(ta, tb);
        t0 = std::max(t0, ta);
        t1 = std::min(t1, tb);
        if (t0 >= t1) return;
    }
    ui.AddLine(x0 + (x1 - x0) * t0, y0 + dy * t0, x0 + (x1 - x0) * t1, y0 + dy * t1, c, 1.0f);
}

// Una flecha entre dos nodos: una S con tangente vertical en los dos extremos (sale
// del padre hacia abajo y llega al hijo desde arriba). En el mismo carril es recta.
void DrawEdge(UiDrawList& ui, const UiRect& view, float x0, float y0, float x1, float y1, const UiColor& c) {
    if (std::fabs(x1 - x0) < 0.5f) {
        ClippedLine(ui, view, x0, y0, x1, y1, c);
        return;
    }
    const float dy = y1 - y0;
    const int steps = std::clamp(static_cast<int>(std::fabs(dy) / 5.0f) + 6, 8, 28);
    float px = x0, py = y0;
    for (int i = 1; i <= steps; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(steps);
        const float u = 1.0f - t;
        // Cubica con los dos puntos de control sobre la vertical de cada extremo, a media altura.
        const float cy0 = y0 + dy * 0.5f, cy1 = y1 - dy * 0.5f;
        const float x = u * u * u * x0 + 3 * u * u * t * x0 + 3 * u * t * t * x1 + t * t * t * x1;
        const float y = u * u * u * y0 + 3 * u * u * t * cy0 + 3 * u * t * t * cy1 + t * t * t * y1;
        ClippedLine(ui, view, std::floor(px), std::floor(py), std::floor(x), std::floor(y), c);
        px = x;
        py = y;
    }
}

// Un hash barato de dos enteros: de donde caen las estrellas del fondo.
unsigned Hash2(int a, int b) {
    unsigned h = static_cast<unsigned>(a) * 374761393u + static_cast<unsigned>(b) * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

// El mapa se calcula una vez: el arbol no cambia mientras se juega.
const TechMap& MapOf(const TechTree& tree) {
    static TechMap map;
    static std::size_t count = 0;
    if (map.nodes.empty() || count != tree.All().size()) {
        map = BuildTechMap(tree);
        count = tree.All().size();
    }
    return map;
}

} // namespace

class TechPanel : public IScript {
public:
    void OnUpdate(float dt) override {
        UiState& state = Ui();
        GameState& game = Session();
        InputActions& in = InputService::Actions();

        if (in.Pressed("ev_tree")) {
            state.treeOpen = !state.treeOpen;
            if (state.treeOpen) state.treeNeedsCenter = true;
        }
        if (in.Pressed("ev_close") && state.treeOpen) state.treeOpen = false;
        if (!state.treeOpen) {
            state.treeDrag = false;
            return;
        }
        state.treeClock += dt;

        UiDrawList& ui = UiService::Frame();
        // El panel va entre las barras del HUD, que estan ancladas al area
        // visible (ver Hud.cpp): con recorte del pixelado, la imagen entera
        // pondria el panel donde la pantalla no llega.
        const UiRect vis = UiService::VisibleRect();
        const Pointer p = ReadPointer();
        const TechTree& tree = game.tree();
        const TechMap& map = MapOf(tree);
        const std::unordered_set<TechId> seen = tree.RevealedIds(); // lo que el mapa muestra, una vez por frame
        const auto shown = [&](const TechId& id) { return seen.count(id) != 0; };

        // --- Geometria: el panel entre las dos barras del HUD ---
        const UiRect panel = kit::Rect(vis.x + 8, vis.y + kBarH + 4, vis.width - 16, vis.height - 2 * kBarH - 8);
        const UiRect canvas = kit::Rect(panel.x + 8, panel.y + 22, panel.width - 24 - kDetailsW, panel.height - 30);
        const UiRect view = kit::Rect(canvas.x, canvas.y + kHeaderH, canvas.width, canvas.height - kHeaderH);
        const UiRect details = kit::Rect(panel.Right() - kDetailsW - 8, panel.y + 22, kDetailsW, panel.height - 30);

        ui.AddRect(panel, kit::Panel());
        ui.AddRectOutline(panel, kit::Line(), 1.0f);
        ui.AddText(panel.x + 8, panel.y + 8, "TECHNOLOGY TREE", kit::Text(), 1);
        const std::string hint = "[T] / [ESC] close   [DRAG] / [WHEEL] move";
        ui.AddText(panel.Right() - 8.0f - kit::TextW(hint), panel.y + 8, hint, kit::Faint(), 1);

        if (p.Over(panel.x, panel.y, panel.width, panel.height)) state.pointerOverUi = true;

        // --- Que se ve: lo revelado, y su aparicion ---
        auto& reveal = state.treeReveal;
        for (const TechDef& d : tree.All()) {
            if (!shown(d.id)) continue;
            auto it = reveal.find(d.id);
            if (it == reveal.end()) reveal[d.id] = state.treeSeeded ? 0.0f : 1.0f;
            else it->second = std::min(1.0f, it->second + dt / kRevealSeconds);
        }
        state.treeSeeded = true;

        // --- Donde esta la camara ---
        const float maxScroll = std::max(0.0f, map.contentH - view.height);
        // El primer frame del juego aun no tiene area visible (el alto sale negativo): se espera.
        if (state.treeNeedsCenter && view.height > 50.0f) {
            // Se centra en lo mas lejano que ya se tiene o se puede comprar: ahi se esta jugando.
            const MapNode* far = nullptr;
            for (const MapNode& n : map.nodes)
                if (shown(n.id) && tree.StateOf(n.id, game.wallet()) != TechState::Locked && (!far || n.row > far->row)) far = &n;
            state.scrollY = far ? far->cy - view.height * 0.5f : 0.0f;
            state.treeNeedsCenter = false;
        }

        const bool overView = p.Over(view.x, view.y, view.width, view.height);
        if (overView) state.scrollY -= in.WheelDelta() * kScrollStep;
        if (in.Pressed("ev_scroll_up")) state.scrollY -= kScrollStep;
        if (in.Pressed("ev_scroll_down")) state.scrollY += kScrollStep;

        // El arrastre y el clic comparten el boton: si el puntero casi no se movio
        // entre apretar y soltar, fue un clic y elige; si se movio, fue un arrastre.
        if (overView && p.clicked) {
            state.treeDrag = true;
            state.treeDragDist = 0.0f;
        }
        bool released = false;
        if (state.treeDrag) {
            if (in.Held("ev_click")) {
                const float dx = in.CursorDeltaX(), dy = in.CursorDeltaY();
                state.treeDragDist += std::fabs(dx) + std::fabs(dy);
                if (state.treeDragDist >= kDragSlop) state.scrollY -= dy;
            }
            if (in.Released("ev_click")) {
                released = state.treeDragDist < kDragSlop;
                state.treeDrag = false;
            }
        }
        state.scrollY = std::clamp(state.scrollY, 0.0f, maxScroll);

        const float ox = view.x + std::floor((view.width - map.contentW) * 0.5f);
        const float oy = view.y - state.scrollY;

        // --- Lo que esta bajo el puntero ---
        const MapNode* hover = nullptr;
        if (overView && !(state.treeDrag && state.treeDragDist >= kDragSlop)) {
            const MapNode* hit = map.HitTest(p.x - ox, p.y - oy);
            if (hit && shown(hit->id)) hover = hit;
        }
        if (!state.selected.empty() && !tree.Find(state.selected)) state.selected.clear();
        if (released && hover) state.selected = hover->id;

        // --- El fondo ---
        ui.AddRect(canvas, UiColor::Bytes(5, 5, 8));
        ui.PushClipRect(view);

        // Estrellas: una de cada tres celdas de 26 px, fijas al mapa (se mueven con el).
        const int cell = 26;
        const int c0 = static_cast<int>(std::floor(state.scrollY / cell));
        const int c1 = static_cast<int>(std::floor((state.scrollY + view.height) / cell)) + 1;
        for (int cy = c0; cy <= c1; ++cy) {
            for (int cx = 0; cx * cell < view.width; ++cx) {
                const unsigned h = Hash2(cx, cy);
                if (h % 3u != 0u) continue;
                const float sx = view.x + cx * cell + static_cast<float>((h >> 4) % cell);
                const float sy = oy - 0.0f + cy * cell + static_cast<float>((h >> 12) % cell);
                const float tw = 0.5f + 0.5f * std::sin(state.treeClock * 1.3f + static_cast<float>(h & 255u));
                ui.AddRect(kit::Rect(std::floor(sx), std::floor(sy), 1, 1), UiColor::Bytes(200, 200, 230, static_cast<int>(36 + 40 * tw)));
            }
        }

        // Los carriles que ya tienen algo a la vista, y los divisores de era.
        std::vector<char> laneSeen(map.lanes.size(), 0);
        for (const MapNode& n : map.nodes)
            if (shown(n.id)) laneSeen[n.lane] = 1;
        for (std::size_t l = 0; l < map.lanes.size(); ++l) {
            if (!laneSeen[l]) continue;
            const float x = std::floor(ox + (static_cast<float>(l) + 0.5f) * 41.0f);
            ui.AddRect(kit::Rect(x, view.y, 1, view.height), Alpha(BranchColor(map.lanes[l]), 0.10f));
        }
        for (const EraMark& e : map.eras) {
            if (!shown(e.first)) continue;
            const float y = std::floor(oy + e.y);
            if (y < view.y - 12 || y > view.Bottom()) continue;
            ui.AddRect(kit::Rect(view.x, y, view.width, 1), Alpha(kit::Faint(), 0.55f));
            const std::string label = "ERA " + std::to_string(e.era) + " - " + EraName(e.era);
            ui.AddText(view.x + 4, y + 3, label, kit::Dim(), 1);
        }

        // --- Las flechas: debajo de los nodos ---
        for (const MapEdge& e : map.edges) {
            if (!shown(e.parent) || !shown(e.child)) continue;
            const float y0 = oy + e.y0, y1 = oy + e.y1;
            if (std::max(y0, y1) < view.y || std::min(y0, y1) > view.Bottom()) continue;
            const TechDef* child = tree.Find(e.child);
            const float fade = reveal.count(e.child) ? reveal[e.child] : 1.0f;
            UiColor c;
            if (tree.IsResearched(e.child)) c = Alpha(BranchColor(child ? child->branch : ""), 0.95f);
            else if (tree.IsResearched(e.parent)) c = Alpha(kit::Text(), 0.55f);
            else c = Alpha(kit::Faint(), 0.7f);
            DrawEdge(ui, view, ox + e.x0, y0, ox + e.x1, y1, Alpha(c, fade));
        }

        // --- Los nodos ---
        const float pulse = 0.35f + 0.25f * std::sin(state.treeClock * 3.5f);
        for (const MapNode& n : map.nodes) {
            if (!shown(n.id)) continue;
            const float cy = oy + n.cy;
            if (cy < view.y - 16 || cy > view.Bottom() + 16) continue;
            const TechDef* def = tree.Find(n.id);
            if (!def) continue;
            const float t = reveal.count(n.id) ? reveal[n.id] : 1.0f;
            const float grow = 0.35f + 0.65f * (1.0f - (1.0f - t) * (1.0f - t)); // sale rapido, asienta lento
            const float size = std::floor(18.0f * grow);
            const float cx = ox + n.cx;

            const TechState st = tree.StateOf(n.id, game.wallet());
            const bool affordable = game.wallet().CanAfford(def->cost);
            const UiColor bc = BranchColor(def->branch);
            UiColor fill, edge, ink;
            const char* glyph = EffectGlyph(*def);
            switch (st) {
                case TechState::Researched:
                    fill = Scale(bc, 0.62f);
                    edge = bc;
                    ink = UiColor::Bytes(255, 255, 255);
                    break;
                case TechState::Available:
                    fill = UiColor::Bytes(16, 16, 22);
                    edge = affordable ? bc : Scale(bc, 0.5f);
                    ink = affordable ? bc : Scale(bc, 0.5f);
                    if (affordable) kit::Gem(ui, cx, cy, size + 6.0f, UiColor::Bytes(0, 0, 0, 0), Alpha(bc, pulse * t));
                    break;
                case TechState::Locked:
                    fill = UiColor::Bytes(10, 10, 13);
                    edge = kit::Faint();
                    ink = kit::Faint();
                    glyph = "?";
                    break;
            }
            kit::Gem(ui, cx, cy, size, Alpha(fill, t), Alpha(edge, t));
            if (t > 0.6f) ui.AddText(std::floor(cx - 2.0f), std::floor(cy - 3.0f), glyph, ink, 1);

            if (n.id == state.selected) {
                const float r = 12.0f;
                ui.AddRectOutline(kit::Rect(std::floor(cx - r), std::floor(cy - r), r * 2, r * 2), kit::Text(), 1.0f);
            }
        }
        ui.PopClipRect();

        // --- La tira de arriba: las ramas que ya se abrieron ---
        ui.AddRect(kit::Rect(canvas.x, canvas.y, canvas.width, kHeaderH), Alpha(UiColor::Bytes(2, 2, 4), 0.94f));
        ui.AddRect(kit::Rect(canvas.x, canvas.y + kHeaderH - 1, canvas.width, 1), kit::Line());
        for (std::size_t l = 0; l < map.lanes.size(); ++l) {
            if (!laneSeen[l]) continue;
            const std::string tag = LaneTag(map.lanes[l]);
            const float x = ox + (static_cast<float>(l) + 0.5f) * 41.0f;
            ui.AddText(std::floor(x - kit::TextW(tag) * 0.5f), canvas.y + 4, tag, BranchColor(map.lanes[l]), 1);
        }
        ui.AddRectOutline(canvas, kit::Line(), 1.0f);

        // --- El aviso del nodo bajo el puntero ---
        if (hover) DrawHoverTip(ui, canvas, tree.Find(hover->id), ox + hover->cx, oy + hover->cy);

        DrawDetails(ui, details, p);
    }

private:
    // Nombre y costo, junto al nodo; del otro lado si no cabe. No es el panel de la
    // derecha (eso es de lo ELEGIDO): es para recorrer el mapa sin hacer clic en cada nodo.
    void DrawHoverTip(UiDrawList& ui, const UiRect& area, const TechDef* def, float nx, float ny) {
        GameState& game = Session();
        const TechTree& tree = game.tree();
        if (!def) return;
        const TechState st = tree.StateOf(def->id, game.wallet());
        std::string second;
        UiColor secondColor = kit::Text();
        if (st == TechState::Researched) {
            second = "RESEARCHED";
            secondColor = kit::Red();
        } else {
            second = "COST " + FormatExact(def->cost.empty() ? 0.0 : def->cost[0].amount);
            if (st == TechState::Locked) secondColor = kit::Dim();
            else if (!game.wallet().CanAfford(def->cost)) secondColor = kit::Red();
        }
        const float w = std::max(kit::TextW(def->name), kit::TextW(second)) + 10.0f;
        const float h = 2.0f * kGlyphLineHeight + 8.0f;
        float x = nx + 14.0f;
        if (x + w > area.Right() - 2.0f) x = nx - 14.0f - w;
        const float y = std::clamp(ny - h * 0.5f, area.y + kHeaderH + 2.0f, area.Bottom() - h - 2.0f);
        ui.AddRect(kit::Rect(x, y, w, h), UiColor::Bytes(4, 4, 7, 245));
        ui.AddRectOutline(kit::Rect(x, y, w, h), BranchColor(def->branch), 1.0f);
        ui.AddText(x + 5, y + 4, def->name, kit::Text(), 1);
        ui.AddText(x + 5, y + 4 + kGlyphLineHeight, second, secondColor, 1);
    }

    void DrawDetails(UiDrawList& ui, const UiRect& box, const Pointer& p) {
        UiState& state = Ui();
        GameState& game = Session();
        const TechTree& tree = game.tree();

        ui.AddRect(box, kit::Field());
        ui.AddRectOutline(box, kit::Line(), 1.0f);

        const TechDef* def = state.selected.empty() ? nullptr : tree.Find(state.selected);
        if (!def) {
            kit::Wrapped(ui, box.x + 8, box.y + 8, box.width - 16, "Pick a technology on the map to see what it costs and what it does.", kit::Dim());
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
        ui.AddText(x, y, def->branch + " - ERA " + std::to_string(def->era), BranchColor(def->branch), 1);
        y += 10;
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
        // Lo que abre en la tienda: una tecnologia puede desbloquear un tipo de instalacion.
        for (const InstallationDef& b : game.installations().All())
            if (b.unlockedBy == def->id) y = kit::Wrapped(ui, x, y, w, "Unlocks to build: " + b.name, kit::Text());
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
