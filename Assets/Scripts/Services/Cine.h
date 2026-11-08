#pragma once

// LA CINEMATICA: una animacion de ~62 segundos (la Tierra, la Luna, el Sol y sus anillos) hecha con el propio juego, para grabarla con
// `Editor.exe --play --record` (ver docs/cinematica.md). NO es un modo del juego: se enciende con EVERGRAVE_CINE=<segundo de
// arranque> y ahi se queda; sin la variable este archivo no hace nada y el juego es el de siempre.
//
// ES UNA FUNCION DEL TIEMPO. `CineEvaluate(t)` devuelve TODO lo que la toma pide en el segundo `t`: no hay estado que dependa de
// cuantos frames se renderizaron ni de lo lento que fue cada uno. Es lo que deja grabar con un paso fijo (`--record`) y saltar a
// cualquier segundo para mirar una escena sola (EVERGRAVE_CINE=23 arranca en el segundo 23). Por lo mismo es CODIGO SIN MOTOR, como
// `Core/`. Los cuerpos (`CineWorldOf`) se calculan con las mismas formulas de SpaceWorld y no se leen de la escena: los scripts corren en
// orden, y leer la Luna del frame anterior dejaria la camara un frame atras (un tiron invisible en una orbita, y un eclipse desalineado).
//
// UN BAILE, NO UNA SERIE DE FOTOS. Las tomas NO frenan al final: cada una es una curva suave (Hermite, con tangentes por diferencias) por
// varias claves, y en sus extremos conserva la velocidad en vez de llegar a cero. El corte entre dos tomas se esconde en un CUERPO QUE
// LLENA LA PANTALLA: la toma termina con la camara pasando muy cerca de un cuerpo (el lado de noche de la Tierra, el lado oscuro de la
// Luna, una roca, el Sol) hasta que lo unico que se ve es su negro, y la siguiente EMPIEZA pegada a otro cuerpo, ya en movimiento, y se
// aleja de el. La velocidad de la camara pasa de una toma a la otra sin pausa. Un pequeno apagon de 0,2 s cubre lo que el cuerpo no
// alcance a tapar (una roca iluminada, las luces de una ciudad).
//
// CADA CLAVE LLEVA SUS ANCLAS: la camara se coloca a (`dist`, `yaw`, `pitch`) de un `anchor` y mira a un `look`. Entre dos claves se
// interpolan por separado la posicion del ancla (en el mundo) y los tres numeros polares: con el MISMO ancla la camara da un arco
// alrededor del cuerpo, y con anclas distintas vuela de un cuerpo a otro.
//
// LA FASE DE LA LUNA se pide como un angulo (`moonOff`): 0 = entre la Tierra y el Sol, 90 = cuarto, 180 = llena. Puede crecer mas de 360:
// es como la Luna da vueltas (el vals) en una toma.
//
// EL ENCUADRE: IMAX (1,43:1, mas alto) y 16:9 se alternan por escena. La imagen siempre se dibuja a 16:9 y el IMAX son barras negras a
// los lados (`ratio`) que se deslizan al nuevo formato en el segundo que sigue al corte.

#include "../Core/Bodies.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace evergrave {

enum class CineAnchor { Earth, Moon, Sun, Mid, Rock0, Rock1, SunSeen }; // SunSeen = el Sol que se VE (no el centro del sistema)

struct CineKey {
    float at = 0.0f;       // en que punto de la toma (0..1) esta esta clave
    CineAnchor anchor = CineAnchor::Earth; // donde esta la camara (a `dist`, `yaw`, `pitch` de el)
    CineAnchor look = CineAnchor::Earth;   // a donde mira
    float dist = 10.5f;    // metros del mundo
    float yaw = 72.0f;     // grados: 0 = camara del lado del Sol (cara de dia a camara), 180 = el cuerpo contra el Sol (su cara de NOCHE a camara)
    float pitch = 18.0f;   // grados sobre el plano del sistema
    float lift = 0.0f;     // metros que sube el punto al que mira (el cuerpo queda mas abajo en el cuadro)
    float roll = 0.0f;     // grados de balanceo de la camara
    float fov = 40.0f;
    float aperture = 0.6f; // profundidad de campo (0 = todo nitido)
    float sunDist = 150.0f; // a cuanto de la Tierra SE VE el Sol; <= 0 = en su sitio, en el centro del sistema
    float moonOff = 90.0f;  // la fase de la Luna: ver arriba
    // EL ECLIPSE. Con `sunAlign` en 1 el Sol NO esta donde dice `sunDist` sino sobre el rayo que sale de la camara y pasa por el punto
    // al que mira (la Luna), a la misma distancia: queda exactamente detras de ella visto desde ahi, sin importar la altura de la
    // camara. `sunSlide` lo corre de lado, en grados vistos desde la camara: es lo que hace que la Luna cruce el Sol (positivo = a la
    // derecha de ella).
    float sunAlign = 0.0f;
    float sunSlide = 0.0f;
    float sunScale = 1.0f; // el tamano del Sol (y sus anillos, y el cinturon) respecto al de la escena
};

struct CineShot {
    float dur = 8.0f;
    bool imax = false;
    BodyId view = BodyId::Earth;
    int beam = 0;           // el hilo de luz: 0 ninguno, 1 de la Tierra a la Luna, 2 de la Tierra hacia el Sol
    // COMO SE ENTRA a esta toma desde la anterior: 0 = el negro de un cuerpo que llena la pantalla (con su apagon), 1 = un destello (algo que
    // nace con la musica y deslumbra), 2 = corte por FORMA, sin tapar nada (el disco del Sol de una toma pasa a ser el de la siguiente).
    int cutIn = 0;
    float glowAt = -1.0f;   // en que punto de la toma (0..1) la corona del Sol se abre y brilla (el eclipse); < 0 = nunca
    bool moonDance = false; // la Luna sale de su orbita, da una vuelta alrededor del Sol que se ve y vuelve (ver `CineWorldOf`)
    bool sunBridge = false; // el Sol va de donde estaba al empezar la toma a donde estara al terminar (empalmar sin alinear con alineado)
    std::vector<CineKey> keys;
};

