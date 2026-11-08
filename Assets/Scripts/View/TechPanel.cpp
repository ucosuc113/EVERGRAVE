// TechPanel: el panel de investigacion, a PANTALLA COMPLETA. Dibuja lo que hay en el arbol de la
// sesion y le pide UNA cosa: investigar la tecnologia elegida.
//
// No contiene reglas. Que una tecnologia este bloqueada, cueste tanto o se pueda comprar lo
// contesta Core/Tech.h; aqui solo se pregunta y se pinta. Por eso un boton "RESEARCH" no puede
// cobrar de mas ni saltarse un requisito: llama a `GameState::Research`, que valida.
//
// POR QUE YA NO ES UN MAPA. Era un mapa de carriles con una flecha por requisito: con 260
// tecnologias y 427 requisitos (la mitad cruzando de una rama a otra) eran lineas por todas
// partes, nodos sin nombre (solo un signo: + ~ x >), diez colores a la vez, estrellas que
// titilaban y nodos que pulsaban. Los testers terminaban con dolor de cabeza, y con razon: la
// pregunta que se hace en un clicker es "que compro ahora?", y el mapa la contestaba al ultimo.
//
// AHORA, sobre el modelo de Cell to Singularity (una pantalla entera de tarjetas con nombre,
// agrupadas, y la ficha de lo elegido al lado), pero ordenado por lo que importa:
//  - ARRIBA, el saldo (el HUD se aparta: este panel ocupa todo) y las ramas ya descubiertas como
//    filtro, con cuantas hay listas y cuanto va cada una;
//  - CENTRO, una rejilla de TARJETAS por ESTADO: LISTO PARA INVESTIGAR (primero lo que alcanza,
//    despues lo mas barato), BLOQUEADO (diciendo que falta) e INVESTIGADO (plegado: es lo que mas
//    crece y lo que menos importa). Cada tarjeta dice el nombre, lo que hace y lo que cuesta, sin
//    pasar el puntero por encima de nada;
//  - DERECHA, la ficha de lo elegido, con sus requisitos y lo que abre como enlaces: se navega el
//    arbol de requisitos saltando por ellos, en vez de seguir lineas con la vista. Y en la
//    rejilla, las tarjetas que son requisito o consecuencia de la elegida llevan un borde.
// Lo que se ve sigue la regla de siempre (`TechTree::IsRevealed`): el arbol se va abriendo a
// medida que se investiga.
//
// Quieto y suave a proposito: nada titila ni pulsa, los neutros no son negro ni blanco puros
// (UiKit.h) y los colores de rama son solo la orbita de cada tarjeta. Un panel que se mira minutos
// tiene que descansar la vista; lo unico que se mueve es lo que la persona toca.
//
// Teclas: flechas eligen, Enter investiga, T o Esc cierran. Doble clic investiga de un tiro
// (comprar seguido es lo que se hace mas en este juego).

#include "Format.h"
#include "UiKit.h"
#include "../Services/Session.h"

#include "Engine/Gameplay/GameTime.h"
#include "Engine/Scripting/IScript.h"
#include "Engine/Scripting/ScriptRegistry.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace strace;
using namespace evergrave;

