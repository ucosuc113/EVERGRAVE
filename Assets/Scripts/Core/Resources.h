#pragma once

// LOS RECURSOS: que hay, cuanto hay y cuanto se ha ganado en total.
//
// Core/ no incluye el motor (ni la UI, ni el arbol de tecnologias): es lo que
// deja simular una partida con un `for` y probarla con g++ sin GPU. Un recurso
// nuevo (energia util, material, datos...) es UNA fila en kResources; ni la
// cartera ni el arbol nombran a ninguno.
//
// Se guardan DOS cifras por recurso, y la segunda no es un adorno: `lifetime`
// es lo ganado desde siempre y NUNCA baja al gastar. Es lo que mide una
// condicion de desbloqueo ("haber reunido 100 en total"): con el saldo actual,
// comprar una tecnologia cerraria la siguiente.

#include <array>
#include <cstddef>
#include <vector>

namespace evergrave {

enum class ResourceId : int {
    Research = 0, // Puntos de Investigacion Energetica
    Count
};

inline constexpr std::size_t kResourceCount = static_cast<std::size_t>(ResourceId::Count);

struct ResourceInfo {
    const char* key;  // estable: es lo que se guarda en disco
    const char* name; // lo que se muestra
};

// El orden es el de ResourceId.
inline constexpr std::array<ResourceInfo, kResourceCount> kResources = {{
    {"research", "Energy Research Points"},
}};

inline const ResourceInfo& InfoOf(ResourceId id) { return kResources[static_cast<std::size_t>(id)]; }

// Una cantidad de un recurso: un costo, una condicion, un premio.
struct ResourceAmount {
    ResourceId id = ResourceId::Research;
    double amount = 0.0;
};

class Wallet {
public:
    double Get(ResourceId id) const { return m_amount[Index(id)]; }
    double Lifetime(ResourceId id) const { return m_lifetime[Index(id)]; }

    // Ganar. Una cantidad negativa o cero no hace nada: gastar tiene su propio
    // camino (TrySpend) para que un bug que "gana" -5 no se coma el saldo sin
    // pasar por la validacion de costos.
    void Add(ResourceId id, double amount) {
        if (!(amount > 0.0)) return; // tambien descarta NaN
        m_amount[Index(id)] += amount;
        m_lifetime[Index(id)] += amount;
    }

    bool CanAfford(const std::vector<ResourceAmount>& costs) const {
        for (const ResourceAmount& c : costs)
            if (Get(c.id) < c.amount) return false;
        return true;
    }

    // Todo o nada: si falta un solo recurso no se descuenta ninguno. (Hoy hay
    // uno; con dos, descontar el primero y fallar en el segundo seria un robo.)
    bool TrySpend(const std::vector<ResourceAmount>& costs) {
        if (!CanAfford(costs)) return false;
        for (const ResourceAmount& c : costs) m_amount[Index(c.id)] -= c.amount;
        return true;
    }

    // Solo para cargar una partida: pone las dos cifras tal cual.
    void Restore(ResourceId id, double amount, double lifetime) {
        const double a = amount > 0.0 ? amount : 0.0; // tambien descarta NaN
        m_amount[Index(id)] = a;
        m_lifetime[Index(id)] = lifetime > a ? lifetime : a; // lo ganado nunca es menos que lo que hay
    }

    void Clear() {
        m_amount.fill(0.0);
        m_lifetime.fill(0.0);
    }

private:
    static std::size_t Index(ResourceId id) { return static_cast<std::size_t>(id); }

    std::array<double, kResourceCount> m_amount{};
    std::array<double, kResourceCount> m_lifetime{};
};

} // namespace evergrave
