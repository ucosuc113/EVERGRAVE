#pragma once

// FxKit: lo que comparten WorldFx (la tienda) y EarthFx (el arbol de tecnologias de la Tierra): la camara del juego,
// los cuerpos vistos desde ella, los sprites, las orbitas y el eclipse. Es de UN solo archivo que lo incluye (WorldFx.cpp):
// por eso vive en un namespace anonimo. Ver la cabecera de WorldFx.cpp.

#include "../Services/FxBus.h"
#include "../Services/Session.h"
#include "UiKit.h"

#include "Engine/Diagnostics/Log.h"
#include "Engine/Rendering/Camera.h"
#include "Engine/Scene/GameObject.h"
#include "Engine/Scene/SceneQuery.h"
#include "Engine/Scene/SceneService.h"
#include "Engine/Scene/SceneTransform.h"
#include "Engine/Scripting/IScript.h"
#include "Engine/Scripting/ScriptRegistry.h"
#include "Engine/UI/UiService.h"

#include <DirectXMath.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <vector>

using namespace strace;
using namespace evergrave;
using namespace DirectX;

namespace {

constexpr const char* kEarthPath = "Space/OrbitPivot/EarthAnchor/Tilt/Earth";
constexpr const char* kMoonPath = "Space/OrbitPivot/EarthAnchor/Moon";
constexpr const char* kSunPath = "Space/OrbitPivot/Sun";
constexpr float kPi = 3.14159265f;

unsigned Hash(unsigned a, unsigned b = 0) {
    unsigned h = a * 374761393u + b * 668265263u + 0x9E3779B9u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}
float Hash01(unsigned a, unsigned b = 0) { return static_cast<float>(Hash(a, b) & 0xFFFFFF) / static_cast<float>(0x1000000); }

unsigned HashString(const std::string& s) {
    unsigned h = 2166136261u;
    for (char c : s) h = (h ^ static_cast<unsigned char>(c)) * 16777619u;
    return h;
}

// LA PALETA DE LOS EFECTOS. Todo lo que el mundo dibuja pasa por aqui (`ColorOf` y `Shaded`), asi que el color se
// decide en UN sitio. Antes cada cosa traia su color de la hoja (rojo, naranja, azul, blanco, todos a tope) y la
// imagen entera leia como sobresaturada: sin una jerarquia de color, nada es el protagonista. Ahora todo se acerca
// un 22% a su gris (`kFxSaturation`) y los picos de luz no pasan de un blanco calido: el blanco puro queda para
// lo que de verdad brilla (el nucleo de un destello). El rojo y el dorado siguen siendo rojo y dorado, pero ya no
// compiten con la Tierra.
constexpr float kFxSaturation = 0.78f;

// LA PROFUNDIDAD DE LO QUE SE DIBUJA AHORA. Los efectos con muchos colores y piezas (la estacion, el anillo, el enjambre del Sol)
// no se pueden reescribir uno por uno para que cada color sepa donde esta; en vez de eso, quien los dibuja dice una vez "esto esta
// lejos" (`g_mod`, ver `Cue` mas abajo) y todo color que pase por `Tame` hasta el aviso siguiente se enfria hacia el azul del
// espacio y se apaga. Lo cercano no cambia: el desenfoque es solo para lo que se redibuja entero como `Bokeh`.
struct FxMod {
    float fade = 1.0f; // lo que le queda de luz
    float haze = 0.0f; // cuanto tira al azul del espacio
};
FxMod g_mod;

UiColor Tame(float r, float g, float b, float a) {
    if (g_mod.haze > 0.0f) {
        r += (40.0f - r) * g_mod.haze;
        g += (60.0f - g) * g_mod.haze;
        b += (112.0f - b) * g_mod.haze;
    }
    a *= g_mod.fade;
    const float luma = 0.299f * r + 0.587f * g + 0.114f * b;
    const auto ch = [&](float v) { return static_cast<int>(std::clamp(luma + (v - luma) * kFxSaturation, 0.0f, 255.0f)); };
    return UiColor::Bytes(ch(r), ch(g), ch(b), static_cast<int>(255.0f * std::clamp(a, 0.0f, 1.0f)));
}

UiColor ColorOf(std::uint32_t rgb, float a = 1.0f) {
    return Tame(static_cast<float>((rgb >> 16) & 0xFF), static_cast<float>((rgb >> 8) & 0xFF), static_cast<float>(rgb & 0xFF), a);
}

// La camara del juego: del mundo a la pantalla, con la misma cuenta que el rayo del puntero.
struct Cam {
    XMMATRIX view, proj;
    float w = 0, h = 0;