namespace {

constexpr float kMargin = 8.0f;
constexpr float kHeadH = 36.0f;
constexpr float kChipY = 41.0f;     // la fila de ramas, bajo la cabecera
constexpr float kChipH = 18.0f;
constexpr float kFootH = 22.0f;
constexpr float kDetailW = 198.0f;
constexpr float kCardH = 36.0f;
constexpr float kCardGap = 4.0f;
constexpr float kSectionH = 20.0f;
constexpr float kScrollStep = 40.0f;
constexpr float kDoubleClick = 0.35f;

UiColor Alpha(const UiColor& c, float a) { return UiColor{c.r, c.g, c.b, c.a * a}; }

// El color de cada rama, un poco apagado hacia el gris (18%): diez colores a tope a la vez cansaban, y
// ahora solo viven en la orbita de cada tarjeta y en el filtro. Una rama que la hoja estrene sale gris.
UiColor BranchColor(const std::string& branch) {
    UiColor c = UiColor::Bytes(160, 160, 170);
    if (branch == "Instruments") c = UiColor::Bytes(110, 165, 255);
    else if (branch == "Materials") c = UiColor::Bytes(228, 168, 66);
    else if (branch == "Power") c = UiColor::Bytes(244, 218, 84);
    else if (branch == "Solar") c = UiColor::Bytes(255, 134, 58);
    else if (branch == "Optics") c = UiColor::Bytes(84, 222, 232);
    else if (branch == "Lasers") c = UiColor::Bytes(255, 76, 110);
    else if (branch == "Orbit") c = UiColor::Bytes(172, 128, 255);
    else if (branch == "Logistics") c = UiColor::Bytes(98, 212, 120);
    else if (branch == "Mining") c = UiColor::Bytes(206, 124, 98);
    else if (branch == "Industrial") c = UiColor::Bytes(204, 204, 214);
    const float lum = 0.3f * c.r + 0.59f * c.g + 0.11f * c.b;
    const float k = 0.18f;
    return UiColor{c.r + (lum - c.r) * k, c.g + (lum - c.g) * k, c.b + (lum - c.b) * k, 1.0f};
}

// El orden de las ramas en la fila: el de cuando aparecen en una partida, que es el que se lee de
// izquierda a derecha. Una que no este en la lista va al final.
const std::vector<std::string>& BranchOrder() {
    static const std::vector<std::string> order{"Instruments", "Power", "Solar", "Optics", "Lasers",
                                                "Materials", "Industrial", "Logistics", "Orbit", "Mining"};
    return order;
}

const char* EraName(int era) {
    static const char* names[] = {"EARLY ENERGY", "LASER R&D", "INDUSTRIAL SCALE", "ADVANCED INSTALLATIONS",
                                  "SOLAR SYSTEM", "LUNAR SETTLEMENT", "LUNAR INDUSTRY", "SOLAR ENGINEERING"};
    return era >= 1 && era <= 8 ? names[era - 1] : "";
}

// El signo del primer efecto, dentro de la orbita de la tarjeta: una pista de categoria, no la unica
// informacion (el texto de la tarjeta dice lo que hace).
const char* EffectGlyph(const TechDef& def) {
    if (def.effects.empty()) return "?";
    switch (def.effects.front().kind) {
        case EffectKind::ClickPowerAdd: return "+";
        case EffectKind::PassiveAdd: return "~";
        case EffectKind::YieldMultiplier: return "x";
        case EffectKind::UnlockInteraction: return ">";
        case EffectKind::UnlockBody: return "o";
    }
    return "?";
}

// Recorta con puntos suspensivos para que el texto quepa en `w`. Un texto que se sale de su tarjeta
// es peor que uno cortado: tapa a la de al lado.
std::string Fit(const std::string& s, float w) {
    if (kit::TextW(s) <= w) return s;
    std::string t = s;
    while (!t.empty() && kit::TextW(t + "...") > w) t.pop_back();
    return t + "...";
}

// Un parrafo con 2 px mas de interlineado que kit::Wrapped: los 8 px de la fuente pegan una linea a la
// otra, y en una ficha que se lee de corrido eso cansa. Devuelve la Y donde sigue.
float Paragraph(UiDrawList& ui, float x, float y, float width, const std::string& text, const UiColor& color) {
    for (const std::string& line : WrapText(text, static_cast<int>(width), 1)) {
        ui.AddText(x, y, line, color, 1);
        y += static_cast<float>(kGlyphLineHeight) + 2.0f;
    }
    return y;
}

double FirstCost(const TechDef& d) { return d.cost.empty() ? 0.0 : d.cost[0].amount; }

// Que fraccion del costo (del primer recurso, el que importa) ya se tiene.
float CostProgress(const TechDef& d, const Wallet& w) {
    if (d.cost.empty() || d.cost[0].amount <= 0.0) return 1.0f;
    return static_cast<float>(std::clamp(w.Get(d.cost[0].id) / d.cost[0].amount, 0.0, 1.0));
}

// Una barrita de progreso: pista tenue y relleno.
void Bar(UiDrawList& ui, float x, float y, float w, float h, float frac, const UiColor& fill) {
    ui.AddRect(kit::Rect(x, y, w, h), UiColor::Bytes(38, 46, 62));
    const float f = std::floor(w * std::clamp(frac, 0.0f, 1.0f));
    if (f > 0.0f) ui.AddRect(kit::Rect(x, y, f, h), fill);
}

enum class Section { Ready, Locked, Done };

// Una fila de la rejilla: una cabecera de seccion, un aviso o una tarjeta. La Y es dentro del contenido
// (sin desplazamiento) y `col` la columna.
struct Row {
    enum Kind { Header, Note, Tech } kind = Tech;
    Section section = Section::Ready;
    std::string text;
    int count = 0;
    const TechDef* def = nullptr;
    TechState state = TechState::Locked;
    bool afford = false;
    int col = 0;
    float y = 0.0f, h = 0.0f;
};

struct BranchStat {
    std::string name;
    int total = 0, done = 0, ready = 0, shown = 0;
};

// Un boton de la fila de ramas, ya con su rectangulo.
struct Chip {
    std::string key, label;
    UiColor color;
    int ready = 0, done = 0, total = 0;
    UiRect r;
};

} // namespace

