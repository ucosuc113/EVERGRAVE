// AsteroidBelt: el cinturon de asteroides alrededor del Sol. Por ahora SOLO SE VE: no se le hace clic, no
// produce nada y la partida no sabe que existe. Es la base visual de las mejoras que hablan de asteroides
// (Asteroid Prospecting, Asteroid Capture, Belt Refineries, el Asteroid Miner...): cuando una de ellas quiera
// mostrarse, las rocas ya estan, con nombre, y se las busca por ruta (`Space/OrbitPivot/Belt/Asteroid042`).
//
// LA COLOCACION ES DATOS DE LA ESCENA, NO DE ESTE SCRIPT. Tools/gen_belt.py escribe los hijos de `Belt` (donde,
// que tamano, que forma, por que entre el Sol y la Tierra: ver su cabecera). Este script lee esa foto en OnStart
// - radio, angulo y altura de cada hijo - y la anima. Asi una roca movida a mano en el Editor sigue orbitando
// desde donde la dejaron, y regenerar el cinturon no pide tocar codigo.
//
// QUE ANIMA:
//  - LA ORBITA, con la tercera ley de Kepler respecto a la Tierra (velocidad angular ~ r^-1,5): las de dentro
//    adelantan a las de fuera. Con solo eso el anillo "se mueve como un fluido" en vez de girar como un disco
//    rigido, que es lo que delata un cinturon falso;
//  - UNA INCLINACION por roca: la altura de la escena es la amplitud, y la roca sube y baja una vez por vuelta
//    (cada una con su nodo), asi el grosor del anillo respira sin que ninguna lo deje;
//  - EL GIRO propio: cada roca da tumbos a su ritmo, las chicas mas rapido.
//
// EL CINTURON SIGUE AL SOL QUE SE VE. En las vistas de la Tierra y la Luna el Sol se aleja por la linea Tierra-Sol
// (SpaceWorld, kSunFarDistance) para verse chico; si el cinturon se quedara en el origen dejaria de estar
// alrededor de el. Se copia la posicion del nodo del Sol al nodo `Belt` cada frame: el anillo se aleja con el y
// desde la Tierra se ve en perspectiva, una elipse de puntos alrededor de una estrella lejana. Este script corre
// antes o despues de SpaceWorld segun el orden de la escena; si es antes, va un frame atrasado respecto al Sol, que
// solo se mueve con un suavizado lento - no se ve.
//
// Y DE LEJOS SE ACERCA A LA CAMARA, a proposito. A 150 unidades una roca de 0,25 mide menos de un pixel de juego:
// se probo y el anillo era una raya de puntos sueltos cruzando el Sol. Agrandar las rocas en su sitio no alcanza
// (las grandes taparian al Sol antes de que las chicas se vean), e inclinar el anillo para que se abra en elipse se
// descarto: el anillo tiene que verse de canto, plano, DELANTE del Sol.
// El truco es de perspectiva: cada roca se mueve por la recta que la une con la CAMARA, hasta `kFarNear` de su
// distancia (una homotecia con centro en la camara). En pantalla cae en el mismo pixel - la misma raya delante del
// mismo Sol -, pero esta mas cerca, asi que el mismo objeto se ve mas grande y con mas detalle, y al girar la camara
// tiene el paralaje de algo cercano. Encima, cada roca se agranda hasta `kFarGrow` veces lo que se veria de lejos
// (las chicas mas, las grandes menos: las cuatro mayores no le pueden hacer sombra al Sol).
// La escena guarda el cinturon "del mapa" (el real, a escala del dibujo); cuanto mas lejos esta el Sol que se ve -
// `Lejania()`, 0 en el mapa y la vista del Sol, 1 en la Tierra y la Luna - mas pesa el truco, y con el Sol en el
// centro no hace nada. Como cuelga de la misma lejania que mueve al Sol, el cambio se ve como un solo movimiento.
// Consecuencia aceptada: de lejos TODAS las rocas quedan delante del Sol, incluidas las que estarian detras de el.
// Desde la Tierra las rocas muestran la cara de NOCHE (la luz va del Sol hacia aca): siluetas oscuras delante del
// Sol, y es a proposito - se probo darles brillo propio y se descarto. No agregarles `emissive` "para que se vean".
//
// EL SOL QUE SE VE, AL SHADER. El filo de contraluz de las rocas (Assets/Shaders/Asteroid.hlsl) es un degradado
// RADIAL alrededor del disco del Sol tal como lo ve la camara: maximo en las rocas que lo tapan, casi nada en los
// bordes del anillo. La direccion de la luz no sirve para eso - va del origen a la Tierra, y el Sol que se ve esta
// en otra parte (se alejo, y la camara no esta en la Tierra): el centro del degradado quedaba unos grados corrido.
// Asi que cada frame se escribe la posicion del Sol en `userParams[1]` (w = 1: "hay dato") de los materiales que
// usan las rocas. Es por RUTA (`SceneService::Materials()`), compartido por todas: es un dato del mundo, no de una
// roca. Sin el (w = 0), el shader vuelve a la direccion de la luz.