constexpr float kCineBar = 3.43f;

// El constructor de claves: lo que se escribe es lo que se lee en el guion, en el mismo orden.
inline CineKey CineK(float at, CineAnchor anchor, CineAnchor look, float dist, float yaw, float pitch, float lift, float roll, float fov,
                     float aperture, float sunDist, float moonOff) {
    CineKey k;
    k.at = at; k.anchor = anchor; k.look = look; k.dist = dist; k.yaw = yaw; k.pitch = pitch; k.lift = lift; k.roll = roll;
    k.fov = fov; k.aperture = aperture; k.sunDist = sunDist; k.moonOff = moonOff;
    return k;
}

// EL GUION: EL ULTIMO BAILE. La Tierra y la Luna bailan una alrededor de la otra por ultima vez, con el Sol (ya rodeado de sus anillos de
// Dyson: la obra esta terminada, es el final de la eternidad) como la luz del salon; y en el medio, la Luna sale a bailar con el Sol y le da
// una vuelta entera, antes de cubrirlo en el eclipse. Precious moon y precious sun: el eclipse es el abrazo de los dos, y por eso brilla.
//
// LAS TRANSICIONES. Un corte solo existe para cambiar de cuerpo (`cutIn` 0: la camara entra en la Luna hasta que todo es negro y sale de la
// Tierra), o para pasar del negro de un cuerpo al negro del ESPACIO (`cutIn` 2, sin apagon: la Luna oscura llena el cuadro y, sin darse
// cuenta, ya se esta mirando el cielo vacio). Las tomas 3, 4 y 5 son UNA sola toma partida en tres tramos que se empalman sin corte (`cutIn`
// 2 con la misma pose en el empalme), para poder dar a cada tramo su propio mundo: en el baile la Luna gira alrededor del Sol.
//
// Las duraciones salen del COMPAS de la cancion (3,43 s, a ~70 pulsos): cada corte cae en una linea de compas. El eclipse cae en el golpe
// fuerte de 45,53 s. El total es 67 s: la letra sigue despues del cambio de seccion de 62,68, y el fundido de salida va despues de ella.