class TechPanel : public IScript {
public:
    void OnUpdate(float dt) override {
        if (Ui().menu || Ui().cine) return; // el menu principal esta abierto (o se esta filmando)
        UiState& state = Ui();
        GameState& game = Session();
        InputActions& in = InputService::Actions();

        if (in.Pressed("ev_tree")) state.treeOpen = !state.treeOpen;
        if (in.Pressed("ev_close") && state.treeOpen) state.treeOpen = false;
        if (!state.treeOpen) {
            state.treeClock = 0.0f;
            return;
        }
        if (state.treeClock == 0.0f) state.treeFocus = true; // recien abierto: que la rejilla muestre lo elegido
        state.treeClock += dt;
        state.pointerOverUi = true; // el panel es toda la pantalla: el mundo de detras no recibe nada

        UiDrawList& ui = UiService::Frame();
        // El contenido va dentro del AREA VISIBLE (ver Hud.cpp): con recorte del pixelado, la imagen entera
        // pondria las orillas donde la pantalla no llega. El fondo si cubre la imagen entera.
        const UiRect vis = UiService::VisibleRect();
        const Pointer p = ReadPointer();
        const TechTree& tree = game.tree();
        const Wallet& wallet = game.wallet();
        const std::unordered_set<TechId> seen = tree.RevealedIds(); // lo que se ve, una vez por frame
        const auto shown = [&](const TechId& id) { return seen.count(id) != 0; };

        const UiRect screen = kit::Rect(0, 0, UiService::Width(), UiService::Height());

        // --- Las ramas que ya se ven, con su cuenta ---
        std::vector<BranchStat> branches;
        {
            std::unordered_map<std::string, std::size_t> at;
            auto stat = [&](const std::string& name) -> BranchStat& {
                auto it = at.find(name);
                if (it != at.end()) return branches[it->second];
                at[name] = branches.size();
                branches.push_back({name});
                return branches.back();
            };
            for (const TechDef& d : tree.All()) {
                BranchStat& b = stat(d.branch);
                ++b.total;
                if (tree.IsResearched(d.id)) ++b.done;
                if (!shown(d.id)) continue;
                ++b.shown;
                if (tree.StateOf(d.id, wallet) == TechState::Available) ++b.ready;
            }
            std::stable_sort(branches.begin(), branches.end(), [](const BranchStat& a, const BranchStat& b) {
                const auto& o = BranchOrder();
                const auto ia = std::find(o.begin(), o.end(), a.name), ib = std::find(o.begin(), o.end(), b.name);
                return (ia - o.begin()) < (ib - o.begin());
            });
            branches.erase(std::remove_if(branches.begin(), branches.end(), [](const BranchStat& b) { return b.shown == 0; }), branches.end());
        }
        int readyTotal = 0;
        for (const BranchStat& b : branches) readyTotal += b.ready;
        // Un filtro de una rama que ya no esta (partida cargada) se suelta.
        if (!state.treeBranch.empty() &&
            std::none_of(branches.begin(), branches.end(), [&](const BranchStat& b) { return b.name == state.treeBranch; }))
            state.treeBranch.clear();

        // --- Geometria: la fila de ramas decide donde empieza la rejilla (puede ocupar dos filas) ---
        float chipsBottom = 0.0f;
        const std::vector<Chip> chips = LayoutChips(vis, branches, readyTotal, tree.ResearchedCount(), static_cast<int>(tree.All().size()), chipsBottom);
        const float bodyY = chipsBottom + 6.0f - vis.y;
        const UiRect det = kit::Rect(vis.Right() - kMargin - kDetailW, vis.y + bodyY, kDetailW, vis.height - bodyY - kFootH - 6.0f);
        const UiRect grid = kit::Rect(vis.x + kMargin, vis.y + bodyY, det.x - kMargin * 0.75f - (vis.x + kMargin), det.height);

        // --- Lo que lista la rejilla: por estado, y filtrado por la rama elegida ---
        std::vector<const TechDef*> ready, locked, done;
        for (const TechDef& d : tree.All()) {
            if (!shown(d.id)) continue;
            if (!state.treeBranch.empty() && d.branch != state.treeBranch) continue;
            switch (tree.StateOf(d.id, wallet)) {
                case TechState::Available: ready.push_back(&d); break;
                case TechState::Locked: locked.push_back(&d); break;
                case TechState::Researched: done.push_back(&d); break;
            }
        }
        // Lo que alcanza, primero; despues lo mas barato (lo que se va a comprar pronto).
        std::stable_sort(ready.begin(), ready.end(), [&](const TechDef* a, const TechDef* b) {
            const bool aa = wallet.CanAfford(a->cost), bb = wallet.CanAfford(b->cost);
            if (aa != bb) return aa;
            return FirstCost(*a) < FirstCost(*b);
        });
        std::stable_sort(locked.begin(), locked.end(), [](const TechDef* a, const TechDef* b) { return FirstCost(*a) < FirstCost(*b); });
        std::reverse(done.begin(), done.end()); // lo ultimo investigado, primero

        const float innerW = grid.width - 16.0f;
        const int cols = innerW >= 360.0f ? 2 : 1;
        const float cardW = std::floor((innerW - kCardGap * static_cast<float>(cols - 1)) / static_cast<float>(cols));

        std::vector<Row> rows;
        float y = 0.0f;
        auto addHeader = [&](Section s, const std::string& text, int count) {
            Row r;
            r.kind = Row::Header;
            r.section = s;
            r.text = text;
            r.count = count;
            r.y = y;
            r.h = kSectionH;
            y += r.h;
            rows.push_back(r);
        };
        auto addNote = [&](const std::string& text) {
            Row r;
            r.kind = Row::Note;
            r.text = text;
            r.y = y;
            r.h = 24.0f;
            y += r.h;
            rows.push_back(r);
        };
        auto addTechs = [&](Section s, const std::vector<const TechDef*>& v) {
            int i = 0;
            for (const TechDef* d : v) {
                Row r;
                r.kind = Row::Tech;
                r.section = s;
                r.def = d;
                r.state = tree.StateOf(d->id, wallet);
                r.afford = r.state == TechState::Available && wallet.CanAfford(d->cost);
                r.col = i % cols;
                r.y = y;
                r.h = kCardH;
                rows.push_back(r);
                if (r.col == cols - 1) y += kCardH + kCardGap;
                ++i;
            }
            if (i % cols != 0) y += kCardH + kCardGap;
        };
        addHeader(Section::Ready, "READY TO RESEARCH", static_cast<int>(ready.size()));
        if (ready.empty()) addNote(locked.empty() && done.empty() ? "Nothing here yet." : "Nothing ready. Earn more points or unlock a requirement.");
        addTechs(Section::Ready, ready);
        if (!locked.empty()) {
            y += 6.0f;
            addHeader(Section::Locked, "LOCKED", static_cast<int>(locked.size()));
            addTechs(Section::Locked, locked);
        }
        if (!done.empty()) {
            y += 6.0f;
            addHeader(Section::Done, state.treeShowDone ? "RESEARCHED  [click to hide]" : "RESEARCHED  [click to show]", static_cast<int>(done.size()));
            if (state.treeShowDone) addTechs(Section::Done, done);
        }
        const float contentH = y + 4.0f;
        state.treeListH = contentH;

        std::vector<const Row*> pickable; // el orden en que las flechas recorren la rejilla
        for (const Row& r : rows)
            if (r.kind == Row::Tech) pickable.push_back(&r);

        // --- Que esta elegido ---
        auto select = [&](const TechId& id) {
            if (state.selected != id) state.treeDetailScroll = 0.0f;
            state.selected = id;
            state.treeFocus = true;
        };
        if (!state.selected.empty() && !tree.Find(state.selected)) state.selected.clear();
        if (state.selected.empty() && !pickable.empty()) select(pickable.front()->def->id); // el panel abre con algo que mirar

        int step = 0;
        if (in.Pressed("ev_scroll_up")) step = -cols;
        if (in.Pressed("ev_scroll_down")) step = cols;
        if (in.Pressed("ev_tree_left")) step = -1;
        if (in.Pressed("ev_tree_right")) step = 1;
        if (step != 0 && !pickable.empty()) {
            int at = -1;
            for (std::size_t i = 0; i < pickable.size(); ++i)
                if (pickable[i]->def->id == state.selected) at = static_cast<int>(i);
            const int next = std::clamp(at < 0 ? 0 : at + step, 0, static_cast<int>(pickable.size()) - 1);
            select(pickable[static_cast<std::size_t>(next)]->def->id);
        }
        if (in.Pressed("ev_research") && !state.selected.empty()) TryResearch(state.selected);

        // --- Desplazamiento de la rejilla ---
        const bool overGrid = p.Over(grid.x, grid.y, grid.width, grid.height);
        if (overGrid) state.scrollY -= in.WheelDelta() * kScrollStep;
        state.scrollY -= TouchScrollY(grid); // con el dedo: arrastrar la rejilla
        // El primer frame del juego aun no tiene area visible (el alto sale negativo): se espera, o el centrado
        // calcularia con una rejilla de alto negativo y la dejaria desplazada para siempre.
        if (state.treeFocus && grid.height > 50.0f) {
            for (const Row& r : rows) {
                if (r.kind != Row::Tech || r.def->id != state.selected) continue;
                const float top = r.y, bottom = r.y + r.h;
                if (bottom < state.scrollY || top > state.scrollY + grid.height) state.scrollY = top + r.h * 0.5f - grid.height * 0.5f; // fuera de vista: al centro
                else if (top < state.scrollY + 4.0f) state.scrollY = top - 4.0f;
                else if (bottom > state.scrollY + grid.height - 12.0f) state.scrollY = bottom - grid.height + 12.0f;
            }
            state.treeFocus = false;
        }
        state.scrollY = std::clamp(state.scrollY, 0.0f, std::max(0.0f, contentH - grid.height + 8.0f));

        // --- Dibujo, de atras hacia adelante ---
        kit::VGradient(ui, screen, UiColor::Bytes(17, 23, 36), UiColor::Bytes(9, 12, 20), 8.0f);
        DrawHeader(ui, vis, readyTotal, p);
        DrawChips(ui, chips, p);
        DrawGrid(ui, grid, rows, cardW, p, overGrid, select);
        DrawDetail(ui, det, p, in, select);
        DrawFooter(ui, vis);
    }

private:
    // Investigar de verdad: pregunta al Core, que valida y cobra. Devuelve si salio.
    bool TryResearch(const TechId& id) {
        GameState& game = Session();
        const TechDef* def = game.tree().Find(id);
        if (!def) return false;
        const ResearchResult r = game.tree().Check(id, game.wallet());
        if (r == ResearchResult::CannotAfford) {
            const double missing = FirstCost(*def) - game.wallet().Get(def->cost[0].id);
            Toast("Need " + FormatExact(missing) + " more", 1.4f);
            return false;
        }
        if (r != ResearchResult::Ok) return false;
        if (game.Research(id) == ResearchResult::Ok) {
            Toast("Researched: " + def->name);
            return true;
        }
        return false;
    }

