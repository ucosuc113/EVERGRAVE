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
// Las instalaciones (lo que se compra una y otra vez, Core/Installations.h) suman a la
// produccion pasiva y se multiplican por lo mismo que todo lo demas: la cuenta es
// (pasivo de tecnologias + lo que producen las instalaciones) x multiplicador.
//
// No conoce la interfaz ni el motor. La interfaz y el mundo lo LEEN (saldo,
// estado de cada tecnologia, que esta desbloqueado, produccion) y le piden dos
// cosas: Use(interaccion) y Research(tecnologia). Todo lo que gana o cobra
// pasa por ahi, y por eso un clic que el mundo da por bueno y uno que el Core
// rechaza (bloqueado, en espera) no pueden diferir.

#include "Bodies.h"
#include "Installations.h"
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

    // Igual que la de arriba, con las instalaciones: `installations(set)` las registra
    // (y sus hitos). Una partida sin ellas es la otra: las pruebas de arbol e
    // interacciones no tienen por que nombrarlas.
    template <typename TechFn, typename InteractionFn, typename InstallationFn>
        requires std::invocable<TechFn&, TechTree&> && std::invocable<InteractionFn&, InteractionSet&> &&
                 std::invocable<InstallationFn&, InstallationSet&>
    GameState(TechFn&& techs, InteractionFn&& interactions, InstallationFn&& installations) {
        techs(m_tree);
        interactions(m_interactions);
        installations(m_installations);
        Recompute();
    }

    const Wallet& wallet() const { return m_wallet; }
    const TechTree& tree() const { return m_tree; }
    const InteractionSet& interactions() const { return m_interactions; }
    const InstallationSet& installations() const { return m_installations; }
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
    double PassiveRate() const { return (m_mods.passivePerSecond + m_installRate) * m_mods.yieldMultiplier; }

    // --- Cuerpos celestes ---

    // La Tierra siempre esta abierta; los demas, cuando una tecnologia investigada los abre.
    bool IsBodyUnlocked(BodyId b) const { return b == BodyId::Earth || m_bodies[static_cast<int>(b)]; }
    // El mapa del sistema (la vista de arriba, desde la que se elige cuerpo).
    bool IsMapUnlocked() const { return m_mapUnlocked; }
    // Cuantas instalaciones de ese cuerpo hay compradas.
    int OwnedOn(BodyId b) const {
        int n = 0;
        for (const InstallationDef& d : m_installations.All())
            if (d.body == b) n += Owned(d.id);
        return n;
    }

    // --- Instalaciones ---

    int Owned(const InstallationId& id) const {
        auto it = m_owned.find(id);
        return it == m_owned.end() ? 0 : it->second;
    }
    int TotalOwned() const {
        int n = 0;
        for (const auto& kv : m_owned) n += kv.second;
        return n;
    }

    // Si ya se puede comprar: su tecnologia esta investigada (o no pide ninguna).
    bool IsInstallationUnlocked(const InstallationId& id) const {
        const InstallationDef* d = m_installations.Find(id);
        return d && (d->unlockedBy.empty() || m_tree.IsResearched(d->unlockedBy));
    }

    // Lo que cuesta comprar `n` mas (la proxima si n = 1).
    double CostOfNext(const InstallationId& id, int n = 1) const {
        const InstallationDef* d = m_installations.Find(id);
        return d ? CostOfUnits(*d, Owned(id), n) : 0.0;
    }
    int MaxAffordable(const InstallationId& id) const {
        const InstallationDef* d = m_installations.Find(id);
        return d && IsInstallationUnlocked(id) ? MaxAffordableUnits(*d, Owned(id), m_wallet.Get(ResourceId::Research)) : 0;
    }

    // Por segundo, ya con hitos y multiplicador: lo que muestra la tienda y lo que se gana.
    double InstallationOutput(const InstallationId& id) const {
        const InstallationDef* d = m_installations.Find(id);
        if (!d) return 0.0;
        const int n = Owned(id);
        return n * d->output * m_installations.MilestoneMultiplier(n) * m_mods.yieldMultiplier;
    }
    // Lo que sumaria UNA unidad mas: lo que dice "+x/s" antes de comprarla.
    double OutputOfNext(const InstallationId& id) const {
        const InstallationDef* d = m_installations.Find(id);
        if (!d) return 0.0;
        const int n = Owned(id);
        return (n + 1) * d->output * m_installations.MilestoneMultiplier(n + 1) * m_mods.yieldMultiplier - InstallationOutput(id);
    }

    // Comprar `n` unidades. Valida (existe, esta abierta, alcanza) y cobra todo junto: una
    // compra a medias no existe. Es la UNICA puerta por la que se gana una instalacion.
    BuyResult BuyInstallation(const InstallationId& id, int n = 1) {
        const InstallationDef* d = m_installations.Find(id);
        if (!d) return BuyResult::Unknown;
        if (n <= 0) return BuyResult::BadCount;
        if (!IsInstallationUnlocked(id)) return BuyResult::Locked;
        if (!m_wallet.TrySpend({{ResourceId::Research, CostOfUnits(*d, Owned(id), n)}})) return BuyResult::CannotAfford;
        m_owned[id] += n;
        RecomputeInstallations();
        return BuyResult::Ok;
    }

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
                BodyId b;
                if (e.kind == EffectKind::UnlockBody && e.target != kMapKey && !BodyFromKey(e.target, b))
                    out.push_back("la tecnologia '" + t.id + "' abre '" + e.target + "', que no es un cuerpo");
            }
        }
        for (const InteractionDef& d : m_interactions.All())
            if (!d.unlockedAtStart && UnlockedBy(d.id).empty())
                out.push_back("la interaccion '" + d.id + "' no se desbloquea nunca");
        for (const InstallationDef& d : m_installations.All())
            if (!d.unlockedBy.empty() && !m_tree.Find(d.unlockedBy))
                out.push_back("la instalacion '" + d.id + "' la abre '" + d.unlockedBy + "', que no existe");
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

    // Las unidades de cada tipo que trae un guardado. Un tipo que ya no existe se descarta y
    // se cuenta (como las tecnologias): un catalogo nuevo no rompe una partida vieja.
    int RestoreInstallations(const std::unordered_map<InstallationId, int>& owned) {
        int dropped = 0;
        m_owned.clear();
        for (const auto& [id, n] : owned) {
            if (n > 0 && m_installations.Find(id)) m_owned[id] = n;
            else ++dropped;
        }
        RecomputeInstallations();
        return dropped;
    }
    const std::unordered_map<InstallationId, int>& OwnedMap() const { return m_owned; }

    // Vuelve a derivar modificadores e interacciones del conjunto de investigadas.
    void Recompute() {
        Modifiers m;
        std::unordered_set<InteractionId> unlocked;
        bool bodies[kBodyCount] = {};
        bool mapUnlocked = false;
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
                    case EffectKind::UnlockBody: {
                        BodyId b;
                        if (e.target == kMapKey) mapUnlocked = true;
                        else if (BodyFromKey(e.target, b)) bodies[static_cast<int>(b)] = true;
                        break;
                    }
                }
            }
        }
        m_mods = std::move(m);
        m_unlocked = std::move(unlocked);
        for (int i = 0; i < kBodyCount; ++i) m_bodies[i] = bodies[i];
        m_mapUnlocked = mapUnlocked;
        RecomputeInstallations();
    }

    // Lo que producen las instalaciones, antes del multiplicador global.
    void RecomputeInstallations() {
        double rate = 0.0;
        for (const InstallationDef& d : m_installations.All()) {
            const int n = Owned(d.id);
            if (n > 0) rate += n * d.output * m_installations.MilestoneMultiplier(n);
        }
        m_installRate = rate;
    }

private:
    Wallet m_wallet;
    TechTree m_tree;
    InteractionSet m_interactions;
    InstallationSet m_installations;
    bool m_bodies[kBodyCount] = {};
    bool m_mapUnlocked = false;
    std::unordered_map<InstallationId, int> m_owned;
    double m_installRate = 0.0;
    Modifiers m_mods;
    std::unordered_set<InteractionId> m_unlocked;
    std::unordered_map<InteractionId, double> m_cooldown;
    double m_elapsed = 0.0;
    std::uint64_t m_useCount = 0;
};

} // namespace evergrave
