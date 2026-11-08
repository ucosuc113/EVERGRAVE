// SunWorks: lo que se construye alrededor del SOL, como objetos de la escena. La hermana de Fleet (que hace lo mismo para la Tierra).
//
// Los anillos de Dyson y la nube de colectores eran puntos de UI (SunFx): una hilera de pixeles dorados que, ademas de no recibir el brillo ni
// el desenfoque del motor, no se leia como una ESTRUCTURA. Ahora son un anillo de verdad (la malla fina de `ring.strmesh`) y sesenta cubos
// que brillan, y el brillo del post los baña. La logica de CUANDO existe cada cosa es la de SunFx (las mismas tecnologias); aqui solo se
// ponen. `Tools/gen_fleet.py` escribe las piezas (`SunWorks/RingSolid`, `.../Collector07`...).
//
// QUE PONE
//  - LOS ANILLOS SON UN GIROSCOPIO, NO UNA FOTO. En la ciencia ficcion un anillo de Dyson gira, y los de varias capas no giran todos igual: cada uno
//    da vueltas sobre su propio eje (`spin`), y ademas su plano GIRA alrededor del Sol (la precesion del nodo, `nodeRate`) y se inclina y desinclina
//    (el vaiven, `wobble`), cada uno a su ritmo y algunos en sentido contrario. Todo muy lento (vueltas de minutos): se nota si se mira un rato, no
//    marea. Estan en CAPAS: radios distintos con holgura de sobra para que, al inclinarse, uno no atraviese a otro. Los mandos estan en `kRings`.
//  - EL ANILLO: un fantasma tenue con `megastructure_design`, solido (naranja, luminoso) con `collector_ring`, y con `solar_assembly` dos
//    anillos mas a otra altura e inclinacion (el Sol parece un giroscopio). Tres pulsos de carga corren por el solido.
//  - LA NUBE: 60 colectores con `solar_collector_swarm`, cada uno con su orbita (Kepler: los de dentro adelantan a los de fuera).
//
// LA MISMA CUENTA QUE SunFx (`OnRing`, `Phase`): una orbita de `k` radios solares, inclinada y girada; en la vista del Sol (que lo muestra
// casi a pantalla completa) los radios se comprimen (k pasa a 1 + (k-1) * 0,42), y en las otras dos van enteros. Si SunFx cambia esa
// cuenta, cambia aqui.
//
// El tamano se mide en el mundo y crece con la distancia de la camara al Sol (como Fleet): desde la Tierra el Sol esta a 150 unidades y un
// colector de 0,25 no llegaria a un pixel.

#include "FxKit.h"
#include "../Services/FxClock.h"

#include "Engine/Rendering/Material.h"
#include "Engine/Rendering/MaterialCache.h"

#include <string>

namespace {
// Un anillo: su radio en radios solares, su plano base, y como se mueve (todo en radianes y segundos).
struct RingDef {
    float k, tilt0, node0;
    float spin;       // giro sobre su propio eje, rad/s (el signo es el sentido)
    float nodeRate;   // la precesion: cuanto gira el plano alrededor del Sol, rad/s
    float wobble;     // la amplitud del vaiven de la inclinacion, rad
    float wobblePeriod, wobblePhase;
};
constexpr float kDeg = 3.14159265f / 180.0f;
//                                   k     tilt0  node0   spin         nodeRate     wobble      periodo fase
constexpr RingDef kSolid{2.4f, 0.00f, 0.0f, 4.0f * kDeg, 0.8f * kDeg, 8.0f * kDeg, 90.0f, 0.0f};    // el principal: el mas lento y casi plano
constexpr RingDef kRingA{1.9f, 0.50f, 0.4f, -6.0f * kDeg, -1.3f * kDeg, 10.0f * kDeg, 70.0f, 1.3f}; // el de dentro: mas rapido y al reves
constexpr RingDef kRingB{2.95f, -0.55f, 1.6f, 3.0f * kDeg, 1.0f * kDeg, 12.0f * kDeg, 110.0f, 2.6f}; // el de fuera: el mas inclinado
constexpr RingDef kGhost{2.4f, 0.00f, 0.0f, 0.0f, 0.5f * kDeg, 5.0f * kDeg, 90.0f, 0.0f};           // el contorno de lo que se va a construir
} // namespace