    // La cabecera: el saldo (que sigue subiendo con el panel abierto), el titulo y cerrar.
    void DrawHeader(UiDrawList& ui, const UiRect& vis, int ready, const Pointer& p) {
        GameState& game = Session();
        UiState& state = Ui();
        ui.AddRect(kit::Rect(0, vis.y, UiService::Width(), kHeadH), kit::Bar());
        ui.AddRect(kit::Rect(0, vis.y + kHeadH - 1, UiService::Width(), 1), kit::Line().WithAlpha(0.6f));

        const std::string balance = FormatAmount(game.wallet().Get(ResourceId::Research));
        ui.AddText(vis.x + 12, vis.y + 11, balance, kit::Text(), 2);
        const float tx = vis.x + 12.0f + kit::TextW(balance, 2) + 8.0f;
        ui.AddText(tx, vis.y + 9, InfoOf(ResourceId::Research).name, kit::Dim(), 1);
        ui.AddText(tx, vis.y + 20, FormatRate(game.PassiveRate()) + "   gather +" + FormatAmount(game.GatherAmount()), kit::TextSoft(), 1);

        const float cx = vis.x + vis.width * 0.5f;
        ui.AddText(std::floor(cx - kit::TextW("RESEARCH", 2) * 0.5f), vis.y + 11, "RESEARCH", kit::Text(), 2);
        if (ready > 0) {
            const std::string r = std::to_string(ready) + " READY";
            const UiRect chip = kit::Rect(std::floor(cx + kit::TextW("RESEARCH", 2) * 0.5f + 10.0f), vis.y + 10, kit::TextW(r) + 12.0f, 16);
            kit::Card(ui, chip, kit::GoDark(), Alpha(kit::Go(), 0.8f));
            ui.AddText(chip.x + 6, chip.y + 4, r, kit::Go(), 1);
        }

        const UiRect close = kit::Rect(vis.Right() - 12.0f - 96.0f, vis.y + 9, 96, 18);
        if (kit::Button(ui, close, "CLOSE [T]", true, p) && state.treeToggledFrame != GameTime::FrameCount()) {
            state.treeOpen = false;
            state.treeToggledFrame = GameTime::FrameCount(); // y el RESEARCH del HUD, si corre despues, tampoco lo reabre
        }
    }