inline const std::vector<CineShot>& CineScript() {
    static const std::vector<CineShot> script = [] {
        using A = CineAnchor;
        std::vector<CineShot> s;
        CineShot c;
        // El Sol del tramo central: mas chico (radio 2 en vez de 5) y mas cerca (24,5 de la Tierra en vez de 62): se ve EXACTAMENTE del
        // mismo tamano, pero la Luna puede bailar a su alrededor en una orbita que cabe en el cuadro. Sus anillos y el cinturon se escalan con el.
        constexpr float kNearSun = 24.5f, kSmallSun = 0.4f;
        const auto smallSun = [&](CineShot& sh, float align) {
            for (CineKey& k : sh.keys) { k.sunScale = kSmallSun; k.sunAlign = align; }
        };

        // 1. 16:9 - LA TIERRA (0 - 14,66). Del negro del lado de noche la camara se separa y gira hasta que el Sol, con sus anillos, asoma por
        // el borde; se da vuelta a buscar a la Luna, que esta detras de ella, y va a su encuentro: la rodea por el borde hasta su lado oscuro.
        c = {}; c.dur = 7.80f + 2 * kCineBar; c.imax = false; c.view = BodyId::Earth;
        c.keys = {CineK(0.00f, A::Earth, A::Earth, 2.2f, 180.0f, 3.0f, 0.0f, 0.0f, 40.0f, 0.40f, 46.0f, 160.0f),
                  CineK(0.32f, A::Earth, A::Earth, 6.0f, 152.0f, 9.0f, 0.9f, -3.0f, 40.0f, 0.50f, 46.0f, 162.0f),
                  CineK(0.62f, A::Earth, A::Moon, 7.0f, 140.0f, 11.0f, 0.0f, 2.0f, 40.0f, 0.40f, 46.0f, 165.0f),
                  CineK(1.00f, A::Moon, A::Moon, 0.8f, 180.0f, 5.0f, 0.0f, 3.0f, 40.0f, 0.30f, 46.0f, 170.0f)};
        s.push_back(c);

        // 2. IMAX - EL VALS (14,66 - 28,38). Sale de la Tierra (de su negro) y baja por debajo de las dos: la Luna da la vuelta alrededor de la
        // Tierra, unidas por el hilo de luz, y la camara gira con ellas. Al final se acerca a la Luna (enfocada, la Tierra desenfocada detras)
        // hasta que su lado oscuro llena el cuadro.
        c = {}; c.dur = 4 * kCineBar; c.imax = true; c.view = BodyId::Earth; c.beam = 1; c.cutIn = 0;
        c.keys = {CineK(0.00f, A::Earth, A::Earth, 2.2f, 180.0f, 3.0f, 0.0f, 0.0f, 40.0f, 0.40f, 46.0f, 170.0f),
                  CineK(0.25f, A::Earth, A::Mid, 10.0f, 140.0f, -30.0f, 0.0f, -3.0f, 44.0f, 0.40f, 46.0f, 230.0f),
                  CineK(0.62f, A::Mid, A::Mid, 12.0f, 60.0f, -40.0f, 0.0f, 3.0f, 44.0f, 0.40f, 46.0f, 330.0f),
                  CineK(1.00f, A::Moon, A::Moon, 0.8f, 180.0f, 5.0f, 0.0f, 3.0f, 40.0f, 0.30f, 46.0f, 420.0f)};
        s.push_back(c);

        // 3. 16:9 - EL CIELO (28,38 - 35,24). Del negro de la Luna al negro del espacio, sin apagon: la camara mira el cielo vacio, sobre la
        // Tierra, y baja hasta su borde de noche; el Sol con sus anillos amanece detras, y la Luna espera cerca de el.
        c = {}; c.dur = 2 * kCineBar; c.imax = false; c.view = BodyId::Earth; c.cutIn = 2;
        c.keys = {CineK(0.00f, A::Earth, A::Earth, 3.4f, 158.0f, 8.0f, 26.0f, -2.0f, 40.0f, 0.30f, kNearSun, 660.0f),
                  CineK(0.42f, A::Earth, A::Earth, 3.4f, 156.0f, 8.0f, 0.8f, 0.0f, 40.0f, 0.30f, kNearSun, 668.0f),
                  CineK(1.00f, A::Earth, A::SunSeen, 5.0f, 150.0f, 14.0f, 0.0f, 2.0f, 40.0f, 0.25f, kNearSun, 680.0f)};
        smallSun(c, 0.0f);
        s.push_back(c);

        // 4. 16:9 - LA LUNA (35,24 - 42,10). Sin corte: la camara deja el Sol y enfoca a la Luna, que sigue su orbita (nada mas: es ella la que
        // va llegando, naturalmente, a ponerse delante del Sol); la camara sube despacio sobre el borde de la Tierra hasta el plano del eclipse.
        // (Se probo que la Luna saliera a dar una vuelta alrededor del Sol, mirando desde arriba: las dos transiciones -subir y bajar- se leian
        // como la camara y la Luna yendose, no como un baile.)
        c = {}; c.dur = 2 * kCineBar; c.imax = false; c.view = BodyId::Earth; c.cutIn = 2; c.sunBridge = true;
        c.keys = {CineK(0.00f, A::Earth, A::SunSeen, 5.0f, 150.0f, 14.0f, 0.0f, 2.0f, 40.0f, 0.25f, kNearSun, 680.0f),
                  CineK(0.45f, A::Earth, A::Moon, 4.2f, 70.0f, 40.0f, 0.0f, 0.0f, 34.0f, 0.20f, kNearSun, 700.0f),
                  CineK(1.00f, A::Earth, A::Moon, 2.9f, 6.0f, 62.0f, 0.0f, -3.0f, 26.0f, 0.15f, kNearSun, 720.0f)};
        smallSun(c, 0.0f);
        c.keys[0].sunSlide = -26.0f; c.keys[1].sunSlide = -20.0f; c.keys[2].sunSlide = -11.0f; // empalma con el -11 del eclipse
        s.push_back(c);

        // 5. 16:9 - EL ECLIPSE (42,10 - 52,39). Sin corte: desde el borde de la Tierra, mirando a la Luna, el Sol le llega por un lado y queda
        // cubierto JUSTO en el golpe de la musica (45,53: un tercio del tramo), y ahi la corona se abre y brilla (`glowAt`). Despues lo deja
        // atras y la camara va hacia la Luna hasta su negro.
        c = {}; c.dur = 3 * kCineBar; c.imax = false; c.view = BodyId::Earth; c.cutIn = 2; c.glowAt = 1.0f / 3.0f;
        {
            // CAMARA LENTA: hasta el golpe el Sol llega rapido (11 -> 2 grados en 2,5 s) y despues casi se detiene: cruza los ultimos 2 grados,
            // el centro y 2 mas en casi 3 s, con la camara tambien casi quieta. El mundo entero va mas lento ahi (`kCineSlow*`): anillos,
            // giro de la Tierra, corona. Despues retoma y la camara va hacia la Luna.
            CineKey k0 = CineK(0.00f, A::Earth, A::Moon, 2.9f, 6.0f, 62.0f, 0.0f, -3.0f, 26.0f, 0.15f, kNearSun, 720.0f);
            CineKey k1 = CineK(0.25f, A::Earth, A::Moon, 2.9f, 1.5f, 62.0f, 0.0f, -1.0f, 24.5f, 0.15f, kNearSun, 720.0f);
            CineKey k2 = CineK(0.333f, A::Earth, A::Moon, 2.9f, 0.0f, 62.0f, 0.0f, 0.0f, 24.0f, 0.15f, kNearSun, 720.0f);
            CineKey k3 = CineK(0.53f, A::Earth, A::Moon, 2.9f, -1.0f, 61.0f, 0.0f, 1.0f, 23.0f, 0.15f, kNearSun, 720.0f);
            CineKey k4 = CineK(0.76f, A::Earth, A::Moon, 3.0f, -4.0f, 58.0f, 0.0f, 3.0f, 22.0f, 0.15f, kNearSun, 720.0f);
            CineKey k5 = CineK(1.00f, A::Moon, A::Moon, 0.8f, 180.0f, 4.0f, 0.0f, 5.0f, 34.0f, 0.15f, kNearSun, 720.0f);
            // El Sol entra por la IZQUIERDA de la Luna (deslizamiento negativo) y sale por la derecha: es el MISMO sentido en que se mueven uno
            // respecto del otro en el tramo anterior (la Luna en su orbita), asi que en el empalme no hay un cambio de direccion.
            k0.sunSlide = -11.0f; k1.sunSlide = -2.0f; k2.sunSlide = 0.0f; k3.sunSlide = 2.0f; k4.sunSlide = 9.0f; k5.sunSlide = 20.0f;
            c.keys = {k0, k1, k2, k3, k4, k5};
        }
        smallSun(c, 1.0f);
        s.push_back(c);

        // 6. IMAX - EL FINAL (52,39 - 67,00). Sale de la Tierra y se aleja despacio hasta ver todo: la Tierra con su anillo, la Luna, y detras
        // el Sol con los suyos. El texto, y todo se apaga con la musica.
        c = {}; c.dur = 67.0f - 52.39f; c.imax = true; c.view = BodyId::Earth; c.cutIn = 0;
        c.keys = {CineK(0.00f, A::Earth, A::Earth, 2.2f, 180.0f, 3.0f, 0.0f, 0.0f, 40.0f, 0.40f, 46.0f, 100.0f),
                  CineK(0.35f, A::Earth, A::Earth, 8.5f, 158.0f, 10.0f, 0.0f, -2.0f, 40.0f, 0.40f, 46.0f, 112.0f),
                  CineK(1.00f, A::Earth, A::Earth, 16.0f, 162.0f, 13.0f, 0.0f, 0.0f, 40.0f, 0.40f, 46.0f, 140.0f)};
        s.push_back(c);
        return s;
    }();
    return script;
}

