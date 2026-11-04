#pragma once

// DONDE VA CADA TECNOLOGIA EN EL MAPA: la geometria del arbol, sin dibujar nada.
//
// El arbol ya no es una pila de filas centradas (con 260 tecnologias una fila de
// ocho pedia 888 px en un panel de 410): es un MAPA de carriles. Cada RAMA
// (Power, Optics...) tiene su carril fijo, de izquierda a derecha, y la altura de
// una tecnologia es su ESCALON dentro de la era (sumando las eras de antes): el mapa
// crece hacia abajo, como el arbol de Cell to Singularity.
//
// Que el carril sea fijo es lo que deja ir abriendo el mapa sin que nada se mueva:
// una tecnologia que aparece lo hace donde ya estaba su sitio. Las posiciones se
// calculan del arbol y no se escriben en el catalogo, asi una tecnologia nueva se
// acomoda sola; solo la rama (que viene de la hoja) decide el carril.
//
// No incluye el motor a proposito: son rectangulos y es lo que se puede probar
// con g++ (Tests/core_tests.cpp) sin abrir una ventana. El panel solo recorre el
// resultado y pinta.

#include "../Core/Tech.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <unordered_map>
#include <vector>

namespace evergrave {

struct LRect {
    float x = 0, y = 0, w = 0, h = 0;
    float Right() const { return x + w; }
    float Bottom() const { return y + h; }
    bool Contains(float px, float py) const { return px >= x && px < Right() && py >= y && py < Bottom(); }
    bool Overlaps(const LRect& o) const { return x < o.Right() && o.x < Right() && y < o.Bottom() && o.y < Bottom(); }
};

// Orden de los carriles, izquierda a derecha. Salio de minimizar la distancia de
// los requisitos que cruzan de una rama a otra en el arbol real (220 de 427 lo
// hacen): una flecha larga atraviesa el mapa y se lee mal. Una rama que no este en
// la lista va al final, en orden de aparicion: la hoja puede estrenar ramas.
inline const std::vector<std::string>& LaneOrder() {
    static const std::vector<std::string> order{"Solar", "Mining", "Industrial", "Instruments", "Orbit",
                                                "Logistics", "Power", "Optics", "Materials", "Lasers"};
    return order;
}

struct MapParams {
    float laneW = 41;     // ancho de un carril
    float rowH = 32;      // alto de una fila
    float node = 18;      // lado de un nodo
    float sideStep = 10;  // cuanto se separan dos nodos de la misma rama y fila (cada uno a un lado)
    float topPad = 8;
};

struct MapNode {
    TechId id;
    int row = 0;  // altura en filas
    int lane = 0;
    float cx = 0, cy = 0; // centro
    LRect rect;
};

// padre -> hijo: un segmento de centro a centro. Los nodos se dibujan encima.
struct MapEdge {
    TechId parent, child;
    float x0 = 0, y0 = 0, x1 = 0, y1 = 0;
};

// Donde empieza una era (la primera fila de sus tecnologias): para el divisor.
struct EraMark {
    int era = 1;
    float y = 0;    // alto del divisor
    TechId first;   // la primera tecnologia de la era (el divisor aparece con ella)
};

struct TechMap {
    std::vector<MapNode> nodes;
    std::vector<MapEdge> edges;
    std::vector<std::string> lanes; // el nombre de la rama de cada carril
    std::vector<EraMark> eras;
    float contentW = 0, contentH = 0;
    std::unordered_map<TechId, int> index;

    const MapNode* Find(const TechId& id) const {
        auto it = index.find(id);
        return it == index.end() ? nullptr : &nodes[it->second];
    }