    // Donde va cada boton de la fila de ramas. "ALL" primero, con el nombre ENTERO de cada rama: si no caben en una
    // fila pasan a una segunda (acortar los nombres los dejaba en "I...", "P...", que no dicen nada). `bottom` es
    // donde acaba la ultima fila, para que la rejilla empiece debajo.
    static std::vector<Chip> LayoutChips(const UiRect& vis, const std::vector<BranchStat>& branches, int readyTotal, int doneTotal, int total, float& bottom) {
        std::vector<Chip> chips;
        chips.push_back({"", "ALL", kit::Text(), readyTotal, doneTotal, total, {}});
        for (const BranchStat& b : branches) chips.push_back({b.name, b.name, BranchColor(b.name), b.ready, b.done, b.total, {}});

        const float gap = 4.0f, avail = vis.width - 2.0f * kMargin;
        float x = vis.x + kMargin, y = vis.y + kChipY;
        for (Chip& c : chips) {
            const float w = 12.0f + (c.key.empty() ? 0.0f : 8.0f) + kit::TextW(c.label) + (c.ready > 0 ? 8.0f + kit::TextW(std::to_string(c.ready)) : 0.0f);
            if (x > vis.x + kMargin && x + w > vis.x + kMargin + avail) {
                x = vis.x + kMargin;
                y += kChipH + 4.0f;
            }
            c.r = kit::Rect(x, y, w, kChipH);
            x += w + gap;
        }
        bottom = y + kChipH;
        return chips;
    }

    // La fila de ramas: filtro y progreso.
    void DrawChips(UiDrawList& ui, const std::vector<Chip>& chips, const Pointer& p) {
        UiState& state = Ui();
        for (const Chip& e : chips) {
            const UiRect& r = e.r;
            const bool sel = state.treeBranch == e.key;
            const bool hot = p.Over(r.x, r.y, r.width, r.height);
            kit::Card(ui, r, sel ? UiColor::Bytes(46, 32, 42) : hot ? UiColor::Bytes(32, 42, 60) : UiColor::Bytes(22, 30, 45), sel ? Alpha(kit::Red(), 0.9f) : UiColor{0, 0, 0, 0});
            float tx = r.x + 6.0f;
            if (!e.key.empty()) {
                ui.AddRect(kit::Rect(tx, r.y + 5, 4, 4), e.color);
                tx += 8.0f;
            }
            ui.AddText(tx, r.y + 3, e.label, sel || hot ? kit::Text() : kit::TextSoft(), 1);
            if (e.ready > 0) {
                const std::string n = std::to_string(e.ready);
                ui.AddText(r.Right() - 6.0f - kit::TextW(n), r.y + 3, n, kit::Go(), 1);
            }
            Bar(ui, r.x + 5, r.Bottom() - 4.0f, r.width - 10.0f, 2.0f, e.total > 0 ? static_cast<float>(e.done) / static_cast<float>(e.total) : 0.0f,
                Alpha(e.key.empty() ? kit::Dim() : e.color, 0.85f));
            if (hot && p.clicked) {
                state.treeBranch = e.key;
                state.scrollY = 0.0f;
                state.selected.clear(); // se elige la primera de la rejilla nueva
                state.treeFocus = true;
            }
        }
    }