// LO CONSTRUIDO: todo esta desde el primer frame (corte 0 = el segundo 0). Es el ultimo baile de la eternidad: la obra ya termino, y nada
// aparece a la vista. Atado a cortes por si algun dia se quiere volver a hacer crecer algo, escondido en uno.
struct CineTechStep {
    int cut;          // el corte en que nace (0 = desde el principio)
    const char* tech; // nullptr = solo la cuenta de satelites
    int satellites;
};
inline const std::vector<CineTechStep>& CineTechSteps() {
    static const std::vector<CineTechStep> steps = {
        {0, "orbital_ring", 22},         {0, "orbital_assembly", 22}, {0, "orbital_hub", 22},
        {0, "space_solar_arrays", 22},   {0, "collector_ring", 22},   {0, "solar_assembly", 22},
        {0, "solar_collector_swarm", 22},
    };
    return steps;
}

// El segundo del corte `n` (1 = el final de la primera toma).
inline float CineCutTime(int n) {
    float t = 0.0f;
    const std::vector<CineShot>& script = CineScript();
    for (int i = 0; i < n && i < static_cast<int>(script.size()); ++i) t += script[static_cast<std::size_t>(i)].dur;
    return t;
}
// Cuantos pasos del calendario ya ocurrieron en el segundo `t` (GameRoot lo compara para saber si hay que reaplicar).
inline int CineTechStage(float t) {
    int n = 0;
    for (const CineTechStep& s : CineTechSteps())
        if (t >= CineCutTime(s.cut)) ++n;
    return n;
}

inline float CineLength() {
    float t = 0.0f;
    for (const CineShot& s : CineScript()) t += s.dur;
    return t;
}

// Lo que la toma pide en un segundo concreto.
struct CineFrame {
    float t = 0.0f;
    int shot = 0;
    float shotT = 0.0f; // segundos dentro de la toma
    float shotX = 0.0f; // 0..1 dentro de la toma
    BodyId view = BodyId::Earth;
    int beam = 0;
    CineKey k;           // la clave ya interpolada (los numeros; las anclas se resuelven con `CinePoseOf`)
    float ratio = 16.0f / 9.0f; // ancho/alto de lo que se ve (las barras cubren el resto)
    float edge = 0.0f;   // 0..1: el filo tenue de las barras mientras se deslizan (contra el negro del espacio no se verian)
    float fade = 0.0f;   // 0 = imagen, 1 = negro (los fundidos de entrada y salida y el apagon de cada corte)
    float flash = 0.0f;  // 0..1: el destello calido de los cortes que nacen con algo nuevo
    float glow = 0.0f;   // 0..1: cuanto se abre la corona del Sol (el eclipse especial)
    float text = 0.0f;   // 0..1: cuanto se ve la dedicatoria
    float orbitDeg = 0.0f, spinDeg = 0.0f;
    float vt = 0.0f;     // el tiempo del MUNDO (giros, anillos, corona): igual a `t` salvo en la camara lenta del eclipse, donde va mas despacio
};

inline float CineSmooth(float x) {
    x = std::clamp(x, 0.0f, 1.0f);
    return x * x * (3.0f - 2.0f * x);
}
inline float CineLerp(float a, float b, float u) { return a + (b - a) * u; }

constexpr float kCineImaxRatio = 1.43f;
constexpr float kCineWideRatio = 16.0f / 9.0f;
constexpr float kCineBarsSeconds = 1.0f;  // lo que tardan las barras en llegar al formato de la toma nueva
constexpr float kCineFadeIn = 1.4f;
constexpr float kCineFadeOut = 4.0f;      // el mismo largo que el fundido del audio
constexpr float kCineDip = 0.20f;         // medio ancho del apagon que cubre cada corte
constexpr float kCineFlash = 0.34f;       // medio ancho del destello

// La curva de UN numero por las claves de una toma: Hermite con tangentes por diferencias (Catmull-Rom para claves desiguales). En las
// puntas la tangente es la pendiente del primer/ultimo tramo, o sea que la toma NO frena en sus extremos. `get` saca el numero de una clave.
template <class G>
float CineSpline(const std::vector<CineKey>& k, float x, G get) {
    const int n = static_cast<int>(k.size());
    if (n == 1) return get(k[0]);
    int i = 0;
    while (i + 2 < n && x >= k[static_cast<std::size_t>(i + 1)].at) ++i;
    const auto p = [&](int j) { return get(k[static_cast<std::size_t>(j)]); };
    const auto at = [&](int j) { return k[static_cast<std::size_t>(j)].at; };
    const auto slope = [&](int j) {
        if (j == 0) return (p(1) - p(0)) / std::max(1e-4f, at(1) - at(0));
        if (j == n - 1) return (p(n - 1) - p(n - 2)) / std::max(1e-4f, at(n - 1) - at(n - 2));
        return (p(j + 1) - p(j - 1)) / std::max(1e-4f, at(j + 1) - at(j - 1));
    };
    const float h = std::max(1e-4f, at(i + 1) - at(i));
    const float t = std::clamp((x - at(i)) / h, 0.0f, 1.0f), t2 = t * t, t3 = t2 * t;
    return (2 * t3 - 3 * t2 + 1) * p(i) + (t3 - 2 * t2 + t) * h * slope(i) + (-2 * t3 + 3 * t2) * p(i + 1) + (t3 - t2) * h * slope(i + 1);
}