    // El nodo bajo un punto (en las coordenadas del mapa), o nullptr. `slack` agranda
    // el nodo: un blanco de 18 px es chico para un clic.
    const MapNode* HitTest(float px, float py, float slack = 2.0f) const {
        for (const MapNode& n : nodes)
            if (px >= n.rect.x - slack && px < n.rect.Right() + slack && py >= n.rect.y - slack && py < n.rect.Bottom() + slack) return &n;
        return nullptr;
    }
};

inline TechMap BuildTechMap(const TechTree& tree, const MapParams& p = {}) {
    TechMap out;

    // Carriles: primero los de la lista fija que existan, despues los nuevos.
    std::unordered_map<std::string, int> laneOf;
    auto laneFor = [&](const std::string& branch) {
        auto it = laneOf.find(branch);
        if (it != laneOf.end()) return it->second;
        // Se decide al final (ver abajo): aqui solo se anota que existe.
        laneOf[branch] = -1;
        return -1;
    };
    for (const TechDef& d : tree.All()) laneFor(d.branch);
    for (const std::string& b : LaneOrder())
        if (laneOf.count(b)) { laneOf[b] = static_cast<int>(out.lanes.size()); out.lanes.push_back(b); }
    for (const TechDef& d : tree.All())
        if (laneOf[d.branch] < 0) { laneOf[d.branch] = static_cast<int>(out.lanes.size()); out.lanes.push_back(d.branch); }

    // Fila: el escalon de la tecnologia, corrido por las eras de antes (la era 2 empieza
    // donde acaba la 1). Asi la altura sigue el progreso que la hoja diseno -y las
    // divisorias de era caen alineadas- y no la cadena de requisitos. Nunca queda
    // por encima de un requisito: si un escalon mal puesto lo pidiera, baja.
    std::unordered_map<int, int> eraTiers; // era -> el escalon mas alto
    for (const TechDef& d : tree.All()) eraTiers[d.era] = std::max(eraTiers[d.era], std::max(1, d.tier));
    std::vector<int> eraOrder;
    for (const auto& kv : eraTiers) eraOrder.push_back(kv.first);
    std::sort(eraOrder.begin(), eraOrder.end());
    std::unordered_map<int, int> eraOffset;
    int acc = 0;
    for (int e : eraOrder) { eraOffset[e] = acc; acc += eraTiers[e]; }

    std::unordered_map<TechId, int> depth;
    std::unordered_map<long long, std::vector<std::size_t>> cell; // (carril, fila) -> nodos
    int maxRow = 0;
    for (const TechDef& d : tree.All()) {
        int dep = eraOffset[d.era] + std::max(1, d.tier) - 1;
        for (const TechId& pre : d.prerequisites) {
            auto it = depth.find(pre);
            if (it != depth.end()) dep = std::max(dep, it->second + 1);
        }
        depth[d.id] = dep;
        maxRow = std::max(maxRow, dep);

        MapNode n;
        n.id = d.id;
        n.row = dep;
        n.lane = laneOf[d.branch];
        out.index[d.id] = static_cast<int>(out.nodes.size());
        cell[static_cast<long long>(n.lane) * 100000 + dep].push_back(out.nodes.size());
        out.nodes.push_back(n);
    }

    // Posicion: el centro del carril; si hay dos en la misma celda, uno a cada lado.
    for (auto& kv : cell) {
        const std::vector<std::size_t>& group = kv.second;
        const float k = static_cast<float>(group.size());
        for (std::size_t i = 0; i < group.size(); ++i) {
            MapNode& n = out.nodes[group[i]];
            const float side = (static_cast<float>(i) - (k - 1.0f) * 0.5f) * p.sideStep * 2.0f;
            n.cx = std::floor((static_cast<float>(n.lane) + 0.5f) * p.laneW + side);
            n.cy = std::floor(p.topPad + static_cast<float>(n.row) * p.rowH + p.node * 0.5f);
            n.rect = LRect{n.cx - p.node * 0.5f, n.cy - p.node * 0.5f, p.node, p.node};
        }
    }
    out.contentW = static_cast<float>(out.lanes.size()) * p.laneW;
    out.contentH = p.topPad + static_cast<float>(maxRow + 1) * p.rowH;

    for (const TechDef& d : tree.All()) {
        const MapNode* child = out.Find(d.id);
        for (const TechId& pre : d.prerequisites) {
            const MapNode* parent = out.Find(pre);
            if (!parent || !child) continue;
            out.edges.push_back({pre, d.id, parent->cx, parent->cy, child->cx, child->cy});
        }
    }

    // Divisores de era: donde cae la primera tecnologia de cada una.
    std::unordered_map<int, std::size_t> firstOfEra;
    for (std::size_t i = 0; i < tree.All().size(); ++i) {
        const int era = tree.All()[i].era;
        auto it = firstOfEra.find(era);
        if (it == firstOfEra.end() || out.nodes[i].row < out.nodes[it->second].row) firstOfEra[era] = i;
    }
    for (const auto& kv : firstOfEra)
        out.eras.push_back({kv.first, out.nodes[kv.second].rect.y - 12.0f, out.nodes[kv.second].id});
    std::sort(out.eras.begin(), out.eras.end(), [](const EraMark& a, const EraMark& b) { return a.era < b.era; });
    return out;
}

} // namespace evergrave
