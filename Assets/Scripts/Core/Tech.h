#pragma once

// EL ARBOL DE TECNOLOGIAS: definiciones (datos) y estado (que se investigo).
//
// Una tecnologia es una fila de datos, no codigo. Agregar una es escribir una
// fila en la hoja Design/EVERGRAVE_Balance.xlsx (que genera Data/TechCatalog.generated.h); ni este archivo ni la interfaz cambian. Lo
// que una tecnologia HACE no esta aqui: lleva una lista de `Effect`, y quien
// los suma es GameState (Core/GameState.h). Asi el arbol decide solo "se puede
// comprar / se compro", y un efecto nuevo es un valor de `EffectKind`, no una
// rama por tecnologia.
//
// Tres decisiones que conviene no deshacer:
//
// - Una tecnologia solo puede nombrar como requisito a otra YA registrada. Asi
//   el grafo es aciclico por construccion (no hace falta buscar ciclos) y un
//   requisito mal escrito falla al registrar, no cuando alguien lo mira.
// - "Disponible" (se cumplen requisitos y condiciones) no es "comprable" (ademas
//   alcanza el saldo). La interfaz tiene que poder decir POR QUE no se puede, y
//   un nodo disponible-pero-caro se ve distinto de uno bloqueado.
// - El estado es solo el conjunto de investigadas. Todo lo demas (efectos
//   totales, disponibilidad) se DERIVA, de modo que guardar una partida es
//   guardar ese conjunto y nada puede quedar a medias entre dos copias.

#include "Resources.h"

#include <algorithm>
#include <string>
#include <unordered_set>
#include <vector>

namespace evergrave {

using TechId = std::string;

// Que cambia una tecnologia en la partida. Al FINAL al agregar: el valor puede
// acabar en un archivo de guardado de alguien.
enum class EffectKind : int {
    ClickPowerAdd,   // suma `value` a lo que da recolectar a mano
    PassiveAdd,      // suma `value` por segundo, sin hacer nada
    YieldMultiplier, // multiplica TODO lo que se gana por `value` (1.25 = +25%)
    UnlockInteraction, // habilita la interaccion `target` (Core/Interactions.h)
};

// `target` nombra a que interaccion se refiere el efecto: para UnlockInteraction
// es obligatoria (cual se habilita); para ClickPowerAdd, vacia = la recoleccion
// basica ("gather") y con id = esa interaccion.
struct Effect {
    EffectKind kind = EffectKind::ClickPowerAdd;
    double value = 0.0;
    std::string target;
};

// Condicion de desbloqueo ademas de los requisitos: haber ganado, en total
// desde siempre, al menos `lifetimeAtLeast` de un recurso. (Ver Wallet::Lifetime
// para por que es lo ganado y no el saldo.)
struct UnlockCondition {
    ResourceId resource = ResourceId::Research;
    double lifetimeAtLeast = 0.0;
};

struct TechDef {
    TechId id;          // unico y estable: es lo que se guarda en disco
    std::string name;   // ASCII: la fuente del motor no tiene acentos
    std::string description;
    std::vector<ResourceAmount> cost;
    std::vector<TechId> prerequisites;
    std::vector<UnlockCondition> conditions;
    std::vector<Effect> effects;
};

enum class TechState {
    Locked,     // falta un requisito o una condicion
    Available,  // se puede investigar (si alcanza el saldo)
    Researched, // ya
};

enum class ResearchResult {
    Ok,
    UnknownTech,
    AlreadyResearched,
    Locked,
    CannotAfford,
};

// Por que una tecnologia esta bloqueada, en datos: la interfaz le pone el texto.
struct Blockers {
    std::vector<TechId> missingPrerequisites;
    std::vector<UnlockCondition> unmetConditions;
    bool Any() const { return !missingPrerequisites.empty() || !unmetConditions.empty(); }
};

class TechTree {
public:
    // Registra una tecnologia. Devuelve false (y no registra nada) si el id esta
    // vacio o repetido, o si un requisito no esta registrado todavia.
    bool Add(TechDef def) {
        if (def.id.empty() || Find(def.id)) return false;
        for (const TechId& p : def.prerequisites)
            if (!Find(p)) return false;
        m_defs.push_back(std::move(def));
        return true;
    }