// LA CAMARA LENTA DEL ECLIPSE: entre estos segundos el mundo avanza a `kCineSlowRate` de su ritmo (con rampas suaves). Solo el mundo: la
// musica y los cortes siguen en tiempo real, asi que el eclipse dura mas en pantalla sin mover un solo compas.
constexpr float kCineSlowFrom = 44.6f, kCineSlowTo = 49.2f, kCineSlowRamp = 0.8f, kCineSlowRate = 0.25f;
inline float CineSlowRate(float t) {
    const float in = CineSmooth((t - kCineSlowFrom) / kCineSlowRamp), out = 1.0f - CineSmooth((t - kCineSlowTo) / kCineSlowRamp);
    return 1.0f - (1.0f - kCineSlowRate) * std::min(in, out);
}
inline float CineVisualTime(float t) {
    if (t <= kCineSlowFrom) return t;
    float v = kCineSlowFrom;
    const float end = std::min(t, kCineSlowTo + kCineSlowRamp);
    const float step = 0.01f;
    for (float x = kCineSlowFrom; x < end; x += step) v += CineSlowRate(x + step * 0.5f) * std::min(step, end - x);
    return v + std::max(0.0f, t - end);
}

inline CineFrame CineEvaluate(float t) {
    const std::vector<CineShot>& script = CineScript();
    const float length = CineLength();
    t = std::max(0.0f, t);
    CineFrame f;
    f.t = t;

    float start = 0.0f;
    int i = 0;
    for (; i + 1 < static_cast<int>(script.size()); ++i) {
        if (t < start + script[static_cast<std::size_t>(i)].dur) break;
        start += script[static_cast<std::size_t>(i)].dur;
    }
    const CineShot& s = script[static_cast<std::size_t>(i)];
    f.shot = i;
    f.shotT = std::min(t - start, s.dur);
    f.shotX = f.shotT / s.dur;
    const float x = f.shotX;
    f.view = s.view;
    f.beam = s.beam;
    const auto& K = s.keys;
    f.k.dist = CineSpline(K, x, [](const CineKey& k) { return k.dist; });
    f.k.yaw = CineSpline(K, x, [](const CineKey& k) { return k.yaw; });
    f.k.pitch = CineSpline(K, x, [](const CineKey& k) { return k.pitch; });
    f.k.lift = CineSpline(K, x, [](const CineKey& k) { return k.lift; });
    f.k.roll = CineSpline(K, x, [](const CineKey& k) { return k.roll; });
    f.k.fov = CineSpline(K, x, [](const CineKey& k) { return k.fov; });
    f.k.aperture = std::max(0.0f, CineSpline(K, x, [](const CineKey& k) { return k.aperture; }));
    f.k.sunDist = CineSpline(K, x, [](const CineKey& k) { return k.sunDist; });
    f.k.moonOff = CineSpline(K, x, [](const CineKey& k) { return k.moonOff; });
    f.k.sunAlign = CineSpline(K, x, [](const CineKey& k) { return k.sunAlign; });
    f.k.sunSlide = CineSpline(K, x, [](const CineKey& k) { return k.sunSlide; });
    f.k.sunScale = CineSpline(K, x, [](const CineKey& k) { return k.sunScale; });
    // EL BRILLO DEL ECLIPSE: sube en un tercio de segundo hasta el golpe y se apaga despacio (la luz que queda).
    if (s.glowAt >= 0.0f) {
        const float d = f.shotT - s.glowAt * s.dur;
        f.glow = d < 0.0f ? CineSmooth((d + 0.35f) / 0.35f) : std::exp(-d / 1.8f);
    }

    // Las barras: del formato de la toma anterior al de esta.
    const float prev = i > 0 ? (script[static_cast<std::size_t>(i - 1)].imax ? kCineImaxRatio : kCineWideRatio) : (s.imax ? kCineImaxRatio : kCineWideRatio);
    const float cur = s.imax ? kCineImaxRatio : kCineWideRatio;
    f.ratio = CineLerp(prev, cur, CineSmooth(f.shotT / kCineBarsSeconds));
    // Solo hay filo si el formato CAMBIA con este corte, y se apaga un poco despues de que las barras llegan.
    f.edge = prev != cur ? 1.0f - CineSmooth(f.shotT / (kCineBarsSeconds * 2.2f)) : 0.0f;

    // Los fundidos de entrada y de salida, y el apagon de cada corte (el cuerpo que llena la pantalla hace casi todo el trabajo; esto cubre
    // lo que le falte). La imagen misma se apaga, asi que el video no depende de un efecto de edicion.
    float dip = 0.0f;
    float cut = 0.0f;
    for (std::size_t j = 0; j + 1 < script.size(); ++j) {
        cut += script[j].dur;
        const int kind = script[j + 1].cutIn;
        if (kind == 0) dip = std::max(dip, 1.0f - CineSmooth(std::fabs(t - cut) / kCineDip));
        else if (kind == 1) f.flash = std::max(f.flash, 1.0f - CineSmooth(std::fabs(t - cut) / kCineFlash));
    }
    f.fade = std::max({1.0f - CineSmooth(t / kCineFadeIn), CineSmooth((t - (length - kCineFadeOut)) / kCineFadeOut), dip});
    // La dedicatoria entra a mitad de la ultima toma y se apaga con el negro.
    f.text = CineSmooth((t - (length - 9.0f)) / 1.6f) * (1.0f - f.fade);

    f.vt = CineVisualTime(t);
    f.orbitDeg = 20.0f + 0.8f * f.vt;
    f.spinDeg = 40.0f + 8.0f * f.vt;
    return f;
}

struct CineV3 {
    float x = 0, y = 0, z = 0;
};