#include "../Services/Cine.h"

#include "Engine/Diagnostics/Log.h"
#include "Engine/Rendering/Material.h"
#include "Engine/Rendering/MaterialCache.h"
#include "Engine/Scene/GameObject.h"
#include "Engine/Scene/SceneQuery.h"
#include "Engine/Scene/SceneService.h"
#include "Engine/Scripting/IScript.h"
#include "Engine/Scripting/ScriptRegistry.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

using namespace strace;

namespace {

constexpr const char* kSunPath = "Space/OrbitPivot/Sun";
constexpr float kRad = 3.14159265f / 180.0f;
// La referencia de Kepler: la Tierra orbita a 30 unidades a 0,8 grados por segundo (SpaceWorld: kEarthOrbitRadius,
// kOrbitDegPerSec). Copiados y no compartidos porque alla son detalles privados de la animacion; si cambian, el
// cinturon solo cambia de ritmo, no se rompe.
constexpr float kRefRadius = 30.0f, kRefDegPerSec = 0.8f;
// Lo lejos que se va el Sol que se ve: SpaceWorld lo pone a 150 de la Tierra por la linea Tierra-origen, o sea a
// 150 - 30 = 120 del origen.
constexpr float kSunFarFromOrigin = 120.0f;
// La exageracion de lejos (ver la cabecera).
constexpr float kFarGrow = 3.2f;   // cuanto mas grande se VE una roca chica de lejos (en pantalla)
constexpr float kFarNear = 0.35f;  // a que fraccion de su distancia a la camara se acerca
constexpr const char* kCameraPath = "Space/OrbitPivot/Camara";

// Un numero en [0,1) por roca y canal, sin estado: la misma roca da tumbos igual en cada corrida.
float Hash01(uint32_t a, uint32_t b) {
    uint32_t h = a * 0x9E3779B1u ^ (b + 0x7F4A7C15u) * 0x85EBCA77u;
    h ^= h >> 15; h *= 0x2C1B3C6Du; h ^= h >> 12; h *= 0x297A2D39u; h ^= h >> 15;
    return static_cast<float>(h >> 8) / 16777216.0f;
}

struct Rock {
    GameObject* node = nullptr;
    float radius = 0.0f;   // distancia al Sol en el plano
    float angle = 0.0f;    // grados, el de ahora
    float degPerSec = 0.0f;
    float lift = 0.0f;     // la altura de la escena: amplitud de la inclinacion
    float node0 = 0.0f;    // el angulo donde la roca esta en lo mas alto (el de la escena)
    float spin[3] = {0, 0, 0};
    float scale[3] = {1, 1, 1}; // la de la escena (la del mapa)
    float grow = 1.0f;          // cuanto crece de lejos: las chicas mas, las grandes menos
};

} // namespace

class AsteroidBelt : public IScript {
public:
    void OnStart() override {
        m_sun = FindByPath(SceneService::MutableRoots(), kSunPath);
        m_cam = FindByPath(SceneService::MutableRoots(), kCameraPath);
        m_rocks.clear();
        uint32_t i = 0;
        for (auto& child : gameObject->children) {
            GameObject* o = child.get();
            Rock r;
            r.node = o;
            const float x = o->transform.position[0], z = o->transform.position[2];
            r.radius = std::sqrt(x * x + z * z);
            if (r.radius < 0.01f) continue; // una roca en el centro estaria dentro del Sol: no se anima
            r.angle = std::atan2(z, x) / kRad;
            r.node0 = r.angle;
            r.lift = o->transform.position[1];
            r.degPerSec = kRefDegPerSec * std::pow(kRefRadius / r.radius, 1.5f);
            // Tumbos: 6-30 grados por segundo, mas rapido cuanto mas chica (el tamano es el mayor eje de la escala).
            const float size = std::max({o->transform.scale[0], o->transform.scale[1], o->transform.scale[2], 0.05f});
            const float pace = std::clamp(0.25f / size, 0.35f, 1.6f);
            for (int k = 0; k < 3; ++k) r.scale[k] = o->transform.scale[k];
            r.grow = 1.0f + (kFarGrow - 1.0f) * std::clamp(0.35f / size, 0.45f, 1.15f);
            for (uint32_t k = 0; k < 3; ++k) {
                const float h = Hash01(i, k);
                r.spin[k] = (6.0f + 18.0f * h) * pace * (Hash01(i, k + 7) < 0.5f ? -1.0f : 1.0f);
            }
            if (!o->materialPath.empty() &&
                std::find(m_materials.begin(), m_materials.end(), o->materialPath) == m_materials.end())
                m_materials.push_back(o->materialPath);
            m_rocks.push_back(r);
            ++i;
        }
        Log::Info(L"EVERGRAVE: cinturon de asteroides con " + std::to_wstring(m_rocks.size()) + L" rocas");
        Follow();
    }