    // En el orden en que se registraron (que siempre pone los requisitos antes).
    const std::vector<TechDef>& All() const { return m_defs; }

    const TechDef* Find(const TechId& id) const {
        for (const TechDef& d : m_defs)
            if (d.id == id) return &d;
        return nullptr;
    }

    bool IsResearched(const TechId& id) const { return m_researched.count(id) != 0; }
    int ResearchedCount() const { return static_cast<int>(m_researched.size()); }

    Blockers BlockersOf(const TechId& id, const Wallet& wallet) const {
        Blockers b;
        const TechDef* d = Find(id);
        if (!d) return b;
        for (const TechId& p : d->prerequisites)
            if (!IsResearched(p)) b.missingPrerequisites.push_back(p);
        for (const UnlockCondition& c : d->conditions)
            if (wallet.Lifetime(c.resource) < c.lifetimeAtLeast) b.unmetConditions.push_back(c);
        return b;
    }

    TechState StateOf(const TechId& id, const Wallet& wallet) const {
        if (IsResearched(id)) return TechState::Researched;
        if (!Find(id)) return TechState::Locked;
        return BlockersOf(id, wallet).Any() ? TechState::Locked : TechState::Available;
    }

    // Todo lo que hace falta para que esto se pueda comprar AHORA.
    ResearchResult Check(const TechId& id, const Wallet& wallet) const {
        const TechDef* d = Find(id);
        if (!d) return ResearchResult::UnknownTech;
        if (IsResearched(id)) return ResearchResult::AlreadyResearched;
        if (BlockersOf(id, wallet).Any()) return ResearchResult::Locked;
        if (!wallet.CanAfford(d->cost)) return ResearchResult::CannotAfford;
        return ResearchResult::Ok;
    }

    // Investiga: valida, descuenta y marca, en ese orden y todo o nada. Si algo
    // falla no se toca el saldo.
    ResearchResult Research(const TechId& id, Wallet& wallet) {
        const ResearchResult r = Check(id, wallet);
        if (r != ResearchResult::Ok) return r;
        wallet.TrySpend(Find(id)->cost); // Check ya comprobo el saldo
        m_researched.insert(id);
        return ResearchResult::Ok;
    }

    // Cargar una partida: marca como investigadas las ids dadas SIN cobrar (ya
    // se pagaron), pero solo las coherentes. Se recorre en el orden del
    // registro, asi que una tecnologia cuyo requisito no vino (un guardado
    // editado, o un catalogo que cambio) se descarta en vez de quedar
    // investigada sobre un requisito que no lo esta. Devuelve cuantas descarto.
    int RestoreResearched(const std::vector<TechId>& ids) {
        m_researched.clear();
        int dropped = 0;
        std::unordered_set<TechId> wanted(ids.begin(), ids.end());
        for (const TechDef& d : m_defs) {
            if (!wanted.erase(d.id)) continue;
            const bool ok = std::all_of(d.prerequisites.begin(), d.prerequisites.end(),
                                        [&](const TechId& p) { return IsResearched(p); });
            if (ok) m_researched.insert(d.id);
            else ++dropped;
        }
        return dropped + static_cast<int>(wanted.size()); // lo que sobra son ids que ya no existen
    }

    void ClearResearched() { m_researched.clear(); }

    // Las investigadas en el orden del registro (para guardar de forma estable).
    std::vector<TechId> ResearchedIds() const {
        std::vector<TechId> out;
        for (const TechDef& d : m_defs)
            if (IsResearched(d.id)) out.push_back(d.id);
        return out;
    }

private:
    std::vector<TechDef> m_defs;
    std::unordered_set<TechId> m_researched;
};

} // namespace evergrave