    // La rejilla de tarjetas. `select` es de OnUpdate: asi la eleccion es de un solo sitio.
    template <typename Select>
    void DrawGrid(UiDrawList& ui, const UiRect& grid, const std::vector<Row>& rows, float cardW, const Pointer& p, bool overGrid, Select select) {
        UiState& state = Ui();
        GameState& game = Session();
        const TechTree& tree = game.tree();
        const Wallet& wallet = game.wallet();

        const UiRect inner = kit::Rect(grid.x + 4, grid.y + 2, grid.width - 8 - 4, grid.height - 4);
        ui.PushClipRect(grid);

        // Quien es requisito o consecuencia de lo elegido: sus tarjetas llevan un borde.
        std::unordered_set<TechId> relPre, relNext;
        if (const TechDef* sel = state.selected.empty() ? nullptr : tree.Find(state.selected)) {
            relPre.insert(sel->prerequisites.begin(), sel->prerequisites.end());
            for (const TechDef& t : tree.All())
                if (std::find(t.prerequisites.begin(), t.prerequisites.end(), sel->id) != t.prerequisites.end()) relNext.insert(t.id);
        }

        for (const Row& r : rows) {
            const float y = inner.y + r.y - state.scrollY;
            if (y + r.h < grid.y || y > grid.Bottom()) continue;

            if (r.kind == Row::Header) {
                const UiRect rect = kit::Rect(inner.x, y, inner.width, r.h);
                const std::string title = r.text + "  " + std::to_string(r.count);
                const bool foldable = r.section == Section::Done;
                const bool hot = foldable && overGrid && p.Over(rect.x, rect.y, rect.width, rect.height);
                const UiColor c = r.section == Section::Ready ? kit::Go() : hot ? kit::Text() : kit::Dim();
                ui.AddText(rect.x + 2, rect.y + 6, title, c, 1);
                ui.AddRect(kit::Rect(rect.x + kit::TextW(title) + 8.0f, rect.y + 10, rect.width - kit::TextW(title) - 8.0f, 1), kit::Line().WithAlpha(0.7f));
                if (hot && p.clicked) state.treeShowDone = !state.treeShowDone;
                continue;
            }
            if (r.kind == Row::Note) {
                kit::Wrapped(ui, inner.x + 6, y + 4, inner.width - 12, r.text, kit::Faint());
                continue;
            }

            const TechDef& d = *r.def;
            const UiRect rect = kit::Rect(inner.x + static_cast<float>(r.col) * (cardW + kCardGap), y, cardW, r.h);
            const bool sel = d.id == state.selected;
            const bool hot = overGrid && p.Over(rect.x, rect.y, rect.width, rect.height);
            const UiColor bc = BranchColor(d.branch);
            const bool isDone = r.state == TechState::Researched;
            const bool isLocked = r.state == TechState::Locked;

            // Fondo: lo que alcanza lleva un verde apenas, lo elegido se aclara.
            UiColor fill = UiColor::Bytes(22, 29, 43);
            if (r.afford) fill = UiColor::Bytes(20, 38, 42);
            if (hot) fill = r.afford ? UiColor::Bytes(25, 46, 50) : UiColor::Bytes(29, 38, 56);
            if (sel) fill = UiColor::Bytes(34, 45, 68);
            UiColor edge{0, 0, 0, 0};
            if (relPre.count(d.id)) edge = Alpha(tree.IsResearched(d.id) ? kit::Go() : kit::Warn(), 0.75f);
            else if (relNext.count(d.id)) edge = Alpha(kit::TextSoft(), 0.55f);
            if (sel) edge = Alpha(bc, 0.95f);
            kit::Card(ui, rect, fill, edge);

            // La orbita de la rama: llena si esta hecha, con borde si se puede, apagada si esta bloqueada.
            const float ocx = rect.x + 19.0f, ocy = rect.y + rect.height * 0.5f;
            const char* glyph = isLocked ? "?" : EffectGlyph(d);
            if (isDone) kit::Gem(ui, ocx, ocy, 24, Alpha(bc, 0.55f), Alpha(bc, 0.9f));
            else if (isLocked) kit::Gem(ui, ocx, ocy, 24, UiColor::Bytes(16, 21, 32), Alpha(bc, 0.3f));
            else kit::Gem(ui, ocx, ocy, 24, Alpha(bc, 0.18f), bc);
            ui.AddText(std::floor(ocx - 2.0f), std::floor(ocy - 3.0f), glyph, isDone ? kit::Text() : isLocked ? kit::Faint() : bc, 1);

            // El costo (o que ya esta) va en la SEGUNDA linea, a la derecha: en la primera se comia el nombre
            // ("Antimatter Pro...") y el nombre es lo que se lee primero. Lo bloqueado no lo lleva: su costo esta
            // en la ficha, y la linea entera la necesita para decir que le falta.
            std::string chip;
            UiColor chipInk = kit::Dim(), chipFill = UiColor::Bytes(16, 22, 33), chipEdge{0, 0, 0, 0};
            if (isDone) {
                chip = "DONE";
                chipInk = kit::Go();
            } else if (!isLocked) {
                chip = FormatExact(FirstCost(d));
                if (r.afford) {
                    chipInk = kit::Text();
                    chipFill = kit::GoDark();
                    chipEdge = Alpha(kit::Go(), 0.9f);
                } else {
                    chipInk = kit::Warn();
                }
            }
            const float chipW = chip.empty() ? 0.0f : kit::TextW(chip) + 10.0f;
            const UiRect chipR = kit::Rect(rect.Right() - chipW - 5.0f, rect.y + 18, chipW, 13);
            if (!chip.empty()) {
                kit::Card(ui, chipR, chipFill, chipEdge);
                ui.AddText(chipR.x + 5, chipR.y + 3, chip, chipInk, 1);
            }

            const float tx = rect.x + 38.0f;
            ui.AddText(tx, rect.y + 6, Fit(d.name, rect.Right() - tx - 6.0f), isDone || isLocked ? kit::TextSoft() : kit::Text(), 1);

            // Segunda linea: lo que hace; si esta bloqueada, lo que le falta.
            std::string sub;
            UiColor subInk = kit::Dim();
            if (isLocked) {
                const Blockers b = tree.BlockersOf(d.id, wallet);
                for (const TechId& pre : b.missingPrerequisites) {
                    const TechDef* pd = tree.Find(pre);
                    sub += (sub.empty() ? "needs " : ", ") + (pd ? pd->name : pre);
                }
                for (const UnlockCondition& c : b.unmetConditions) sub += (sub.empty() ? "needs " : ", ") + DescribeCondition(c);
                subInk = Alpha(kit::Warn(), 0.85f);
            } else if (!d.effects.empty()) {
                sub = DescribeEffect(d.effects.front(), &game.interactions());
                // En la tarjeta cada letra cuenta: "/s" en vez de " per second" (la ficha lo dice entero).
                const std::string full = " per second";
                const std::size_t at = sub.find(full);
                if (at != std::string::npos) sub.replace(at, full.size(), "/s");
                if (d.effects.size() > 1) sub += "  (+" + std::to_string(d.effects.size() - 1) + ")";
            }
            ui.AddText(tx, rect.y + 21, Fit(sub, (chip.empty() ? rect.Right() : chipR.x) - tx - 6.0f), subInk, 1);

            // Cuanto falta para pagarla: una linea al pie, solo en lo que se puede investigar y aun no alcanza.
            if (r.state == TechState::Available && !r.afford)
                Bar(ui, rect.x + 4, rect.Bottom() - 3.0f, rect.width - 8.0f, 2.0f, CostProgress(d, wallet), Alpha(bc, 0.8f));

            if (hot && p.clicked) {
                const bool again = state.treeLastClick == d.id && state.treeClock - state.treeLastClickAt < kDoubleClick;
                select(d.id);
                state.treeFocus = false; // ya esta a la vista: no mover la rejilla bajo el puntero
                if (again) TryResearch(d.id);
                state.treeLastClick = d.id;
                state.treeLastClickAt = state.treeClock;
            }
        }
        ui.PopClipRect();

        // El indicador de desplazamiento.
        if (state.treeListH > grid.height) {
            const float frac = grid.height / state.treeListH;
            const float pos = state.scrollY / std::max(1.0f, state.treeListH - grid.height + 8.0f);
            const float th = std::max(14.0f, (grid.height - 4.0f) * frac);
            ui.AddRect(kit::Rect(grid.Right() - 4.0f, grid.y + 2 + (grid.height - 4.0f - th) * pos, 3, th), kit::Faint());
        }
    }

