#pragma once

// EL ESTADO DE LA PARTIDA: la cartera, el arbol, las interacciones y lo que
// producen juntos.
//
// Es el unico sitio donde un efecto de tecnologia se convierte en numeros o en
// una interaccion desbloqueada. Los `Modifiers` y el conjunto de desbloqueadas
// se DERIVAN de las tecnologias investigadas cada vez que cambia el conjunto
// (Recompute): no se guardan ni se van sumando a mano. Una tecnologia que "suma
// +1" aplicada dos veces por un bug de carga no existe, porque nadie suma: se
// recalcula desde cero.
//
// No conoce la interfaz ni el motor. La interfaz y el mundo lo LEEN (saldo,
// estado de cada tecnologia, que esta desbloqueado, produccion) y le piden dos
// cosas: Use(interaccion) y Research(tecnologia). Todo lo que gana o cobra
// pasa por ahi, y por eso un clic que el mundo da por bueno y uno que el Core
// rechaza (bloqueado, en espera) no pueden diferir.

#include "Interactions.h"
#include "Resources.h"
#include "Tech.h"

#include <concepts>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace evergrave {

// Lo que la partida produce, ya con las tecnologias aplicadas.
struct Modifiers {
    double passivePerSecond = 0.0;  // por segundo, antes del multiplicador
    double yieldMultiplier = 1.0;   // sobre todo lo que se gana
    // Lo que se suma al rendimiento base de cada interaccion (ClickPowerAdd).
    std::unordered_map<InteractionId, double> yieldAdd;
};

// La recoleccion basica: el clic sobre la Tierra. Existe siempre; GameState la
// usa por nombre para Gather().
inline const InteractionId kGatherId = "gather";

class GameState {
public:
    // El contenido se registra una vez, antes de jugar: `techs(tree)` y
    // `interactions(set)`. El `requires` no es adorno: sin el, esta plantilla
    // gana a la copia cuando se copia un GameState NO const, y falla al compilar
    // de una forma que no dice nada del catalogo.
    template <typename TechFn, typename InteractionFn>
        requires std::invocable<TechFn&, TechTree&> && std::invocable<InteractionFn&, InteractionSet&>
    GameState(TechFn&& techs, InteractionFn&& interactions) {
        techs(m_tree);
        interactions(m_interactions);
        Recompute();
    }

    const Wallet& wallet() const { return m_wallet; }
    const TechTree& tree() const { return m_tree; }
    const InteractionSet& interactions() const { return m_interactions; }
    const Modifiers& modifiers() const { return m_mods; }

    // --- Interacciones ---

    bool IsUnlocked(const InteractionId& id) const { return m_unlocked.count(id) != 0; }

    // Cuanto da UN uso, ya con efectos y multiplicador: lo que la interfaz
    // muestra y lo que se gana de verdad son la misma cuenta.
    double YieldOf(const InteractionId& id) const {
        const InteractionDef* d = m_interactions.Find(id);
        if (!d) return 0.0;
        auto it = m_mods.yieldAdd.find(id);
        return (d->baseYield + (it == m_mods.yieldAdd.end() ? 0.0 : it->second)) * m_mods.yieldMultiplier;
    }
    double GatherAmount() const { return YieldOf(kGatherId); }
    double PassiveRate() const { return m_mods.passivePerSecond * m_mods.yieldMultiplier; }

    // Segundos de espera que le quedan (0 = lista).
    double CooldownLeft(const InteractionId& id) const {
        auto it = m_cooldown.find(id);
        return it == m_cooldown.end() ? 0.0 : it->second;
    }

    // Usar una interaccion: valida (existe, esta desbloqueada, no esta en
    // espera), gana lo que rinde y arranca su espera. Es la UNICA puerta por la
    // que un clic produce puntos.
    UseResult Use(const InteractionId& id, double* gained = nullptr) {
        if (gained) *gained = 0.0;
        const InteractionDef* d = m_interactions.Find(id);
        if (!d) return UseResult::Unknown;
        if (!IsUnlocked(id)) return UseResult::Locked;
        if (CooldownLeft(id) > 0.0) return UseResult::Cooling;
        const double a = YieldOf(id);
        m_wallet.Add(ResourceId::Research, a);
        if (d->cooldown > 0.0) m_cooldown[id] = d->cooldown;
        ++m_useCount;
        if (gained) *gained = a;
        return UseResult::Ok;
    }