// Donde esta cada cuerpo y a que angulo (grados) esta la Luna en su orbita. Las mismas constantes que SpaceWorld. Las rocas las pone
// quien las conozca (la escena): el codigo puro no sabe donde esta cada una.
struct CineWorld {
    CineV3 earth, moon, sun;
    CineV3 rock[2];
    float monthDeg = 0.0f;
    float moonScale = 1.0f; // la Luna crece un poco mientras baila con el Sol (para leerse desde arriba)
};
constexpr float kCineEarthOrbit = 30.0f, kCineMoonOrbit = 8.0f;
constexpr float kCineRad = 3.14159265f / 180.0f;

inline CineV3 CineAnchorPos(const CineWorld& w, CineAnchor a) {
    switch (a) {
        case CineAnchor::Moon: return w.moon;
        case CineAnchor::Sun: return {0.0f, 0.0f, 0.0f}; // el ancla del Sol es el centro del sistema (su sitio), no el Sol que se ve
        case CineAnchor::Mid: return {(w.earth.x + w.moon.x) * 0.5f, 0.0f, (w.earth.z + w.moon.z) * 0.5f};
        case CineAnchor::Rock0: return w.rock[0];
        case CineAnchor::Rock1: return w.rock[1];
        case CineAnchor::SunSeen: return w.sun;
        case CineAnchor::Earth: break;
    }
    return w.earth;
}

// La camara resuelta: donde esta, a donde mira y cuanto se balancea.
struct CinePose {
    CineV3 eye, target;
    float roll = 0.0f;
};

// Solo usa la Tierra, la Luna y las rocas del mundo (nunca el Sol que se ve), asi que se puede pedir antes de saber donde esta el Sol.
inline CinePose CinePoseOf(const CineFrame& f, const CineWorld& w) {
    const std::vector<CineKey>& K = CineScript()[static_cast<std::size_t>(f.shot)].keys;
    const float x = f.shotX;
    // Las posiciones del ancla y de la mirada: cada clave pone la suya (en el mundo de ESTE instante) y la curva las une.
    // El tramo de claves en que cae `x`: si sus dos claves usan el MISMO cuerpo (como ancla o como mirada), la posicion es la de ese cuerpo,
    // exacta. La curva sola se desviaba: su tangente mira a las claves vecinas, y una vecina que miraba al Sol corria la mirada de la Luna.
    int seg = 0;
    while (seg + 2 < static_cast<int>(K.size()) && x >= K[static_cast<std::size_t>(seg + 1)].at) ++seg;
    const CineKey& ka = K[static_cast<std::size_t>(seg)];
    const CineKey& kb = K[static_cast<std::size_t>(std::min<int>(seg + 1, static_cast<int>(K.size()) - 1))];
    const auto comp = [&](bool look, int axis) {
        const CineAnchor same = look ? ka.look : ka.anchor;
        if (same == (look ? kb.look : kb.anchor)) {
            const CineV3 q = CineAnchorPos(w, same);
            return axis == 0 ? q.x : axis == 1 ? q.y : q.z;
        }
        return CineSpline(K, x, [&](const CineKey& k) {
            const CineV3 p = CineAnchorPos(w, look ? k.look : k.anchor);
            return axis == 0 ? p.x : axis == 1 ? p.y : p.z;
        });
    };
    const CineV3 c = {comp(false, 0), comp(false, 1), comp(false, 2)};
    const CineV3 l = {comp(true, 0), comp(true, 1), comp(true, 2)};
    // EL ANGULO DE LA CAMARA alrededor del ancla es absoluto (azimut) = referencia + yaw, y se calcula POR CLAVE: la referencia es "hacia el
    // Sol (el centro del sistema)", salvo con el ancla EN el centro (el Sol), donde no hay "hacia el Sol" y se usa la direccion a la Tierra
    // (yaw 0 = "del lado de la Tierra"). Se desenvuelven de clave a clave (nunca saltan 360) y la curva pasa por esos angulos: una vuelta
    // entera alrededor de un cuerpo es un yaw que crece, y no se interpola por la cuerda.
    std::vector<CineKey> kk = K;
    float prevAz = 0.0f;
    for (std::size_t i = 0; i < kk.size(); ++i) {
        const CineV3 ci = CineAnchorPos(w, kk[i].anchor);
        const float ref = std::fabs(ci.x) + std::fabs(ci.z) < 0.5f ? std::atan2(w.earth.x, w.earth.z) : std::atan2(-ci.x, -ci.z);
        float az = ref / kCineRad + kk[i].yaw;
        if (i > 0) {
            while (az - prevAz > 180.0f) az -= 360.0f;
            while (az - prevAz < -180.0f) az += 360.0f;
        }
        kk[i].yaw = az;
        prevAz = az;
    }
    const float a = CineSpline(kk, x, [](const CineKey& k) { return k.yaw; }) * kCineRad, p = f.k.pitch * kCineRad;
    CinePose r;
    r.eye = {c.x + f.k.dist * std::cos(p) * std::sin(a), c.y + f.k.dist * std::sin(p), c.z + f.k.dist * std::cos(p) * std::cos(a)};
    // LA CAMARA NO ATRAVIESA NADA. Entre dos claves la curva puede cruzar un cuerpo (volar de un lado de la Luna al otro es una recta que
    // pasa por su centro); si el ojo cae dentro de la esfera de seguridad de la Tierra o la Luna, se saca por el radio: la camara se
    // desliza por la superficie y lo RODEA, que es lo que haria un vuelo de verdad. Las esferas son un poco mayores que los cuerpos (el aire
    // de la Tierra, un margen de la Luna); las inmersiones que llenan la pantalla (2,2 y 0,8) quedan justo fuera.
    const auto push = [&](const CineV3& b, float radius) {
        float dx = r.eye.x - b.x, dy = r.eye.y - b.y, dz = r.eye.z - b.z;
        const float len = std::sqrt(dx * dx + dy * dy + dz * dz);
        if (len >= radius) return;
        if (len < 1e-4f) { dx = 0.0f; dy = 1.0f; dz = 0.0f; } else { dx /= len; dy /= len; dz /= len; }
        r.eye = {b.x + dx * radius, b.y + dy * radius, b.z + dz * radius};
    };
    push(w.earth, 2.15f);
    push(w.moon, 0.62f);
    r.target = {l.x, l.y + f.k.lift, l.z};
    r.roll = f.k.roll;
    return r;
}