    // Un punto del mundo a pixeles de juego. `vz` es su profundidad en la vista (mayor = mas lejos).
    bool Project(FXMVECTOR world, float& sx, float& sy, float& vz) const {
        const XMVECTOR v = XMVector3TransformCoord(world, view);
        vz = XMVectorGetZ(v);
        if (vz <= 0.05f) return false;
        const XMVECTOR c = XMVector3TransformCoord(v, proj);
        sx = (XMVectorGetX(c) * 0.5f + 0.5f) * w;
        sy = (0.5f - XMVectorGetY(c) * 0.5f) * h;
        return true;
    }
};

// Una esfera vista desde la camara: donde cae su centro, que tan grande se ve y a que profundidad.
struct Body {
    XMVECTOR center = XMVectorZero();
    float radius = 1.0f;
    float sx = 0, sy = 0, sr = 0, vz = 0;
    bool ok = false;
};

Body MakeBody(const Cam& cam, const GameObject& o) {
    Body b;
    const XMMATRIX m = ComputeWorldMatrix(o);
    b.center = XMVector3TransformCoord(XMVectorZero(), m);
    b.radius = XMVectorGetX(XMVector3Length(XMVectorSubtract(XMVector3TransformCoord(XMVectorSet(1, 0, 0, 0), m), b.center)));
    b.ok = cam.Project(b.center, b.sx, b.sy, b.vz);
    if (!b.ok) return b;
    // El borde: el centro mas el radio sobre el eje x de la VISTA (no del mundo): asi el
    // radio en pantalla es el del disco que se ve, sin importar hacia donde mire la camara.
    const XMVECTOR vc = XMVector3TransformCoord(b.center, cam.view);
    const XMVECTOR ce = XMVector3TransformCoord(XMVectorAdd(vc, XMVectorSet(b.radius, 0, 0, 0)), cam.proj);
    const float ex = (XMVectorGetX(ce) * 0.5f + 0.5f) * cam.w;
    b.sr = std::fabs(ex - b.sx);
    return b;
}

// Un punto que esta TAPADO por el cuerpo: detras de su centro y dentro de su disco.
bool Occluded(const Body& b, float sx, float sy, float vz) {
    if (vz <= b.vz) return false;
    const float dx = sx - b.sx, dy = sy - b.sy;
    return dx * dx + dy * dy < b.sr * b.sr;
}

// REGLA: NADA FLOTA SOLO. Un punto de un pixel con menos de `kMinSpeck` de opacidad no se dibuja: sobre el negro es una mota que no
// pertenece a nada (las estelas, los anillos de puntos y los trenes de carga dejaban decenas por cuadro, y eran la mitad del "confeti" de
// la imagen). Lo que de verdad es un punto - un nodo, una baliza - es de 2 pixeles o mas, o tiene su halo.
constexpr float kMinSpeck = 0.22f;

void Dot(UiDrawList& ui, float x, float y, float size, const UiColor& c) {
    if (size <= 1.0f && c.a < kMinSpeck) return;
    ui.AddRect(kit::Rect(std::floor(x - size * 0.5f), std::floor(y - size * 0.5f), size, size), c);
}

// ------------------------------------------------------------------ sprites
// Cada cosa que se compra tiene SILUETA, no solo color: un satelite son dos paneles y un cuerpo, un espejo es un
// hexagono, un relay una cruz. Son dibujos de pixeles (como los glifos de PixelFont): '.' es vacio, 'a' el color de
// la unidad, 'b' el mismo mas oscuro, 'w' blanco, 'e' blanco rojizo. Un rectangulo por pixel: el batch es el mismo
// de los puntos de antes, asi que cientos cuestan lo mismo.
struct Sprite {
    int w, h;
    const char* rows[5];
};
const Sprite kSatS{5, 3, {"aa.aa", "aawaa", "aa.aa"}};
const Sprite kSatL{7, 4, {"...w...", "aaa.aaa", "aaawaaa", "aaa.aaa"}}; // con 50 o mas: el modelo grande, con antena
const Sprite kMirror{5, 3, {".aaa.", "waaaa", ".aaa."}};
const Sprite kRelay{3, 3, {".a.", "aea", ".a."}};
const Sprite kProbe{3, 3, {"a.a", ".w.", "a.a"}};
const Sprite kCollector{2, 2, {"aa", "ab"}};
const Sprite kDome{3, 2, {".a.", "aaa"}};
const Sprite kSail{3, 3, {"a..", "aa.", "aaa"}};
const Sprite kTower{3, 4, {".w.", ".a.", ".a.", "aaa"}}; // la torre de acero de una plataforma de lanzamiento
const Sprite kTug{3, 2, {"aaa", "f.."}};                    // un remolcador: casco y llama azul
const Sprite kStationCore{3, 3, {".a.", "aaa", ".a."}};      // el nucleo de la estacion
const Sprite kLabModule{2, 2, {"aa", "aa"}};                 // un modulo
const Sprite kRock{3, 2, {"aaa", ".aa"}};                         // una roca capturada
const Sprite kNet{5, 3, {"a.a.a", ".aaa.", "a.a.a"}};               // la red que atrapa la carga
const Sprite kDish{7, 3, {"..aaa..", ".aaaaa.", "...a..."}};         // un plato grande: el telescopio
const Sprite kWing{11, 3, {"pppp.g.pppp", "ppppgggpppp", "pppp.g.pppp"}}; // el panel solar espacial: alas azules y un cuerpo gris

// Que dibujo lleva cada unidad. Una que no este aqui (una nueva de la hoja) cae en el punto de siempre segun su Look.
enum class Art { Default, Sat, Mirror, Relay, Probe, Collector, Dome, Sail, Mast, Plume, Solar, LaserGrid, City, Beam, Pit, Forge, Elevator, Ship };
Art ArtOf(const std::string& id) {
    struct E {
        const char* id;
        Art art;
    };
    static const E table[] = {
        {"weather_station", Art::Mast},    {"satellite", Art::Sat},           {"steam_plant", Art::Plume},       {"solar_farm", Art::Solar},
        {"laser_relay", Art::Relay},       {"ground_station", Art::Beam},     {"laser_array", Art::LaserGrid},   {"smart_city", Art::City},
        {"orbital_mirror", Art::Mirror},   {"space_elevator", Art::Elevator}, {"mercury_mirrors", Art::Mirror},  {"asteroid_miner", Art::Ship},
        {"regolith_miner", Art::Pit},      {"lunar_foundry", Art::Forge},     {"mass_driver", Art::Ship},        {"lunar_solar_farm", Art::Collector},
        {"solar_probe", Art::Probe},       {"solar_catcher", Art::Collector}, {"observatory_dome", Art::Dome},   {"fission_reactor", Art::Dome},
        {"lunar_dome", Art::Dome},         {"sail_fleet", Art::Sail},         {"cislunar_freighter", Art::Ship}, {"statite", Art::Collector},
    };
    for (const E& e : table)
        if (id == e.id) return e.art;
    return Art::Default;
}

UiColor Shaded(std::uint32_t rgb, float shade, float a) {
    auto ch = [&](int shift) { return std::clamp(static_cast<float>((rgb >> shift) & 0xFF) * shade, 0.0f, 255.0f); };
    return Tame(ch(16), ch(8), ch(0), a);
}

void DrawSprite(UiDrawList& ui, const Sprite& s, float cx, float cy, std::uint32_t body, float shade, float a) {
    const float x0 = std::floor(cx - static_cast<float>(s.w) * 0.5f), y0 = std::floor(cy - static_cast<float>(s.h) * 0.5f);
    for (int r = 0; r < s.h; ++r)
        for (int c = 0; c < s.w; ++c) {
            const char ch = s.rows[r][c];
            if (ch == '.') continue;
            const std::uint32_t rgb = ch == 'w' ? 0xFFFFFF : ch == 'e' ? 0xFFD8D8 : ch == 'f' ? 0x80B0FF : ch == 'g' ? 0xC0C8D8 : ch == 'p' ? 0x2A4A8A : body;
            ui.AddRect(kit::Rect(x0 + static_cast<float>(c), y0 + static_cast<float>(r), 1.0f, 1.0f), Shaded(rgb, ch == 'b' ? shade * 0.55f : shade, a));
        }
}

// ------------------------------------------------------------------ la mirada del cine: brillo y enfoque
// El motor da brillo (bloom), enfoque (profundidad de campo) y ajuste de color a la ESCENA, pero todo lo que este
// archivo dibuja es UI y la UI va encima de eso: no le llega nada. Asi que los efectos hacen SU version, con la
// misma idea y las herramientas que tiene la UI (rectangulos con alfa):
//   - `Halo`: el brillo. Discos concentricos y tenues bajo lo que emite luz; se suman por transparencia, como un bloom.
//   - `Bokeh`: el desenfoque. Lo que esta fuera del plano de enfoque no se dibuja nitido sino como un disco suave
//     con el borde un poco mas marcado, que es la forma de una lente de verdad.
//   - `DepthCue`: DONDE esta algo respecto al plano de enfoque (el del cuerpo que se mira) y que le toca: cuanto
//     desenfoque, cuanto se apaga, cuanto se enfria hacia el azul del espacio.
// Es lo que separa los planos: lo cercano grande y borroso, lo del plano de enfoque nitido y lo lejano chico y
// apagado. Sin esto, 200 satelites se dibujan igual de nitidos y del mismo tamano y se leen como UN plano.

// El plano de enfoque de este frame; lo fija WorldFx al empezar (el cuerpo que se mira: su profundidad y su radio).
struct FxFocus {
    float vz = 0.0f;
    float radius = 0.0f; // 0 = sin enfoque (todo nitido)
};
FxFocus g_focus;

struct Cue {
    float blur = 0.0f;  // 0 = nitido, 1 = del todo fuera de foco
    float fade = 1.0f;  // cuanto de su luz le queda (lo lejano se apaga)
    float haze = 0.0f;  // cuanto se enfria hacia el azul del espacio
    float depth = 0.0f; // en radios del cuerpo: negativo = delante del plano de enfoque (mas cerca de la camara)
    bool near = false;
};

float Smooth(float a, float b, float x) {
    const float t = std::clamp((x - a) / (b - a), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

Cue DepthCue(float vz) {
    Cue c;
    if (g_focus.radius <= 0.0f) return c;
    c.depth = (vz - g_focus.vz) / g_focus.radius;
    c.near = c.depth < 0.0f;
    c.blur = Smooth(0.7f, 1.9f, std::fabs(c.depth));
    // Lo de atras se enfria y se apaga (perspectiva atmosferica); lo de adelante conserva su luz: esta mas cerca.
    const float back = std::clamp(c.depth / 2.0f, 0.0f, 1.0f);
    c.fade = 1.0f - 0.5f * back;
    c.haze = 0.55f * back;
    return c;
}

FxMod ModOf(const Cue& c) { return FxMod{c.fade, c.haze}; }

// Un color que se enfria hacia el azul del espacio: la mitad lejana de la imagen tira al fondo.
std::uint32_t Hazed(std::uint32_t rgb, float haze) {
    if (haze <= 0.0f) return rgb;
    const auto mix = [&](int shift, int to) {
        const int v = static_cast<int>((rgb >> shift) & 0xFF);
        return static_cast<std::uint32_t>(v + static_cast<int>(static_cast<float>(to - v) * haze));
    };
    return (mix(16, 0x28) << 16) | (mix(8, 0x3C) << 8) | mix(0, 0x70);
}

// Un color mezclado hacia otro: `t` = 0 es `rgb`, 1 es `to`.
std::uint32_t Mixed(std::uint32_t rgb, std::uint32_t to, float t) {
    const auto mix = [&](int shift) {
        const int v = static_cast<int>((rgb >> shift) & 0xFF), w = static_cast<int>((to >> shift) & 0xFF);
        return static_cast<std::uint32_t>(v + static_cast<int>(static_cast<float>(w - v) * t));
    };
    return (mix(16) << 16) | (mix(8) << 8) | mix(0);
}

// Un disco de pixeles de radio `r`, por filas (un rectangulo por fila). Con `rim`, el borde sale un poco mas marcado.
void Disc(UiDrawList& ui, float cx, float cy, float r, const UiColor& fill, const UiColor* rim = nullptr) {
    cx = std::floor(cx);
    cy = std::floor(cy);
    const int R = std::max(1, static_cast<int>(std::floor(r + 0.5f)));
    for (int dy = -R; dy <= R; ++dy) {
        const float hw = std::sqrt(std::max(0.0f, r * r - static_cast<float>(dy * dy)));
        const int w = static_cast<int>(std::floor(hw + 0.5f));
        if (std::abs(dy) == R || w < 1) {
            if (rim) ui.AddRect(kit::Rect(cx - static_cast<float>(std::max(w, 0)), cy + static_cast<float>(dy), static_cast<float>(2 * std::max(w, 0) + 1), 1.0f), *rim);
            else if (w >= 1) ui.AddRect(kit::Rect(cx - static_cast<float>(w), cy + static_cast<float>(dy), static_cast<float>(2 * w + 1), 1.0f), fill);
            continue;
        }
        if (rim) {
            ui.AddRect(kit::Rect(cx - static_cast<float>(w - 1), cy + static_cast<float>(dy), static_cast<float>(2 * w - 1), 1.0f), fill);
            ui.AddRect(kit::Rect(cx - static_cast<float>(w), cy + static_cast<float>(dy), 1.0f, 1.0f), *rim);
            ui.AddRect(kit::Rect(cx + static_cast<float>(w), cy + static_cast<float>(dy), 1.0f, 1.0f), *rim);
        } else {
            ui.AddRect(kit::Rect(cx - static_cast<float>(w), cy + static_cast<float>(dy), static_cast<float>(2 * w + 1), 1.0f), fill);
        }
    }
}

// El brillo de algo que emite luz: tres discos tenues, uno dentro de otro. Se suman por transparencia, asi el centro
// queda mas claro y el borde se deshace, como un bloom. `r` es el alcance, en pixeles de juego.
void Halo(UiDrawList& ui, float x, float y, float r, std::uint32_t rgb, float a) {
    if (a <= 0.01f || r < 1.5f) return;
    Disc(ui, x, y, r, ColorOf(rgb, 0.07f * a));
    Disc(ui, x, y, r * 0.62f, ColorOf(rgb, 0.09f * a));
    Disc(ui, x, y, r * 0.30f, ColorOf(rgb, 0.14f * a));
}

// Lo que esta fuera del plano de enfoque: un disco suave con el borde un poco mas claro (un punto de luz
// desenfocado, como lo da una lente). `size` es el radio en foco; el desenfoque lo agranda.
void Bokeh(UiDrawList& ui, float x, float y, float size, float blur, std::uint32_t rgb, float a) {
    const float r = size + 0.5f + 2.5f * blur;
    // Relleno tenue y borde marcado: el anillo claro que dibuja una lente en un punto de luz fuera de foco. Un disco
    // parejo y apagado sobre el negro se leia como una mancha gris; con el borde se lee como luz.
    const float fill = (0.2f - 0.08f * blur) * a;
    const UiColor rim = ColorOf(rgb, std::min(1.0f, 0.7f * a));
    Disc(ui, x, y, r, ColorOf(rgb, fill), &rim);
}

// Un haz: un nucleo de un pixel y, debajo, una estela ancha y tenue. Un `AddLine` grueso se veia como una barra;
// asi se ve como luz. `core` es el color del nucleo (el centro de un laser es casi blanco).
void Beam(UiDrawList& ui, float x0, float y0, float x1, float y1, std::uint32_t rgb, float a, std::uint32_t core = 0xFFFFFF) {
    ui.AddLine(x0, y0, x1, y1, ColorOf(rgb, 0.22f * a), 3.0f);
    ui.AddLine(x0, y0, x1, y1, ColorOf(rgb, 0.85f * a), 1.0f);
    ui.AddLine(x0, y0, x1, y1, ColorOf(core, 0.35f * a), 1.0f);
}

// Un destello de un pixel de cruz: un panel que atrapa el Sol un instante.
void Glint(UiDrawList& ui, float x, float y, float a) {
    const UiColor c = UiColor::Bytes(255, 252, 244, static_cast<int>(255.0f * std::clamp(a, 0.0f, 1.0f)));
    Halo(ui, x, y, 5.0f, 0xFFF4DC, 0.7f * a);
    ui.AddRect(kit::Rect(std::floor(x) - 1.0f, std::floor(y), 3.0f, 1.0f), c);
    ui.AddRect(kit::Rect(std::floor(x), std::floor(y) - 1.0f, 1.0f, 3.0f), c);
}

// Un destello redondeado (una cruz de rectangulos con el centro blanco): el despegue y el aterrizaje. Un `Dot`
// grande se veia como una pegatina cuadrada.
void Flash(UiDrawList& ui, float x, float y, std::uint32_t rgb, float a) {
    x = std::floor(x);
    y = std::floor(y);
    Halo(ui, x, y, 7.0f, rgb, a);
    ui.AddRect(kit::Rect(x - 2.0f, y - 1.0f, 5.0f, 3.0f), ColorOf(rgb, 0.55f * a));
    ui.AddRect(kit::Rect(x - 1.0f, y - 2.0f, 3.0f, 5.0f), ColorOf(rgb, 0.55f * a));
    ui.AddRect(kit::Rect(x - 1.0f, y - 1.0f, 3.0f, 3.0f), ColorOf(0xFFF8EE, a));
}

// ------------------------------------------------------------------ orbitas
// La orbita `ring` de `n`: inclinada y girada distinto de las otras, cada una a su ritmo. Los radios se separan
// 8% (antes 7%, y 11% en una prueba que los dispersaba demasiado): con menos, todas las orbitas se cruzaban casi en el mismo punto y los puntos se amontonaban ahi.
struct Orbit {
    float radius, speed, tilt, node, phase;
};
Orbit OrbitOf(unsigned seed, int ring, float baseRadius, float spread = 0.08f) {
    Orbit o;
    o.radius = baseRadius * (1.0f + spread * static_cast<float>(ring));
    o.speed = (0.55f / (1.0f + 0.25f * static_cast<float>(ring))) * (ring % 2 == 0 ? 1.0f : -1.0f);
    o.tilt = (0.35f + 0.22f * static_cast<float>(ring % 3)) * (ring % 2 == 0 ? 1.0f : -1.0f);
    o.node = 1.05f * static_cast<float>(ring) + Hash01(seed, static_cast<unsigned>(ring)) * 0.6f;
    o.phase = Hash01(seed, 100u + static_cast<unsigned>(ring)) * 2.0f * kPi;
    return o;
}

// Un punto de la orbita, en el mundo, alrededor de `center`.
XMVECTOR OrbitPoint(const XMVECTOR& center, const Orbit& o, float angle) {
    XMVECTOR p = XMVectorSet(o.radius * std::cos(angle), 0.0f, o.radius * std::sin(angle), 0.0f);
    p = XMVector3TransformNormal(p, XMMatrixRotationX(o.tilt) * XMMatrixRotationY(o.node));
    return XMVectorAdd(center, p);
}

// Donde esta la unidad numero `j` de `shown`: la misma cuenta para dibujarla y para que un relay le tire un pulso.
struct Slot {
    XMVECTOR pos;
    Orbit orbit;
    float angle;
};
Slot OrbitSlot(const XMVECTOR& center, float base, unsigned seed, int shown, int j, float time, float spread = 0.08f) {
    const int rings = std::clamp(1 + shown / 8, 1, 6);
    const int ring = j % rings;
    const int inRing = (shown - ring + rings - 1) / rings;
    const int idx = j / rings;
    Slot s;
    s.orbit = OrbitOf(seed, ring, base, spread);
    s.angle = s.orbit.phase + s.orbit.speed * time + 2.0f * kPi * static_cast<float>(idx) / static_cast<float>(std::max(1, inRing));
    s.pos = OrbitPoint(center, s.orbit, s.angle);
    return s;
}

// La sombra del planeta: un punto detras de el, respecto al Sol, y dentro de su cilindro de sombra esta en eclipse.
// 1 = a pleno sol, 0,3 = en la sombra, con una penumbra delgada.
float Eclipse(const XMVECTOR& p, const XMVECTOR& c, const XMVECTOR& toSun, float radius) {
    const XMVECTOR r = XMVectorSubtract(p, c);
    const float d = XMVectorGetX(XMVector3Dot(r, toSun));
    if (d >= 0.0f) return 1.0f;
    const float dist = XMVectorGetX(XMVector3Length(XMVectorSubtract(r, XMVectorScale(toSun, d))));
    if (dist < radius) return 0.3f;
    if (dist < radius * 1.12f) return 0.3f + 0.7f * (dist - radius) / (radius * 0.12f);
    return 1.0f;
}

// LA ALTURA DE CADA FAMILIA. Antes los satelites, los relays y los espejos compartian la misma banda (de 1,2 a 1,7 radios) y
// se mezclaban en una nube sin estructura. Ahora cada una tiene su CAPA, como en el espacio de verdad: los satelites
// pegados al planeta (orbita baja, bien juntos), los relays por encima (los que hablan con los de abajo), y los espejos
// altos y separados. De lado se leen como tres cascaras y no como una nube. Alrededor del Sol y de la Luna no cambia nada.
// La usan WorldFx (lo que queda en UI) y Fleet (lo que ya es de la escena): las dos tienen que dar la MISMA posicion.
struct Band {
    float base, spread;
};
Band BandOf(Art art, bool aroundEarth) {
    if (!aroundEarth) return {1.2f, 0.08f};
    switch (art) {
        case Art::Sat: return {1.16f, 0.045f};
        case Art::Relay: return {1.50f, 0.05f};
        case Art::Mirror: return {1.95f, 0.09f};
        default: return {1.3f, 0.08f};
    }
}
// LAS FORMACIONES. Lo que se compra en cantidad no se reparte parejo por una orbita (eso se lee como ruido): se ORGANIZA, como en el espacio de
// verdad, y la estructura crece con la cantidad.
//   - Satelites: TRENES de `kTrainSize`, uno detras de otro con un hueco fijo (la constelacion que se ve como un collar de luces), cada tren
//     en su propio plano. Un satelite nuevo se suma a la cola del tren: los que ya estaban no se mueven. Pasados los nitidos, un VELO.
//   - Espejos: ARCOS en una sola orbita alta, uno al lado del otro (una pared de espejos que atrapa el Sol), de a `kArcSize`.
//   - Relays: sin formacion (siguen en `OrbitSlot`): son pocos y el protagonismo es el de sus enlaces.
// `FamilySlot` es la UNICA cuenta: la usan Fleet (los objetos) y WorldFx (los pulsos que salen de un satelite), y tiene que dar lo mismo.
constexpr int kTrainSize = 10;
constexpr float kTrainGap = 0.17f; // radianes entre dos satelites de un tren (~0,36 m del mundo: un poco mas que el ancho de uno)
constexpr int kArcSize = 12;
constexpr float kArcGap = 0.13f;

Slot TrainSlot(const XMVECTOR& center, float base, unsigned seed, int j, float time, float spread) {
    const int train = j / kTrainSize, k = j % kTrainSize;
    Slot s;
    s.orbit = OrbitOf(seed, train, base, spread);
    s.angle = s.orbit.phase + s.orbit.speed * time + static_cast<float>(k) * kTrainGap * (s.orbit.speed < 0.0f ? -1.0f : 1.0f);
    s.pos = OrbitPoint(center, s.orbit, s.angle);
    return s;
}

Slot ArcSlot(const XMVECTOR& center, float base, unsigned seed, int j, float time, float spread) {
    const int arc = j / kArcSize, k = j % kArcSize;
    Slot s;
    s.orbit = OrbitOf(seed, arc, base, spread);
    s.angle = s.orbit.phase + s.orbit.speed * time + static_cast<float>(k) * kArcGap * (s.orbit.speed < 0.0f ? -1.0f : 1.0f);
    s.pos = OrbitPoint(center, s.orbit, s.angle);
    return s;
}

Slot FamilySlot(Art art, const XMVECTOR& center, float base, float spread, unsigned seed, int shown, int j, float time) {
    if (art == Art::Sat) return TrainSlot(center, base, seed, j, time, spread);
    if (art == Art::Mirror) return ArcSlot(center, base, seed, j, time, spread);
    return OrbitSlot(center, base, seed, shown, j, time, spread);
}

// Cuantos satelites se dibujan NITIDOS (los de la escena). Pasado esto el resto son puntos tenues sobre las mismas orbitas: "juntar en
// vez de multiplicar" (docs/plan-visual.md, principio 6).
constexpr int kCrispSats = 40;

} // namespace

// ------------------------------------------------------------------ la red de un cuerpo
// Lo que EarthFx y MoonFx comparten: una red de NODOS repartidos por la esfera (una espiral dorada, para que no se
// amontonen) y las aristas mas cortas entre ellos, ordenadas por largo: las cortas salen primero y las largas, al final.
// Sobre esa red viven la mayoria de las familias de la hoja Families, asi cada cuerpo se lee como UN lugar habitado y no como
// cosas sueltas. Todo es fijo a la superficie (rota con el nodo del cuerpo, que es la matriz de su GameObject): no se anima.
class BodyNet {
protected:
    struct Node {
        float lat, lon;
        XMVECTOR dir;
    };
    struct Edge {
        int a, b;
        float angle; // largo, en radianes
        int samples;
    };

    void BuildNet(int count, unsigned seed, int maxEdges, float maxAngle) {
        for (int i = 0; i < count; ++i) {
            const float lat = std::asin(1.0f - 2.0f * (static_cast<float>(i) + 0.5f) / static_cast<float>(count));
            if (std::fabs(lat) > 1.25f) continue; // sin los polos: ahi no hay nada que conectar
            const float lon = static_cast<float>(i) * 2.39996f + (Hash01(seed, static_cast<unsigned>(i)) - 0.5f) * 0.4f;
            m_nodes.push_back({lat, lon, DirOf(lat, lon)});
        }
        for (std::size_t i = 0; i < m_nodes.size(); ++i)
            for (std::size_t j = i + 1; j < m_nodes.size(); ++j) {
                const float c = std::clamp(XMVectorGetX(XMVector3Dot(m_nodes[i].dir, m_nodes[j].dir)), -1.0f, 1.0f);
                const float ang = std::acos(c);
                if (ang < maxAngle) m_edges.push_back({static_cast<int>(i), static_cast<int>(j), ang, std::max(6, static_cast<int>(ang * 22.0f))});
            }
        std::sort(m_edges.begin(), m_edges.end(), [](const Edge& x, const Edge& y) { return x.angle < y.angle; });
        if (static_cast<int>(m_edges.size()) > maxEdges) m_edges.resize(static_cast<std::size_t>(maxEdges));
    }

    static XMVECTOR DirOf(float lat, float lon) { return XMVectorSet(std::cos(lat) * std::cos(lon), std::sin(lat), std::cos(lat) * std::sin(lon), 0.0f); }

    // Lo que cambia cada frame: donde esta la camara, el cuerpo y el Sol.
    void Begin(UiDrawList& ui, const Cam& cam, const Body& body, const GameState& game, const GameObject& bodyObj, const XMVECTOR& sunCenter, float time) {
        m_ui = &ui;
        m_cam = &cam;
        m_body = &body;
        m_tree = &game.tree();
        m_game = &game;
        m_time = time;
        m_m = ComputeWorldMatrix(bodyObj);
        m_vc = XMVector3TransformCoord(body.center, cam.view);
        m_toSun = XMVector3Normalize(XMVectorSubtract(sunCenter, body.center));
    }

    bool Has(const char* key) const { return m_tree->IsResearched(key); }
    XMVECTOR World(const XMVECTOR& dir, float h) const { return XMVector3TransformCoord(XMVectorScale(dir, 1.0f + h), m_m); }
    // Un punto de la SUPERFICIE esta de este lado del cuerpo si la camara lo ve de frente.
    bool NearSide(const XMVECTOR& world) const {
        const XMVECTOR pv = XMVector3TransformCoord(world, m_cam->view);
        return XMVectorGetX(XMVector3Dot(XMVectorSubtract(pv, m_vc), XMVectorNegate(pv))) > 0.0f;
    }
    // 1 = medianoche, 0 = mediodia.
    float Night(const XMVECTOR& world) const {
        const XMVECTOR nw = XMVector3Normalize(XMVectorSubtract(world, m_body->center));
        return std::clamp(0.5f - XMVectorGetX(XMVector3Dot(nw, m_toSun)) * 0.5f, 0.0f, 1.0f);
    }
    bool Proj(const XMVECTOR& world, float& x, float& y) const {
        float z;
        return m_cam->Project(world, x, y, z);
    }
    // El punto de un nodo en pantalla, solo si esta de este lado.
    bool NodeAt(int i, float& x, float& y, float& night, float h = 0.0f) const {
        if (i < 0 || i >= static_cast<int>(m_nodes.size())) return false;
        const XMVECTOR dir = m_nodes[static_cast<std::size_t>(i)].dir;
        if (!NearSide(World(dir, 0.0f))) return false;
        const XMVECTOR w = World(dir, h);
        if (!Proj(w, x, y)) return false;
        night = Night(w);
        return true;
    }
    // El punto `h` radios por encima de un nodo, en pantalla: hacia donde "sube" lo que sale del suelo.
    bool UpFrom(int i, float h, float& x, float& y) const {
        if (i < 0 || i >= static_cast<int>(m_nodes.size())) return false;
        return Proj(World(m_nodes[static_cast<std::size_t>(i)].dir, h), x, y);
    }
    XMVECTOR EdgeDir(const Edge& e, float t) const {
        return XMVector3Normalize(XMVectorLerp(m_nodes[static_cast<std::size_t>(e.a)].dir, m_nodes[static_cast<std::size_t>(e.b)].dir, t));
    }
    // Un punto en una arista, `t` de 0 a 1, visible solo si esta de frente. Devuelve la noche de ese punto.
    bool EdgePoint(const Edge& e, float t, float& x, float& y, float& night) const {
        const XMVECTOR dir = EdgeDir(e, t);
        if (!NearSide(World(dir, 0.0f))) return false;
        const XMVECTOR w = World(dir, 0.004f);
        night = Night(w);
        return Proj(w, x, y);
    }
    int NodeCount() const { return static_cast<int>(m_nodes.size()); }

    UiDrawList* m_ui = nullptr;
    const Cam* m_cam = nullptr;
    const Body* m_body = nullptr;
    const TechTree* m_tree = nullptr;
    const GameState* m_game = nullptr;
    float m_time = 0.0f;
    XMMATRIX m_m = XMMatrixIdentity();
    XMVECTOR m_vc = XMVectorZero();
    XMVECTOR m_toSun = XMVectorSet(0, 0, 1, 0);
    std::vector<Node> m_nodes;
    std::vector<Edge> m_edges;
};