    // La ficha de lo elegido: que es, que hace, cuanto cuesta, que pide y a donde lleva.
    template <typename Select>
    void DrawDetail(UiDrawList& ui, const UiRect& box, const Pointer& p, InputActions& in, Select select) {
        UiState& state = Ui();
        GameState& game = Session();
        const TechTree& tree = game.tree();
        const Wallet& wallet = game.wallet();

        kit::Card(ui, box, UiColor::Bytes(18, 24, 37), Alpha(kit::Line(), 0.8f));

        const TechDef* def = state.selected.empty() ? nullptr : tree.Find(state.selected);
        if (!def) {
            kit::Wrapped(ui, box.x + 10, box.y + 10, box.width - 20, "Pick a technology to see what it costs and what it does.", kit::Dim());
            return;
        }

        const UiRect clip = kit::Rect(box.x + 1, box.y + 1, box.width - 2, box.height - 40);
        const bool overDetail = p.Over(clip.x, clip.y, clip.width, clip.height);
        if (overDetail) state.treeDetailScroll -= in.WheelDelta() * kScrollStep;
        state.treeDetailScroll -= TouchScrollY(clip); // con el dedo: arrastrar el detalle
        state.treeDetailScroll = std::clamp(state.treeDetailScroll, 0.0f, std::max(0.0f, state.treeDetailH - clip.height));

        const float x = box.x + 11.0f, w = box.width - 22.0f - 4.0f;
        const TechState st = tree.StateOf(def->id, wallet);
        const bool afford = wallet.CanAfford(def->cost);
        const UiColor bc = BranchColor(def->branch);
        const float top = box.y + 10.0f;
        float y = top - state.treeDetailScroll;

        // Una linea de enlace (un requisito o lo que abre): se puede hacer clic para saltar a esa tecnologia.
        auto link = [&](const TechDef& t, const std::string& mark, const UiColor& markInk) {
            const UiRect r = kit::Rect(x - 3, y - 2, w + 6, 12);
            const bool hot = overDetail && p.Over(r.x, r.y, r.width, r.height);
            if (hot) kit::Card(ui, r, UiColor::Bytes(32, 42, 62), UiColor{0, 0, 0, 0});
            ui.AddText(x, y, mark, markInk, 1);
            const float mx = x + kit::TextW(mark) + 5.0f;
            ui.AddText(mx, y, Fit(t.name, x + w - mx), hot ? kit::Text() : kit::TextSoft(), 1);
            y += 12.0f;
            if (hot && p.clicked) select(t.id);
        };
        auto section = [&](const char* title) {
            y += 7.0f;
            ui.AddText(x, y, title, kit::Dim(), 1);
            ui.AddRect(kit::Rect(x + kit::TextW(title) + 6.0f, y + 3, w - kit::TextW(title) - 6.0f, 1), kit::Line().WithAlpha(0.7f));
            y += 13.0f;
        };

        ui.PushClipRect(clip);

        // Nombre: grande si cabe en una linea.
        if (kit::TextW(def->name, 2) <= w) {
            ui.AddText(x, y, def->name, kit::Text(), 2);
            y += 18.0f;
        } else {
            y = Paragraph(ui, x, y, w, def->name, kit::Text()) + 2.0f;
        }
        ui.AddRect(kit::Rect(x, y + 2, 4, 4), bc);
        ui.AddText(x + 9, y, Fit(def->branch, w - 9.0f), kit::TextSoft(), 1);
        y += 11.0f;
        ui.AddText(x, y, Fit("ERA " + std::to_string(def->era) + " - " + EraName(def->era), w), kit::Faint(), 1);
        y += 14.0f;

        std::string status;
        UiColor statusInk = kit::Dim();
        if (st == TechState::Researched) { status = "RESEARCHED"; statusInk = kit::Go(); }
        else if (st == TechState::Locked) { status = "LOCKED"; }
        else if (afford) { status = "READY TO RESEARCH"; statusInk = kit::Go(); }
        else { status = "NOT ENOUGH POINTS YET"; statusInk = kit::Warn(); }
        ui.AddText(x, y, status, statusInk, 1);
        y += 15.0f;

        y = Paragraph(ui, x, y, w, def->description, kit::TextSoft()) + 2.0f;

        section("EFFECTS");
        for (const Effect& e : def->effects) {
            ui.AddRect(kit::Rect(x, y + 2, 3, 3), bc);
            y = Paragraph(ui, x + 8, y, w - 8, DescribeEffect(e, &game.interactions()), kit::Text());
        }
        for (const InstallationDef& b : game.installations().All()) {
            if (b.unlockedBy != def->id) continue;
            ui.AddRect(kit::Rect(x, y + 2, 3, 3), kit::Go());
            y = Paragraph(ui, x + 8, y, w - 8, "Unlocks to build: " + b.name, kit::Text());
        }

        section("COST");
        for (const ResourceAmount& c : def->cost) {
            const double have = wallet.Get(c.id);
            const bool ok = have >= c.amount;
            const UiColor ink = st == TechState::Researched ? kit::Dim() : ok ? kit::Go() : kit::Warn();
            // El unico recurso hoy son los puntos de investigacion: su nombre entero ("Energy Research Points") no cabe.
            ui.AddText(x, y, Fit(FormatExact(c.amount) + " " + (c.id == ResourceId::Research ? "points" : InfoOf(c.id).name), w), ink, 1);
            y += 11.0f;
            if (st != TechState::Researched) {
                Bar(ui, x, y, w, 3.0f, static_cast<float>(std::clamp(have / std::max(c.amount, 1e-9), 0.0, 1.0)), ink);
                y += 6.0f;
                ui.AddText(x, y, "you have " + FormatAmount(have), kit::Faint(), 1);
                y += 11.0f;
            }
        }

        if (!def->prerequisites.empty() || !def->conditions.empty()) {
            section("REQUIRES");
            for (const TechId& pre : def->prerequisites) {
                const TechDef* pd = tree.Find(pre);
                if (!pd) continue;
                const bool ok = tree.IsResearched(pre);
                link(*pd, ok ? "[x]" : "[ ]", ok ? kit::Go() : kit::Warn());
            }
            for (const UnlockCondition& c : def->conditions) {
                const bool ok = wallet.Lifetime(c.resource) >= c.lifetimeAtLeast;
                y = Paragraph(ui, x, y, w, std::string(ok ? "[x] " : "[ ] ") + DescribeCondition(c) + " (" + FormatAmount(wallet.Lifetime(c.resource)) + ")",
                              ok ? kit::Go() : kit::Warn());
            }
        }

        // A donde lleva: lo que tiene a esta como requisito. Lo que todavia no se descubrio se cuenta, no se nombra.
        {
            const std::unordered_set<TechId> seen = tree.RevealedIds();
            std::vector<const TechDef*> next;
            int hidden = 0;
            for (const TechDef& t : tree.All()) {
                if (std::find(t.prerequisites.begin(), t.prerequisites.end(), def->id) == t.prerequisites.end()) continue;
                if (seen.count(t.id)) next.push_back(&t);
                else ++hidden;
            }
            if (!next.empty() || hidden > 0) {
                section("LEADS TO");
                for (const TechDef* t : next) {
                    const TechState ts = tree.StateOf(t->id, wallet);
                    link(*t, ts == TechState::Researched ? "[x]" : ts == TechState::Available ? "[>]" : "[ ]",
                         ts == TechState::Researched ? kit::Go() : ts == TechState::Available ? kit::Text() : kit::Faint());
                }
                if (hidden > 0) {
                    ui.AddText(x, y, std::to_string(hidden) + " more, not discovered yet", kit::Faint(), 1);
                    y += 12.0f;
                }
            }
        }
        ui.PopClipRect();
        state.treeDetailH = (y + state.treeDetailScroll) - top + 10.0f;

        // Un indicador de que hay mas debajo.
        if (state.treeDetailH > clip.height)
            ui.AddRect(kit::Rect(box.Right() - 4.0f, clip.y + 2 + (clip.height - 4.0f - 14.0f) * (state.treeDetailScroll / std::max(1.0f, state.treeDetailH - clip.height)), 2, 14), kit::Faint());

        // --- El boton: dice POR QUE no se puede cuando no se puede ---
        const ResearchResult can = tree.Check(def->id, wallet);
        std::string label = "RESEARCH  [ENTER]";
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
        ui.AddRect(kit::Rect(box.x + 1, box.Bottom() - 38, box.width - 2, 1), kit::Line().WithAlpha(0.7f));
        const UiRect btn = kit::Rect(box.x + 9, box.Bottom() - 29, box.width - 18, 21);
        if (kit::Button(ui, btn, label, can == ResearchResult::Ok, p)) TryResearch(def->id);
    }