// El Sol del eclipse: sobre el rayo de la camara al punto que mira, a la distancia a la que lo pondria `sunDist`, corrido de lado
// `sunSlide` grados (rotando el rayo sobre el eje vertical).
inline CineV3 CineAlignedSun(const CineFrame& f, const CineWorld& w) {
    const CinePose p = CinePoseOf(f, w);
    // Distancia de la camara al Sol "en su sitio" (el que habria sin alinear).
    const float len = std::max(0.001f, std::sqrt(w.earth.x * w.earth.x + w.earth.z * w.earth.z));
    const CineV3 nominal = {w.earth.x - w.earth.x / len * f.k.sunDist, 0.0f, w.earth.z - w.earth.z / len * f.k.sunDist};
    const float nx = nominal.x - p.eye.x, ny = nominal.y - p.eye.y, nz = nominal.z - p.eye.z;
    const float reach = std::sqrt(nx * nx + ny * ny + nz * nz); // (no `far`: es una macro de windows.h)
    float dx = p.target.x - p.eye.x, dy = p.target.y - p.eye.y, dz = p.target.z - p.eye.z;
    const float dl = std::max(0.001f, std::sqrt(dx * dx + dy * dy + dz * dz));
    dx /= dl; dy /= dl; dz /= dl;
    // Se gira alrededor del eje VERTICAL DE LA CAMARA (el "arriba" del mundo, hecho perpendicular al rayo), no del mundo: con la camara
    // mirando hacia abajo, girar sobre el vertical del mundo corria el Sol tambien de arriba a abajo en pantalla, y el movimiento de los dos
    // cuerpos hacia una curva con vuelta. Asi `sunSlide` es un corrimiento solo de lado.
    float ax = -dx * dy, ay = 1.0f - dy * dy, az = -dz * dy; // up - d * (up . d)
    const float al = std::max(1e-4f, std::sqrt(ax * ax + ay * ay + az * az));
    ax /= al; ay /= al; az /= al;
    const float th = f.k.sunSlide * kCineRad, cs = std::cos(th), sn = std::sin(th);
    // Rodrigues: d' = d cos + (a x d) sin (a y d son perpendiculares).
    const float cx = ay * dz - az * dy, cy = az * dx - ax * dz, cz = ax * dy - ay * dx;
    const float rx = dx * cs + cx * sn, ry = dy * cs + cy * sn, rz = dz * cs + cz * sn;
    return {p.eye.x + rx * reach, p.eye.y + ry * reach, p.eye.z + rz * reach};
}

inline CineWorld CineWorldOf(const CineFrame& f) {
    CineWorld w;
    const float o = f.orbitDeg * kCineRad;
    w.earth = {kCineEarthOrbit * std::cos(o), 0.0f, kCineEarthOrbit * std::sin(o)};
    // La direccion Tierra -> centro del sistema (donde esta el Sol de verdad) es la del angulo `orbit + 180`.
    w.monthDeg = f.orbitDeg + 180.0f + f.k.moonOff;
    const float m = w.monthDeg * kCineRad;
    w.moon = {w.earth.x + kCineMoonOrbit * std::cos(m), 0.0f, w.earth.z + kCineMoonOrbit * std::sin(m)};
    if (f.k.sunDist > 0.0f) {
        const float len = std::max(0.001f, std::sqrt(w.earth.x * w.earth.x + w.earth.z * w.earth.z));
        w.sun = {w.earth.x - w.earth.x / len * f.k.sunDist, 0.0f, w.earth.z - w.earth.z / len * f.k.sunDist};
        if (f.k.sunAlign > 0.5f) w.sun = CineAlignedSun(f, w);
    }
    // EL BAILE: la Luna sale de su orbita, da una vuelta entera alrededor del Sol que se ve y vuelve. El Sol de este tramo no se alinea con
    // la camara (lo que mira la camara depende de la Luna, y la Luna de el): va de donde estaba al empezar el tramo a donde estara al
    // terminar, asi los dos empalmes (con el tramo anterior y el siguiente) son exactos. La Luna entra y sale de la vuelta en el primer y el
    // ultimo quinto, mezclando su sitio de siempre con el de la vuelta.
    const CineShot& shot = CineScript()[static_cast<std::size_t>(f.shot)];
    // EL PUENTE DEL SOL (tramo 4): empieza donde lo dejo el tramo anterior (en su sitio, sin alinear) y termina atado a la Luna como en el
    // eclipse (alineado, `sunSlide` grados a su lado). El cambio se hace en el primer 40% del tramo, mientras la camara todavia mira al Sol y
    // al borde de la Tierra: cuando enfoca a la Luna, el Sol ya se mueve respecto de ella en el MISMO sentido que en el eclipse, sin vueltas.
    if (shot.sunBridge) {
        const CineV3 nominal = w.sun;
        const CineV3 aligned = CineAlignedSun(f, w);
        const float u = CineSmooth(f.shotX / 0.4f);
        w.sun = {CineLerp(nominal.x, aligned.x, u), CineLerp(nominal.y, aligned.y, u), CineLerp(nominal.z, aligned.z, u)};
        return w;
    }
    if (shot.moonDance) {
        const float start = CineCutTime(f.shot), end = start + shot.dur;
        const CineWorld a = CineWorldOf(CineEvaluate(start - 1e-3f));
        const CineWorld b = CineWorldOf(CineEvaluate(end + 1e-3f));
        const float u = CineSmooth(f.shotX);
        w.sun = {CineLerp(a.sun.x, b.sun.x, u), CineLerp(a.sun.y, b.sun.y, u), CineLerp(a.sun.z, b.sun.z, u)};
        if (!shot.moonDance) return w;
        const float radius = 4.2f * 5.0f * f.k.sunScale; // por fuera de los anillos
        const float th0 = std::atan2(a.moon.z - a.sun.z, a.moon.x - a.sun.x);
        const float th = th0 + 6.2831853f * CineSmooth((f.shotX - 0.10f) / 0.52f); // la vuelta termina a media toma: despues la camara baja
        const CineV3 orbit = {w.sun.x + radius * std::cos(th), w.sun.y, w.sun.z + radius * std::sin(th)};
        const float mix = CineSmooth(f.shotX / 0.18f) * CineSmooth((0.88f - f.shotX) / 0.22f); // en su sitio antes del empalme
        w.moon = {CineLerp(w.moon.x, orbit.x, mix), CineLerp(w.moon.y, orbit.y, mix), CineLerp(w.moon.z, orbit.z, mix)};
        w.moonScale = 1.0f + 2.0f * mix; // x3 en la vuelta: desde arriba, a su tamano real, era un punto
    }
    return w;
}