class SunWorks : public IScript {
public:
    void OnStart() override {
        m_root = FindByPath(SceneService::MutableRoots(), "SunWorks");
        m_sun = FindByPath(SceneService::MutableRoots(), kSunPath);
        if (!m_root || !m_sun) {
            m_root = nullptr;
            return;
        }
        FxBus().sunWorksInScene = true;
        for (auto& c : m_root->children) {
            GameObject* o = c.get();
            if (o->name == "RingSolid") m_solid = o;
            else if (o->name == "RingGhost") m_ghost = o;
            else if (o->name == "RingA") m_ringA = o;
            else if (o->name == "RingB") m_ringB = o;
            else if (o->name.rfind("Pulse", 0) == 0) m_pulses.push_back(o);
            else if (o->name.rfind("Collector", 0) == 0) m_collectors.push_back(o);
        }
    }
    void OnDestroy() override { FxBus().sunWorksInScene = false; }

    void OnUpdate(float) override {
        if (!m_root) return;
        const UiState& ui = Ui();
        const bool show = !ui.menu && !ui.mapView;
        m_root->active = show;
        if (!show) return;

        const TechTree& t = Session().tree();
        const XMFLOAT3 cf = ComputeWorldPosition(*m_sun);
        m_center = XMLoadFloat3(&cf);
        const XMMATRIX sm = ComputeWorldMatrix(*m_sun);
        m_radius = XMVectorGetX(XMVector3Length(XMVectorSubtract(XMVector3TransformCoord(XMVectorSet(1, 0, 0, 0), sm), m_center)));
        m_big = ui.view == BodyId::Sun;
        m_time = FxClock().time;
        XMVECTOR cam = XMVectorAdd(m_center, XMVectorSet(0, 0, kRefDist, 0));
        if (const GameObject* camObj = FindActiveCamera(SceneService::Roots())) {
            const XMFLOAT3 p = ComputeWorldPosition(*camObj);
            cam = XMLoadFloat3(&p);
        }
        const float grow = std::max(1.0f, XMVectorGetX(XMVector3Length(XMVectorSubtract(cam, m_center))) / kRefDist);

        const bool ghost = t.IsResearched("megastructure_design"), solid = t.IsResearched("collector_ring"), assembly = solid && t.IsResearched("solar_assembly");
        Ring(m_solid, solid, kSolid);
        Ring(m_ghost, ghost && !solid, kGhost);
        Ring(m_ringA, assembly, kRingA);
        Ring(m_ringB, assembly, kRingB);
        for (std::size_t k = 0; k < m_pulses.size(); ++k) {
            m_pulses[k]->active = solid;
            if (!solid) continue;
            // Los pulsos van POR el anillo solido, que ahora se mueve: en su plano de este instante, y llevados por su giro ademas de correr.
            SetPos(*m_pulses[k], OnRing(kSolid.k, TiltOf(kSolid), NodeOf(kSolid), kSolid.spin * m_time + m_time * 0.4f + static_cast<float>(k) * 2.1f));
            SetUniform(*m_pulses[k], 0.5f * grow);
        }
        // EL SOL, A LOS MATERIALES DEL COLECTOR: su shader (DysonCollector.hlsl) ilumina cada cara segun cuanto mira al Sol, y la luz de la escena
        // no sirve para eso (va del origen a la Tierra, y el Sol que se ve esta en otra parte). Es por RUTA y compartido: es un dato del mundo.
        if (MaterialCache* cache = SceneService::Materials())
            for (const wchar_t* path : {L"Assets/Materials/CollectorPanel.strmat", L"Assets/Materials/CollectorGold.strmat"})
                if (Material* m = cache->GetMutable(path)) m->userParams[1] = XMFLOAT4(cf.x, cf.y, cf.z, 1.0f);

        const bool cloud = t.IsResearched("solar_collector_swarm");
        for (std::size_t i = 0; i < m_collectors.size(); ++i) {
            GameObject* c = m_collectors[i];
            c->active = cloud;
            if (!cloud) continue;
            const unsigned u = static_cast<unsigned>(i);
            const float k = 1.6f + Hash01(u, 500u) * 3.0f;
            const XMVECTOR pos = OnRing(k, (Hash01(u, 501u) - 0.5f) * 2.2f, Hash01(u, 502u) * 6.2832f, Phase(k, Hash01(u, 503u) * 6.2832f));
            SetPos(*c, pos);
            SetUniform(*c, 0.32f * grow);
            // +Z (la cara que se abre) hacia el Sol, y un giro lento sobre ese mismo eje: cada uno a su ritmo.
            AimZ(*c, XMVectorSubtract(m_center, pos));
            c->transform.rotation[2] = std::fmod(m_time * 12.0f + static_cast<float>(i) * 37.0f, 360.0f);
        }
    }

private:
    // La misma cuenta que SunFx::OnRing.
    XMVECTOR OnRing(float k, float tilt, float node, float a) const {
        if (m_big) k = 1.0f + (k - 1.0f) * 0.42f;
        const XMVECTOR v = XMVectorSet(std::cos(a) * k * m_radius, 0.0f, std::sin(a) * k * m_radius, 0.0f);
        return XMVectorAdd(m_center, XMVector3TransformNormal(v, XMMatrixRotationX(tilt) * XMMatrixRotationY(node)));
    }
    float Phase(float k, float phase) const { return phase + m_time * 1.3f / std::pow(std::max(k, 1.05f), 1.5f); }

