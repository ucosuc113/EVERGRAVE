#pragma once

// DONDE VA CADA TECNOLOGIA EN EL PANEL: la geometria del arbol, sin dibujar nada.
//
// Se calcula del arbol y no se escribe a mano en el catalogo: una tecnologia
// nueva no trae coordenadas, y el panel no se rompe cuando alguien agrega una en
// medio. La regla es una: la FILA de una tecnologia es su profundidad (la
// cadena de requisitos mas larga hasta una raiz), y dentro de una fila van en el
// orden del catalogo, centradas. Un requisito siempre queda en una fila de
// arriba, asi que las flechas solo van hacia abajo.
//
// No incluye el motor a proposito: son rectangulos y es lo que se puede probar
// con g++ (Tests/core_tests.cpp) sin abrir una ventana. El panel solo recorre
// el resultado y pinta.
//
// Limite conocido: una flecha entre filas NO consecutivas baja en vertical por
// el x del padre y puede pasar por detras de un nodo intermedio. Con un arbol
// de 6 filas no ocurre; cuando ocurra se resuelve aqui (un carril por arista),
// no en el panel.

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

// Un tramo recto, siempre horizontal o vertical.
struct Segment {
    float x0 = 0, y0 = 0, x1 = 0, y1 = 0;
};

struct NodeLayout {
    TechId id;
    int depth = 0;  // fila
    int column = 0; // lugar dentro de la fila
    LRect rect;
};

// padre -> hijo, en tres tramos: baja, cruza, baja.
struct EdgeLayout {
    TechId parent, child;
    Segment seg[3];
};

struct LayoutParams {
    float nodeW = 104, nodeH = 34, gapX = 8, gapY = 12;
};

struct TreeLayout {
    std::vector<NodeLayout> nodes;
    std::vector<EdgeLayout> edges;
    float contentHeight = 0;

    const NodeLayout* Find(const TechId& id) const {
        for (const NodeLayout& n : nodes)
            if (n.id == id) return &n;
        return nullptr;
    }

    // El nodo bajo un punto (en las mismas coordenadas que `rect`), o nullptr.
    const NodeLayout* HitTest(float px, float py) const {
        for (const NodeLayout& n : nodes)
            if (n.rect.Contains(px, py)) return &n;
        return nullptr;
    }
};

// Coordenadas con origen en la esquina de arriba a la izquierda del lienzo.
// `canvasW` es el ancho disponible: cada fila se centra en el.
inline TreeLayout BuildTreeLayout(const TechTree& tree, float canvasW, const LayoutParams& p = {}) {
    TreeLayout out;
    std::unordered_map<TechId, int> depth;
    std::vector<std::vector<const TechDef*>> rows;
    for (const TechDef& d : tree.All()) {
        int dep = 0;
        for (const TechId& pre : d.prerequisites) {
            auto it = depth.find(pre);
            if (it != depth.end()) dep = std::max(dep, it->second + 1);
        }
        depth[d.id] = dep;
        if (static_cast<int>(rows.size()) <= dep) rows.resize(dep + 1);
        rows[dep].push_back(&d);
    }

    for (std::size_t r = 0; r < rows.size(); ++r) {
        const int n = static_cast<int>(rows[r].size());
        const float rowW = n * p.nodeW + (n - 1) * p.gapX;
        const float x0 = std::floor((canvasW - rowW) * 0.5f);
        for (int c = 0; c < n; ++c) {
            NodeLayout nl;
            nl.id = rows[r][c]->id;
            nl.depth = static_cast<int>(r);
            nl.column = c;
            nl.rect = LRect{x0 + c * (p.nodeW + p.gapX), r * (p.nodeH + p.gapY), p.nodeW, p.nodeH};
            out.nodes.push_back(nl);
        }
    }
    out.contentHeight = rows.empty() ? 0.0f : rows.size() * (p.nodeH + p.gapY) - p.gapY;

    for (const TechDef& d : tree.All()) {
        const NodeLayout* child = out.Find(d.id);
        for (const TechId& pre : d.prerequisites) {
            const NodeLayout* parent = out.Find(pre);
            if (!parent || !child) continue;
            EdgeLayout e;
            e.parent = pre;
            e.child = d.id;
            const float px = std::floor(parent->rect.x + parent->rect.w * 0.5f);
            const float cx = std::floor(child->rect.x + child->rect.w * 0.5f);
            const float y0 = parent->rect.Bottom();
            const float y1 = child->rect.y;
            const float mid = std::floor(y1 - p.gapY * 0.5f);
            e.seg[0] = {px, y0, px, mid};
            e.seg[1] = {std::min(px, cx), mid, std::max(px, cx), mid};
            e.seg[2] = {cx, mid, cx, y1};
            out.edges.push_back(e);
        }
    }
    return out;
}

} // namespace evergrave