    // El pie: las teclas, el ultimo aviso (el HUD no se ve: el aviso de "investigado" tiene que salir aqui) y el progreso.
    void DrawFooter(UiDrawList& ui, const UiRect& vis) {
        UiState& state = Ui();
        GameState& game = Session();
        const float y = vis.Bottom() - kFootH;
        ui.AddRect(kit::Rect(0, y, UiService::Width(), kFootH + 4.0f), kit::Bar());
        ui.AddRect(kit::Rect(0, y, UiService::Width(), 1), kit::Line().WithAlpha(0.6f));

        const std::string hint = "[ARROWS] select   [ENTER] research   [DOUBLE CLICK] research   [T] close";
        ui.AddText(vis.x + 12, y + 8, hint, kit::Faint(), 1);
        const std::string progress = "TECH " + std::to_string(game.tree().ResearchedCount()) + "/" + std::to_string(game.tree().All().size());
        ui.AddText(vis.Right() - 12.0f - kit::TextW(progress), y + 8, progress, kit::Dim(), 1);

        if (state.toastTime > 0.0f && !state.toast.empty()) {
            const float a = state.toastTime < 0.5f ? state.toastTime / 0.5f : 1.0f;
            const float w = kit::TextW(state.toast) + 16.0f;
            const UiRect r = kit::Rect(std::floor(vis.x + vis.width * 0.5f - w * 0.5f), y - 22.0f, w, 16);
            kit::Card(ui, r, UiColor::Bytes(8, 11, 18, static_cast<int>(240 * a)), Alpha(kit::Red(), a));
            ui.AddTextCentered(r, state.toast, kit::Text().WithAlpha(a), 1);
        }
    }
};

STRACE_REGISTER_SCRIPT(TechPanel)