    float TiltOf(const RingDef& r) const { return r.tilt0 + r.wobble * std::sin(6.2831853f * m_time / r.wobblePeriod + r.wobblePhase); }
    float NodeOf(const RingDef& r) const { return r.node0 + r.nodeRate * m_time; }

    // Un anillo: la malla es de radio 1, asi que la escala es el radio de la orbita; la rotacion, la misma inclinacion (X) y giro (Y).
    // El parent lleva la inclinacion y la precesion (X y Y); su hijo `Spin`, el giro sobre el eje propio del anillo (Y local).
    void Ring(GameObject* o, bool on, const RingDef& r) const {
        if (!o) return;
        o->active = on;
        if (!on) return;
        float k = r.k;
        if (m_big) k = 1.0f + (k - 1.0f) * 0.42f;
        SetPos(*o, m_center);
        SetUniform(*o, k * m_radius);
        o->transform.rotation[0] = TiltOf(r) * 180.0f / kPi;
        o->transform.rotation[1] = NodeOf(r) * 180.0f / kPi;
        o->transform.rotation[2] = 0.0f;
        for (auto& c : o->children)
            if (c->name == "Spin") c->transform.rotation[1] = std::fmod(r.spin * m_time * 180.0f / kPi, 360.0f);
    }

    static void SetPos(GameObject& o, const XMVECTOR& p) {
        XMFLOAT3 f;
        XMStoreFloat3(&f, p);
        o.transform.position[0] = f.x;
        o.transform.position[1] = f.y;
        o.transform.position[2] = f.z;
    }
    // Gira `o` para que su eje local Z apunte a `dir` (como una camara): el pitch positivo inclina el +Z hacia abajo. El balanceo (Z) lo pone quien llama.
    static void AimZ(GameObject& o, FXMVECTOR dir) {
        const XMVECTOR d = XMVector3Normalize(dir);
        o.transform.rotation[0] = -std::asin(std::clamp(XMVectorGetY(d), -1.0f, 1.0f)) * 180.0f / kPi;
        o.transform.rotation[1] = std::atan2(XMVectorGetX(d), XMVectorGetZ(d)) * 180.0f / kPi;
    }
    static void SetUniform(GameObject& o, float s) { o.transform.scale[0] = o.transform.scale[1] = o.transform.scale[2] = s; }

    static constexpr float kRefDist = 36.0f; // la distancia de la toma del Sol (CameraRig)

    GameObject* m_root = nullptr;
    GameObject* m_sun = nullptr;
    GameObject* m_solid = nullptr;
    GameObject* m_ghost = nullptr;
    GameObject* m_ringA = nullptr;
    GameObject* m_ringB = nullptr;
    std::vector<GameObject*> m_pulses, m_collectors;
    XMVECTOR m_center = XMVectorZero();
    float m_radius = 5.0f;
    float m_time = 0.0f;
    bool m_big = false;
};

STRACE_REGISTER_SCRIPT(SunWorks)
