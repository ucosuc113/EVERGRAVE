#pragma once

// LAS INSTALACIONES: lo que se compra una y otra vez para que el planeta produzca
// solo. Es la mitad "idle" del juego, a la que el arbol de tecnologias le da forma:
// una tecnologia abre un tipo de instalacion (un satelite, una planta de vapor) y de
// ahi en adelante se compran tantas como se quiera, cada una un poco mas cara.
//
// Igual que las tecnologias y las interacciones, una instalacion es una FILA de la
// hoja (Design/EVERGRAVE_Balance.xlsx, hoja Installations) que Tools/export_catalog.py
// convierte en TechCatalog.generated.h: aqui no hay un solo numero de balance.
//
// Tres reglas que conviene no deshacer:
//
// - El costo de la n-esima unidad es costoBase x crecimiento^(n-1). Es la curva de
//   Cookie Clicker: cada compra empuja la siguiente, asi que no hay una unidad
//   "correcta" que comprar mil veces, y por eso unidades de 0.05 por segundo no
//   desbalancean nada.
// - Los HITOS (10, 25, 50, 100, 200 unidades de un mismo tipo) multiplican lo que
//   produce ESE tipo. Es lo que hace que tener cincuenta satelites valga mas que
//   cincuenta veces uno, y lo que da una razon para seguir comprando lo mismo.
// - Se DERIVA todo de cuantas hay: lo que se guarda es el numero de unidades de cada
//   tipo y nada mas (ver SaveData.h). La produccion se recalcula.

#include "Bodies.h"

#include <cmath>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace evergrave {

using InstallationId = std::string;

// Donde se ve lo comprado, alrededor del planeta. Solo presentacion: el Core no lo mira.
enum class InstallationLook : int {
    Orbit,   // puntos que giran alrededor de la Tierra (satelites, espejos)
    Surface, // luces sobre la Tierra (estaciones, plantas, ciudades)
    Sun,     // alrededor del Sol (espejos, enjambre, forjas)
    Far,     // puntos que se alejan hacia el borde (sondas, puertas)
};

struct InstallationDef {
    InstallationId id;
    std::string name;
    std::string description;
    std::string unlockedBy;    // la tecnologia que la abre; vacia = desde el inicio
    double baseCost = 1.0;     // lo que cuesta la primera
    double output = 0.0;       // puntos por segundo de UNA unidad, antes de hitos y del multiplicador
    double growth = 1.15;      // cuanto sube el costo con cada unidad que ya se tiene
    int visibleCap = 30;       // cuantas se dibujan como maximo (el resto cuenta pero no se ve)
    InstallationLook look = InstallationLook::Surface;
    std::uint32_t color = 0xFFFFFF; // 0xRRGGBB, para dibujarla
    int era = 1;
    BodyId body = BodyId::Earth; // en que cuerpo se construye: la tienda de cada cuerpo es la suya
};

// Tener `count` unidades de un tipo multiplica su produccion por `multiplier`. Se apilan.
struct Milestone {
    int count = 0;
    double multiplier = 1.0;
};

class InstallationSet {
public:
    bool Add(InstallationDef def) {
        if (def.id.empty() || Find(def.id) || !(def.baseCost > 0.0) || !(def.growth > 1.0)) return false;
        m_defs.push_back(std::move(def));
        return true;
    }
    void SetMilestones(std::vector<Milestone> m) { m_milestones = std::move(m); }

    const std::vector<InstallationDef>& All() const { return m_defs; }
    const std::vector<Milestone>& Milestones() const { return m_milestones; }
    const InstallationDef* Find(const InstallationId& id) const {
        for (const InstallationDef& d : m_defs)
            if (d.id == id) return &d;
        return nullptr;
    }

    // Lo que multiplican, juntos, los hitos que `owned` unidades ya alcanzaron.
    double MilestoneMultiplier(int owned) const {
        double m = 1.0;
        for (const Milestone& x : m_milestones)
            if (owned >= x.count) m *= x.multiplier;
        return m;
    }
    // El proximo hito que falta (el numero de unidades), o 0 si ya se alcanzaron todos.
    int NextMilestone(int owned) const {
        int best = 0;
        for (const Milestone& x : m_milestones)
            if (owned < x.count && (best == 0 || x.count < best)) best = x.count;
        return best;
    }

private:
    std::vector<InstallationDef> m_defs;
    std::vector<Milestone> m_milestones;
};

enum class BuyResult {
    Ok,
    Unknown,      // no existe ese tipo
    Locked,       // la tecnologia que la abre no esta investigada
    BadCount,     // cero o negativo
    CannotAfford, // no alcanza el saldo
};

// El costo de comprar `n` unidades teniendo ya `owned`: la suma de n terminos de la serie
// geometrica. Una cuenta, para que lo que el boton muestra y lo que se cobra sean lo mismo.
inline double CostOfUnits(const InstallationDef& d, int owned, int n) {
    if (n <= 0) return 0.0;
    const double first = d.baseCost * std::pow(d.growth, owned);
    return first * (std::pow(d.growth, n) - 1.0) / (d.growth - 1.0);
}

// Cuantas unidades se pueden comprar con `balance` teniendo ya `owned` (0 si ni una).
inline int MaxAffordableUnits(const InstallationDef& d, int owned, double balance) {
    const double first = d.baseCost * std::pow(d.growth, owned);
    if (!(balance >= first)) return 0;
    int n = static_cast<int>(std::floor(std::log(1.0 + balance * (d.growth - 1.0) / first) / std::log(d.growth) + 1e-9));
    // La formula se acerca por flotantes: se ajusta contra la suma exacta.
    while (n > 0 && CostOfUnits(d, owned, n) > balance) --n;
    while (CostOfUnits(d, owned, n + 1) <= balance) ++n;
    return n;
}

} // namespace evergrave