    void OnUpdate(float dt) override {
        Follow();
        PublishSun();
        // FILMANDO NO HAY TRUCO: cada roca va en su sitio de verdad (las que estan detras del Sol, detras de el, tapadas; con el truco todas
        // quedaban DELANTE del Sol, y en un plano de cine se lee como un error), y el cinturon entero se escala con el Sol de la toma.
        const bool cine = evergrave::Cine().active;
        const float far = cine ? 0.0f : Lejania();
        const float near = 1.0f - (1.0f - kFarNear) * far;
        const float zoom = cine ? evergrave::Cine().f.k.sunScale : 1.0f;
        // La camara en coordenadas del cinturon (Belt y Camara cuelgan de OrbitPivot, sin giro ni escala: restar basta).
        float cam[3] = {0, 0, 0};
        if (m_cam)
            for (int k = 0; k < 3; ++k) cam[k] = m_cam->transform.position[k] - gameObject->transform.position[k];
        for (Rock& r : m_rocks) {
            r.angle = std::fmod(r.angle + r.degPerSec * dt, 360.0f);
            const float a = r.angle * kRad;
            const float q[3] = {zoom * r.radius * std::cos(a), zoom * r.lift * std::cos(a - r.node0 * kRad), zoom * r.radius * std::sin(a)};
            // Hacia la camara por la misma recta: mismo pixel, mas cerca.
            float* p = r.node->transform.position;
            for (int k = 0; k < 3; ++k) p[k] = cam[k] + (q[k] - cam[k]) * near;
            const float g = 1.0f + (r.grow - 1.0f) * far;
            // Al acercarse ya se ve 1/near mas grande: la escala real se compensa para que lo que se VE sea g.
            for (int k = 0; k < 3; ++k) r.node->transform.scale[k] = r.scale[k] * g * near * zoom;
            float* rot = r.node->transform.rotation;
            for (int k = 0; k < 3; ++k) rot[k] = std::fmod(rot[k] + r.spin[k] * dt + 360.0f, 360.0f);
        }
    }

private:
    // 0 con el Sol en el centro del sistema (mapa, vista del Sol), 1 con el Sol alejado del todo (Tierra, Luna).
    // Se mide de la posicion del Sol y no se le pregunta a SpaceWorld: sigue sola su suavizado.
    float Lejania() const {
        if (!m_sun) return 0.0f;
        const float* p = m_sun->transform.position;
        return std::clamp(std::sqrt(p[0] * p[0] + p[2] * p[2]) / kSunFarFromOrigin, 0.0f, 1.0f);
    }

    // El centro del cinturon es el Sol que se ve (ver la cabecera).
    // La posicion del Sol visible, a los materiales de las rocas (ver la cabecera). `GetMutable` da nullptr hasta
    // que el material se dibujo una vez: por eso se pide cada frame y no se guarda.
    void PublishSun() const {
        MaterialCache* cache = SceneService::Materials();
        if (!cache || !m_sun) return;
        const float* s = m_sun->transform.position;
        for (const std::wstring& path : m_materials)
            if (Material* m = cache->GetMutable(path)) m->userParams[1] = DirectX::XMFLOAT4(s[0], s[1], s[2], 1.0f);
    }

    void Follow() {
        if (!m_sun) return;
        for (int k = 0; k < 3; ++k) gameObject->transform.position[k] = m_sun->transform.position[k];
    }

    GameObject* m_sun = nullptr;
    GameObject* m_cam = nullptr;
    std::vector<Rock> m_rocks;
    std::vector<std::wstring> m_materials;
};

STRACE_REGISTER_SCRIPT(AsteroidBelt)