    // Recolectar a mano (el clic en la Tierra). Devuelve lo ganado.
    double Gather() {
        double a = 0.0;
        Use(kGatherId, &a);
        return a;
    }

    // La tecnologia que desbloquea esa interaccion ("" si ninguna: o viene
    // desbloqueada, o es inalcanzable - ver ContentProblems). La interfaz lo usa
    // para decir QUE hay que investigar cuando se intenta algo bloqueado.
    TechId UnlockedBy(const InteractionId& id) const {
        for (const TechDef& t : m_tree.All())
            for (const Effect& e : t.effects)
                if (e.kind == EffectKind::UnlockInteraction && e.target == id) return t.id;
        return {};
    }

    // Avanza la produccion pasiva y las esperas `dt` segundos de juego.
    void Tick(double dt) {
        if (!(dt > 0.0)) return;
        const double rate = PassiveRate();
        if (rate > 0.0) m_wallet.Add(ResourceId::Research, rate * dt);
        for (auto& [id, left] : m_cooldown) left = left > dt ? left - dt : 0.0;
        m_elapsed += dt;
    }

    ResearchResult Research(const TechId& id) {
        const ResearchResult r = m_tree.Research(id, m_wallet);
        if (r == ResearchResult::Ok) Recompute();
        return r;
    }

    double Elapsed() const { return m_elapsed; }
    std::uint64_t UseCount() const { return m_useCount; }

    // Incoherencias del contenido, en texto: un efecto que desbloquea una
    // interaccion que no existe, o una interaccion que ninguna tecnologia
    // desbloquea y no viene abierta (inalcanzable). Vacio = bien. Lo comprueba
    // una prueba, para que un error de catalogo salte al compilar y no cuando
    // alguien intenta desbloquear algo que nunca se abre.
    std::vector<std::string> ContentProblems() const {
        std::vector<std::string> out;
        if (!m_interactions.Find(kGatherId)) out.push_back("falta la interaccion base '" + kGatherId + "'");
        for (const TechDef& t : m_tree.All()) {
            for (const Effect& e : t.effects) {
                if (e.kind == EffectKind::UnlockInteraction && !m_interactions.Find(e.target))
                    out.push_back("la tecnologia '" + t.id + "' desbloquea '" + e.target + "', que no existe");
                if (e.kind == EffectKind::ClickPowerAdd && !e.target.empty() && !m_interactions.Find(e.target))
                    out.push_back("la tecnologia '" + t.id + "' mejora '" + e.target + "', que no existe");
            }
        }
        for (const InteractionDef& d : m_interactions.All())
            if (!d.unlockedAtStart && UnlockedBy(d.id).empty())
                out.push_back("la interaccion '" + d.id + "' no se desbloquea nunca");
        return out;
    }

    // --- Para cargar una partida (Core/SaveData.h) ---
    Wallet& MutableWallet() { return m_wallet; }
    TechTree& MutableTree() { return m_tree; }
    void RestoreProgress(double elapsed, std::uint64_t uses) {
        m_elapsed = elapsed > 0.0 ? elapsed : 0.0;
        m_useCount = uses;
        m_cooldown.clear();
        Recompute();
    }

    // Vuelve a derivar modificadores e interacciones del conjunto de investigadas.
    void Recompute() {
        Modifiers m;
        std::unordered_set<InteractionId> unlocked;
        for (const InteractionDef& d : m_interactions.All())
            if (d.unlockedAtStart) unlocked.insert(d.id);

        for (const TechId& id : m_tree.ResearchedIds()) {
            const TechDef* d = m_tree.Find(id);
            if (!d) continue;
            for (const Effect& e : d->effects) {
                switch (e.kind) {
                    case EffectKind::ClickPowerAdd: m.yieldAdd[e.target.empty() ? kGatherId : e.target] += e.value; break;
                    case EffectKind::PassiveAdd: m.passivePerSecond += e.value; break;
                    case EffectKind::YieldMultiplier: m.yieldMultiplier *= e.value; break;
                    case EffectKind::UnlockInteraction: unlocked.insert(e.target); break;
                }
            }
        }
        m_mods = std::move(m);
        m_unlocked = std::move(unlocked);
    }

private:
    Wallet m_wallet;
    TechTree m_tree;
    InteractionSet m_interactions;
    Modifiers m_mods;
    std::unordered_set<InteractionId> m_unlocked;
    std::unordered_map<InteractionId, double> m_cooldown;
    double m_elapsed = 0.0;
    std::uint64_t m_useCount = 0;
};

} // namespace evergrave