// ------------------------------------------------------------------ subtitulos
// Un .srt cualquiera (EVERGRAVE_CINE_SUBS=<ruta>): el texto lo pone quien lo trae; este codigo solo lo lee y lo muestra. La fuente del
// motor es ASCII, asi que lo que no lo sea se descarta (un acento sale como un hueco).
struct CineSub {
    float t0 = 0.0f, t1 = 0.0f;
    std::string text;
};

inline float CineSrtTime(const std::string& s) { // "HH:MM:SS,mmm"
    int h = 0, m = 0, sec = 0, ms = 0;
    if (std::sscanf(s.c_str(), "%d:%d:%d%*[,.]%d", &h, &m, &sec, &ms) < 3) return -1.0f;
    return static_cast<float>(h * 3600 + m * 60 + sec) + static_cast<float>(ms) / 1000.0f;
}

inline std::vector<CineSub> CineParseSrt(const std::string& text) {
    std::vector<CineSub> out;
    std::istringstream in(text);
    std::string line;
    CineSub cur;
    bool have = false;
    const auto flush = [&] {
        if (have && !cur.text.empty()) out.push_back(cur);
        cur = CineSub{};
        have = false;
    };
    while (std::getline(in, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();
        if (line.empty()) { flush(); continue; }
        const std::size_t arrow = line.find("-->");
        if (arrow != std::string::npos) {
            flush();
            cur.t0 = CineSrtTime(line.substr(0, arrow));
            cur.t1 = CineSrtTime(line.substr(arrow + 3));
            have = cur.t0 >= 0.0f && cur.t1 > cur.t0;
            continue;
        }
        if (!have) continue; // el numero de orden del bloque, o basura antes del primero
        std::string ascii;
        for (char ch : line) if (static_cast<unsigned char>(ch) >= 32 && static_cast<unsigned char>(ch) < 127) ascii += ch;
        if (ascii.empty()) continue;
        if (!cur.text.empty()) cur.text += '\n';
        cur.text += ascii;
    }
    flush();
    return out;
}

// Cuanto se ve un subtitulo en el segundo `t` (0 = nada): entra y sale en un cuarto de segundo.
inline float CineSubAlpha(const CineSub& s, float t) {
    return CineSmooth((t - s.t0) / 0.25f) * CineSmooth((s.t1 - t) / 0.25f);
}

// ------------------------------------------------------------------ el estado vivo
struct CineState {
    bool active = false;
    bool freeze = false;     // EVERGRAVE_CINE_FREEZE=1: el tiempo no avanza (para mirar una toma quieta)
    float t = 0.0f;
    std::string text;        // la dedicatoria (EVERGRAVE_CINE_TEXT), ASCII: la fuente no tiene acentos
    std::vector<CineSub> subs;
    CineFrame f;
    CineWorld world;
};

inline CineState& Cine() {
    static CineState s;
    return s;
}

// Lee las variables de entorno. Sin EVERGRAVE_CINE no hace nada.
inline void CineInit() {
    CineState& c = Cine();
    c = CineState{};
#ifdef _WIN32
    const auto env = [](const char* name, std::string& out) {
        char* buf = nullptr;
        size_t len = 0;
        const bool ok = _dupenv_s(&buf, &len, name) == 0 && buf;
        if (ok) out = buf;
        std::free(buf);
        return ok;
    };
    std::string v;
    if (!env("EVERGRAVE_CINE", v)) return;
    c.active = true;
    c.t = static_cast<float>(std::atof(v.c_str()));
    if (env("EVERGRAVE_CINE_FREEZE", v)) c.freeze = v != "0";
    c.text = "my precious moon"; // la dedicatoria de este regalo; EVERGRAVE_CINE_TEXT la cambia (vacia = ninguna)
    env("EVERGRAVE_CINE_TEXT", c.text);
    if (env("EVERGRAVE_CINE_SUBS", v)) {
        std::ifstream f{v, std::ios::binary};
        std::stringstream ss;
        ss << f.rdbuf();
        c.subs = CineParseSrt(ss.str());
    }
#endif
    c.f = CineEvaluate(c.t);
    c.world = CineWorldOf(c.f);
}

// Una vez por frame, ANTES de que nadie lea la toma (lo llama GameRoot, el primero).
inline void CineAdvance(float dt) {
    CineState& c = Cine();
    if (!c.active) return;
    // Las rocas son de la escena: se conservan las del frame anterior (CameraRig las escribe antes de pedir la camara).
    const CineV3 r0 = c.world.rock[0], r1 = c.world.rock[1];
    if (!c.freeze) c.t += dt;
    c.f = CineEvaluate(c.t);
    c.world = CineWorldOf(c.f);
    c.world.rock[0] = r0;
    c.world.rock[1] = r1;
}

} // namespace evergrave
