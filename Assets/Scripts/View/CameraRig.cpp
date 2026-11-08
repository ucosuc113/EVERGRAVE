// CameraRig: la camara del juego. Gira alrededor del cuerpo que se mira, o se aleja al MAPA
// del sistema, y de paso decide que fue un clic y que un arrastre.
//
// DOS MODOS (Ui().view / Ui().mapView):
//  - Cuerpo (Tierra, Luna, Sol): la camara orbita ese cuerpo. Se arrastra con el boton
//    izquierdo para girar, la rueda acerca y aleja, y si nadie la toca un rato gira despacio
//    sola. El angulo se mide respecto a la direccion al Sol (0 = del lado del dia), asi
//    "la cara iluminada" no depende de en que punto de su orbita este el planeta.
//  - Mapa: desde arriba casi del todo (75 grados, no 90: se sigue viendo que es un espacio con
//    volumen) y lo bastante lejos para que quepa el sistema entero. Todo se ve chico pero se
//    puede tocar: un clic cerca de un cuerpo lo elige, y la camara vuela hasta el.
//
// Al EMPEZAR la persona esta en la Tierra y no puede salir (el mapa esta cerrado): lo abre
// una tecnologia (`UnlockBody` "map"), y cada cuerpo, otra.
//
// EL CLIC Y EL ARRASTRE COMPARTEN EL BOTON. Antes el clic recolectaba al APRETAR; ahora, para
// poder arrastrar sin recolectar, se decide al SOLTAR: si el puntero casi no se movio fue un
// clic (`Ui().worldClick`, que lee SpaceWorld), y si se movio fue un arrastre. Un arrastre que
// empieza sobre un panel no cuenta: eso es de la interfaz.
//
// Corre justo despues de GameRoot, antes del HUD y los paneles: lo que dibuja aqui (las etiquetas
// del mapa) queda DEBAJO de la interfaz. Tambien mueve el cielo (`Space/Stars`) con la camara:
// las estrellas estan a 80 unidades del origen y la camara ya no se queda cerca de el.

#include "../Services/Cine.h"
#include "../Services/Controls.h"
#include "../Services/Session.h"
#include "UiKit.h"

#include "Engine/Rendering/Camera.h"
#include "Engine/Rendering/Material.h"
#include "Engine/Rendering/MaterialCache.h"
#include "Engine/Scene/GameObject.h"
#include "Engine/Scene/SceneQuery.h"
#include "Engine/Scene/SceneService.h"
#include "Engine/Scene/SceneTransform.h"
#include "Engine/Scripting/IScript.h"
#include "Engine/Scripting/ScriptRegistry.h"
#include "Engine/UI/UiService.h"

#include <DirectXMath.h>

#include <algorithm>
#include <cmath>
#include <string>

using namespace strace;
using namespace evergrave;
using namespace DirectX;

namespace {

constexpr const char* kCameraPath = "Space/OrbitPivot/Camara";
constexpr const char* kStarsPath = "Space/Stars";
constexpr const char* kEarthPath = "Space/OrbitPivot/EarthAnchor/Tilt/Earth";
constexpr const char* kMoonPath = "Space/OrbitPivot/EarthAnchor/Moon";
constexpr const char* kSunPath = "Space/OrbitPivot/Sun";

constexpr float kRad = 3.14159265f / 180.0f;
constexpr float kDragSlop = 4.0f;
constexpr float kIdleBeforeSpin = 2.5f;   // segundos
// LA CAMARA QUIETA SE BALANCEA, NO DA LA VUELTA. Antes giraba sola 1,4 grados por segundo: en cuatro minutos pasaba por TODOS los
// angulos, y la toma compuesta (el Sol detras, la media luna) duraba un instante. Ahora oscila `kSwayDeg` grados a cada lado del
// angulo que dejo la persona, con un periodo de `kSwayPeriod` segundos: la imagen respira sin perder la luz. Arrastrar vuelve a
// fijar el angulo base.
constexpr float kSwayDeg = 9.0f;
constexpr float kSwayPeriod = 70.0f;
constexpr float kPitchLimit = 89.0f;      // grados: casi el polo, arriba y abajo (ver HandleMouse)
constexpr float kMapDist = 96.0f, kMapPitch = 75.0f;
// La distancia de la toma de la Tierra: el planeta ocupa ~49% del alto (antes, a 16, un 32%; a 9 pasaba del 57% y la flota se salia del cuadro): un planeta dominante, no un adorno.
constexpr float kEarthDist = 10.5f;
// La Luna mide 0,5 de radio: a 5,5 ocupaba un cuarto del alto; a 3,6 un 40%, y es lo que la deja ser el protagonista de su vista.
constexpr float kMoonDist = 3.6f;
// EL ENCUADRE DEL MENU: la Tierra sube desde el borde de abajo y solo se ve su mitad de arriba. La camara mira a un
// punto ENCIMA del centro de la Tierra (`kMenuLift`), asi el planeta queda debajo del cuadro; casi de lado
// (`kMenuPitch`) para que se vea el limbo y no el polo; y del lado del dia, con el terminador a la derecha (el Sol entra por la izquierda).
constexpr float kMenuDist = 5.3f, kMenuPitch = 9.0f, kMenuYaw = -38.0f, kMenuLift = 2.1f;
constexpr float kShopEdge = 178.0f;      // hasta donde llega la tienda desde el borde izquierdo (ShopPanel: 6 de margen + kShopW 172)
constexpr float kPickRadius = 18.0f;      // pixeles de juego: el blanco de un cuerpo en el mapa

// La diferencia mas corta entre dos angulos, en grados.
float AngleDelta(float from, float to) {
    float d = std::fmod(to - from, 360.0f);
    if (d > 180.0f) d -= 360.0f;
    if (d < -180.0f) d += 360.0f;
    return d;
}

struct Pose {
    XMFLOAT3 center{0, 0, 0};
    float dist = 13.0f;
    float yaw = 0.0f;   // grados
    float pitch = 35.0f;
};

} // namespace

class CameraRig : public IScript {
public:
    void OnStart() override {
        m_cam = FindByPath(SceneService::MutableRoots(), kCameraPath);
        m_stars = FindByPath(SceneService::MutableRoots(), kStarsPath);
        m_earth = FindByPath(SceneService::MutableRoots(), kEarthPath);
        m_moon = FindByPath(SceneService::MutableRoots(), kMoonPath);
        m_sun = FindByPath(SceneService::MutableRoots(), kSunPath);
        m_snap = true;
    }

    void OnUpdate(float dt) override {
        if (!m_cam || !m_earth || !m_moon || !m_sun) return;
        UiState& state = Ui();
        GameState& game = Session();
        InputActions& in = InputService::Actions();

        // FILMANDO: la camara la lleva la cinematica (Services/Cine.h), sin suavizado ni teclado.
        if (Cine().active) {
            DriveCine(state);
            return;
        }

        // --- Teclado: cambiar de vista ---
        if (in.Pressed("ev_view_earth")) GoToBody(BodyId::Earth);
        if (in.Pressed("ev_view_moon") && !GoToBody(BodyId::Moon)) Toast("The Moon is not open yet", 1.6f);
        if (in.Pressed("ev_view_sun") && !GoToBody(BodyId::Sun)) Toast("The Sun is not open yet", 1.6f);
        if (in.Pressed("ev_map")) {
            if (state.mapView) GoToBody(state.view);
            else if (!GoToMap()) Toast("The system map is not open yet", 1.6f);
        }

        if (!state.menu) HandleMouse(state, in, dt);
        // El balanceo: sube a su amplitud entera cuando la camara lleva quieta `kIdleBeforeSpin` segundos y baja a cero al tocarla.
        m_sway += ((state.camIdle > kIdleBeforeSpin && !state.camDragging ? 1.0f : 0.0f) - m_sway) * (1.0f - std::exp(-1.2f * dt));
        m_swayT += dt * 2.0f * 3.14159265f / kSwayPeriod;

        // --- A donde mira la camara ---
        // El Sol "de verdad" esta siempre en el centro del sistema; el que se ve se aleja (SpaceWorld) cuando se mira la
        // Tierra o la Luna, asi que para apuntar la camara y marcar el mapa se usa el punto fijo.
        const XMFLOAT3 earth = ComputeWorldPosition(*m_earth), moon = ComputeWorldPosition(*m_moon), sun(0, 0, 0);
        Pose target;
        if (state.menu) {
            target.center = XMFLOAT3(earth.x, earth.y + kMenuLift, earth.z);
            target.dist = kMenuDist;
            target.pitch = kMenuPitch;
            target.yaw = std::atan2(-earth.x, -earth.z) / kRad + kMenuYaw;
        } else if (state.mapView) {
            target.center = XMFLOAT3(0, 0, 0);
            target.dist = kMapDist;
            target.pitch = kMapPitch;
            target.yaw = 0.0f;
        } else {
            const XMFLOAT3& c = state.view == BodyId::Moon ? moon : state.view == BodyId::Sun ? sun : earth;
            target.center = c;
            target.dist = (state.view == BodyId::Moon ? kMoonDist : state.view == BodyId::Sun ? 36.0f : kEarthDist) * state.camZoom;
            target.pitch = state.camPitch;
            // El angulo se mide respecto a la direccion al Sol (para el Sol mismo, desde +z): la Tierra y la LUNA tienen la misma toma. La Luna
            // fue antes la excepcion (su angulo era FIJO EN EL MUNDO, porque medirlo respecto a la Tierra giraba con el mes de 45 s y la
            // camara no la orbitaba, la arrastraba); pero respecto al SOL la direccion cambia unos grados en todo el mes, asi que no gira, y
            // a cambio la fase que se ve es la misma cada vez que se entra, en vez de depender del momento del mes.
            float ref = 0.0f;
            if (state.view != BodyId::Sun) ref = std::atan2(-c.x, -c.z) / kRad;
            target.yaw = ref + state.camYaw + kSwayDeg * m_sway * std::sin(m_swayT);
            // EL ENCUADRE: el planeta no va en el centro de la pantalla sino en el centro del AREA LIBRE (a la derecha de la
            // tienda). Se logra apuntando la camara a un punto del lado contrario, a lo ancho de la imagen: `pan` es ese
            // corrimiento en metros del mundo, calculado para el ancho de la pantalla a esta distancia. Sale del vector derecho de la
            // camara, asi que sigue al giro y el planeta queda quieto en pantalla mientras la camara lo orbita.
            if (!state.mapView) {
                const float pan = FramingPan(state, target.dist);
                const float a = target.yaw * kRad;
                target.center.x += std::cos(a) * pan;
                target.center.z -= std::sin(a) * pan;
            }
            m_bodyCenter = c;
        }

        if (m_snap) {
            m_pose = target;
            m_snap = false;
        } else {
            // Suavizado exponencial: la camara llega en un segundo o dos sin arrancar ni frenar de golpe.
            const float k = 1.0f - std::exp(-3.2f * dt);
            m_pose.center.x += (target.center.x - m_pose.center.x) * k;
            m_pose.center.y += (target.center.y - m_pose.center.y) * k;
            m_pose.center.z += (target.center.z - m_pose.center.z) * k;
            m_pose.dist += (target.dist - m_pose.dist) * k;
            m_pose.pitch += (target.pitch - m_pose.pitch) * k;
            m_pose.yaw += AngleDelta(m_pose.yaw, target.yaw) * k;
        }

        // --- Ponerla ---
        const float p = m_pose.pitch * kRad, a = m_pose.yaw * kRad;
        const XMFLOAT3 pos(m_pose.center.x + m_pose.dist * std::cos(p) * std::sin(a), m_pose.center.y + m_pose.dist * std::sin(p),
                           m_pose.center.z + m_pose.dist * std::cos(p) * std::cos(a));
        m_cam->transform.position[0] = pos.x;
        m_cam->transform.position[1] = pos.y;
        m_cam->transform.position[2] = pos.z;
        // Mira al centro: el angulo en Y hacia el, y el de arriba/abajo por la altura.
        m_cam->transform.rotation[0] = m_pose.pitch;
        m_cam->transform.rotation[1] = std::atan2(m_pose.center.x - pos.x, m_pose.center.z - pos.z) / kRad;
        m_cam->transform.rotation[2] = 0.0f;
        if (m_stars) {
            m_stars->transform.position[0] = pos.x;
            m_stars->transform.position[1] = pos.y;
            m_stars->transform.position[2] = pos.z;
        }
        UpdateFocus(state, dt);
        UpdateLook(state, dt);

        if (state.mapView) DrawMap(game, state, in, earth, moon, sun);
    }

private:
    // LA CAMARA DE LA CINEMATICA. La pose sale entera de la toma (`CinePoseOf`), con los cuerpos calculados por la misma funcion del
    // tiempo (`CineWorldOf`): sin suavizado - un corte entre dos tomas tiene que ser un corte, y el ajuste de color y el enfoque tambien
    // saltan con el, no se arrastran de la toma anterior.
    void DriveCine(const UiState& state) {
        const CineFrame& f = Cine().f;
        // Las rocas que tapan los cortes del cinturon: la escena sabe donde estan (lo del frame anterior, a 60 fps no se nota).
        if (!m_rock0) m_rock0 = FindByPath(SceneService::MutableRoots(), "Space/OrbitPivot/Belt/Asteroid000");
        if (!m_rock1) m_rock1 = FindByPath(SceneService::MutableRoots(), "Space/OrbitPivot/Belt/Asteroid001");
        CineWorld& world = Cine().world;
        if (m_rock0) { const XMFLOAT3 p = ComputeWorldPosition(*m_rock0); world.rock[0] = {p.x, p.y, p.z}; }
        if (m_rock1) { const XMFLOAT3 p = ComputeWorldPosition(*m_rock1); world.rock[1] = {p.x, p.y, p.z}; }
        const CinePose pose = CinePoseOf(f, world);
        m_cam->transform.position[0] = pose.eye.x;
        m_cam->transform.position[1] = pose.eye.y;
        m_cam->transform.position[2] = pose.eye.z;
        const float dx = pose.target.x - pose.eye.x, dy = pose.target.y - pose.eye.y, dz = pose.target.z - pose.eye.z;
        m_cam->transform.rotation[0] = std::atan2(-dy, std::sqrt(dx * dx + dz * dz)) / kRad; // positivo = mira hacia abajo
        m_cam->transform.rotation[1] = std::atan2(dx, dz) / kRad;
        m_cam->transform.rotation[2] = pose.roll; // el balanceo: una camara de verdad no va siempre a nivel
        if (m_stars) {
            m_stars->transform.position[0] = pose.eye.x;
            m_stars->transform.position[1] = pose.eye.y;
            m_stars->transform.position[2] = pose.eye.z;
        }
        if (m_cam->camera) {
            CameraComponent& cc = *m_cam->camera;
            cc.fovYDegrees = f.k.fov;
            // EL ENFOQUE VA A LO QUE SE VE EN EL CENTRO DEL CUADRO, no a un punto fijo de la toma: se busca el cuerpo que el rayo central de
            // la camara atraviesa (o, si ninguno, el que pasa mas cerca de ese rayo) y se enfoca la superficie que se ve de el. Con el punto
            // de mira como unica referencia el foco caia a veces delante o detras de lo que el cuadro mostraba, y lo nitido salia borroso.
            const float vl = std::max(1e-4f, std::sqrt(dx * dx + dy * dy + dz * dz));
            const float vx = dx / vl, vy = dy / vl, vz = dz / vl;
            struct Hit { const CineV3* c; float radius; };
            const Hit bodies[3] = {{&world.earth, 1.8f}, {&world.moon, 0.5f}, {&world.sun, 5.0f}};
            float focus = vl, best = 1e9f;
            for (const Hit& b : bodies) {
                if (b.c == &world.sun && f.k.sunDist <= 0.0f) continue;
                const float bx = b.c->x - pose.eye.x, by = b.c->y - pose.eye.y, bz = b.c->z - pose.eye.z;
                const float along = bx * vx + by * vy + bz * vz;
                if (along <= 0.05f) continue;
                const float perp = std::sqrt(std::max(0.0f, bx * bx + by * by + bz * bz - along * along));
                const float miss = perp - b.radius; // negativo = el rayo central da en el cuerpo
                if (miss >= best) continue;
                best = miss;
                focus = miss < 0.0f ? along - std::sqrt(std::max(0.0f, b.radius * b.radius - perp * perp)) : along;
            }
            cc.focusDistance = std::max(0.3f, focus);
            // La apertura de cada clave es un maximo de "cuanto se desenfoca el resto": a escala completa el desenfoque se comia lo que se
            // miraba. Una tercera parte deja el fondo y lo que pasa MUY cerca de la camara suaves, y lo enfocado nitido.
            cc.aperture = f.k.aperture * 0.35f;
        }
        m_bodyCenter = XMFLOAT3(pose.target.x, pose.target.y, pose.target.z);
        m_snap = true; // el ajuste de color salta, no se desliza
        UpdateLook(state, 1.0f / 60.0f);
        m_snap = false;
        // EL ECLIPSE ESPECIAL (precious moon, precious sun): en el golpe la corona del Sol se abre, se ensancha y brilla, y el resplandor del
        // post sube con ella; despues vuelve despacio. Se escribe CADA frame (con brillo 0 queda en los valores del .strmat) para que nada se
        // quede a medias si se salta a otro segundo.
        if (MaterialCache* cache = SceneService::Materials()) {
            if (Material* corona = cache->GetMutable(L"Assets/Materials/SunCorona.strmat")) {
                corona->userParams[0].x = 1.6f + 0.9f * f.glow;  // el halo llega mas lejos
                corona->userParams[0].y = 1.0f + 2.4f * f.glow;  // y brilla mas
                corona->userParams[0].w = 3.0f - 1.4f * f.glow;  // y cae mas despacio
            }
            if (f.glow > 0.0f)
                if (Material* post = cache->GetMutable(L"Assets/Materials/PostProcess.strmat")) post->userParams[2].z += 1.1f * f.glow;
        }
    }

    // EL ENFOQUE SIGUE AL CUERPO QUE SE MIRA. La camara del juego tiene apertura (profundidad de campo) y el post
    // (PostProcess.hlsl) desenfoca lo que esta lejos del foco; sin esto el foco quedaria fijo en la distancia de la
    // Tierra y mirando la Luna, o el Sol, saldria todo borroso. El foco va a la SUPERFICIE cercana del cuerpo
    // (`dist - radio * 0.55`) y no a su centro: el lado que se ve de una esfera esta mas cerca que su centro, y la
    // cara iluminada es lo que tiene que quedar nitido. En el mapa todo es chico y esta lejos: sin apertura (nada
    // desenfocado, y se puede tocar lo que se ve nitido).
    //
    // La apertura SE SUAVIZA con la misma constante que la pose: un salto de 0 a 0,6 al abrir el mapa seria un parpadeo.
    void UpdateFocus(const UiState& state, float dt) {
        if (!m_cam || !m_cam->camera) return;
        CameraComponent& cc = *m_cam->camera;
        const float radius = state.menu || state.mapView || state.view == BodyId::Earth ? 1.8f : state.view == BodyId::Moon ? 0.5f : 5.0f;
        // La distancia al CUERPO (no al punto al que apunta la camara, que con el encuadre esta corrido hacia un lado).
        const XMFLOAT3 camPos(m_cam->transform.position[0], m_cam->transform.position[1], m_cam->transform.position[2]);
        const float dx = camPos.x - m_bodyCenter.x, dy = camPos.y - m_bodyCenter.y, dz = camPos.z - m_bodyCenter.z;
        const float toBody = state.menu || state.mapView ? m_pose.dist : std::sqrt(dx * dx + dy * dy + dz * dz);
        cc.focusDistance = std::max(0.5f, toBody - radius * 0.55f);
        // El menu lleva la apertura mas abierta: la Tierra sube desde el borde y el cielo vacio de arriba es lo que
        // mas se beneficia de un fondo suave. El Sol llena la pantalla, asi que casi todo lo que se ve esta cerca del foco.
        const float wanted = state.mapView ? 0.0f : state.menu ? 0.7f : state.view == BodyId::Sun ? 0.35f : 0.6f;
        if (m_snap) cc.aperture = wanted;
        else cc.aperture += (wanted - cc.aperture) * (1.0f - std::exp(-3.2f * dt));
        if (cc.aperture < 0.01f) cc.aperture = 0.0f; // sin esto la apertura nunca llega a 0 y el post sigue pidiendo profundidad
    }

    // EL COLOR DE CADA TOMA. La camara lleva un ajuste de color (`grade`) y el post tiene vineta y brillo; cada vista los quiere
    // distintos: la Tierra es fria con luces calidas (sombras verde azuladas), la Luna es plata casi sin color, el Sol es calido y
    // comprimido (sus luces ya queman: se bajan), el menu es mas contrastado. Es lo que hace que cambiar de cuerpo se SIENTA como
    // cambiar de plano y no solo de objeto. Todo se suaviza con la constante de la pose: un salto de color al cambiar de vista parpadearia.
    struct Look {
        float liftX, liftY, liftM;      // sombras
        float gainX, gainY;             // luces
        float saturation, contrast, highlights, vibrance;
        float vignette, glow;           // del post: fuerza de la vineta, intensidad del brillo
    };
    static Look LookOf(const UiState& s) {
        if (s.menu) return {-0.20f, -0.14f, -0.03f, 0.13f, 0.10f, 0.92f, 1.10f, -0.12f, 0.10f, 0.50f, 0.65f};
        if (s.mapView) return {-0.10f, -0.08f, 0.0f, 0.06f, 0.05f, 0.95f, 1.04f, -0.05f, 0.05f, 0.30f, 0.50f};
        if (s.view == BodyId::Moon) return {-0.07f, -0.07f, -0.01f, 0.05f, 0.04f, 0.78f, 1.08f, -0.08f, 0.0f, 0.45f, 0.55f};
        if (s.view == BodyId::Sun) return {-0.10f, -0.04f, 0.0f, 0.16f, 0.08f, 1.0f, 1.04f, -0.22f, 0.0f, 0.30f, 0.80f};
        return {-0.18f, -0.13f, -0.02f, 0.12f, 0.10f, 0.92f, 1.06f, -0.12f, 0.10f, 0.40f, 0.55f}; // la Tierra
    }
    void UpdateLook(const UiState& state, float dt) {
        if (!m_cam || !m_cam->camera) return;
        const Look t = LookOf(state);
        const float k = m_snap ? 1.0f : 1.0f - std::exp(-3.2f * dt);
        const auto to = [&](float& v, float target) { v += (target - v) * k; };
        ColorGrading& g = m_cam->camera->grade;
        to(g.lift.x, t.liftX);
        to(g.lift.y, t.liftY);
        to(g.lift.master, t.liftM);
        to(g.gain.x, t.gainX);
        to(g.gain.y, t.gainY);
        to(g.saturation, t.saturation);
        to(g.contrast, t.contrast);
        to(g.highlights, t.highlights);
        to(g.vibrance, t.vibrance);
        // El post: la vineta (UserParams[1].w) y el brillo (UserParams[2].z) del material de PostProcess. Es por RUTA y compartido,
        // como lo que SpaceWorld le escribe a la Tierra; sin la ruta (otro proyecto, otro material) no hace nada.
        if (MaterialCache* cache = SceneService::Materials())
            if (Material* m = cache->GetMutable(L"Assets/Materials/PostProcess.strmat")) {
                to(m->userParams[1].w, t.vignette);
                to(m->userParams[2].z, t.glow);
            }
    }

    // Cuanto correr el punto de mira, en metros del mundo, para que el cuerpo caiga en el centro del area libre (la pantalla menos la
    // tienda). Positivo = el planeta se ve MAS A LA DERECHA. Con la tienda cerrada o el arbol abierto no hay corrimiento.
    float FramingPan(const UiState& state, float dist) const {
        if (!state.shopOpen || state.treeOpen) return 0.0f;
        const float w = UiService::Width(), h = UiService::Height();
        if (w <= 0.0f || h <= 0.0f || !m_cam || !m_cam->camera) return 0.0f;
        const UiRect vis = UiService::VisibleRect();
        const float freeLeft = vis.x + kShopEdge, freeRight = vis.Right();
        const float shiftPx = (freeLeft + freeRight) * 0.5f - w * 0.5f; // cuanto se aleja el centro del area libre del de la pantalla
        const float halfWidth = dist * std::tan(m_cam->camera->fovYDegrees * 0.5f * kRad) * (w / h); // mitad del ancho visible a esa distancia
        return shiftPx / (w * 0.5f) * halfWidth;
    }

    // Clic, arrastre y rueda sobre el mundo.
    void HandleMouse(UiState& state, InputActions& in, float dt) {
        const bool freeOfUi = !state.pointerOverUiLast && in.CursorInside();
        // Con el dedo (Services/Touch.h): dos dedos pellizcan - el zoom, que en el PC es la rueda - y mientras haya dos
        // no se orbita ni se cuenta un clic, porque el cursor salta de un dedo al otro y eso no es un arrastre de nadie.
        if (TouchMode()) {
            TouchUpdate();
            const TouchGesture& g = Touch();
            if (g.cancelled) {
                state.camDragging = false;
                if (g.fingers >= 2 && freeOfUi && !state.mapView && g.pinch != 1.0f) {
                    state.camZoom = std::clamp(state.camZoom / g.pinch, 0.55f, 1.9f); // dedos que se separan = acercar
                    state.camIdle = 0.0f;
                }
                state.camIdle += dt;
                return;
            }
        }
        if (freeOfUi && in.Pressed("ev_click")) {
            state.camDragging = true;
            state.camDragDist = 0.0f;
        }
        if (state.camDragging) {
            if (in.Held("ev_click")) {
                const float dx = in.CursorDeltaX(), dy = in.CursorDeltaY();
                state.camDragDist += std::fabs(dx) + std::fabs(dy);
                // En el mapa no se gira (esta fijo a proposito: si se mueve, tocar un cuerpo chico se vuelve adivinar).
                // Los signos los eligio la persona probando, eje por eje (yaw += dx, pitch += dy): las otras tres
                // combinaciones se probaron y se sentian al reves. No "corregirlos" por simetria. El giro vertical es libre: se puede pasar por
                // debajo del plano del sistema y mirar desde abajo. El unico tope es el polo, a `kPitchLimit` de cada
                // lado: en 90 la camara quedaria mirando recto y al pasarlo se daria vuelta (el mundo cabeza abajo y el
                // arrastre horizontal al reves).
                if (state.camDragDist >= kDragSlop && !state.mapView) {
                    state.camYaw += dx * 0.4f;
                    state.camPitch = std::clamp(state.camPitch + dy * 0.3f, -kPitchLimit, kPitchLimit);
                    state.camIdle = 0.0f;
                }
            }
            if (in.Released("ev_click")) {
                state.camDragging = false;
                if (state.camDragDist < kDragSlop) state.worldClick = true;
            }
        }
        if (freeOfUi && !state.mapView && in.WheelDelta() != 0.0f) {
            state.camZoom = std::clamp(state.camZoom * (1.0f - in.WheelDelta() * 0.1f), 0.55f, 1.9f);
            state.camIdle = 0.0f;
        }
        // Quieta un rato: gira sola, despacio.
        state.camIdle += dt;
    }

    // El mapa: las orbitas, un nombre junto a cada cuerpo, y el clic para elegir.
    void DrawMap(GameState& game, UiState& state, InputActions& in, const XMFLOAT3& earth, const XMFLOAT3& moon, const XMFLOAT3& sun) {
        const GameObject* camObj = FindActiveCamera(SceneService::Roots());
        if (!camObj || !camObj->camera) return;
        const float w = UiService::Width(), h = UiService::Height();
        if (w <= 0.0f || h <= 0.0f) return;
        const XMMATRIX view = ComputeGameCameraView(*camObj);
        const XMMATRIX proj = ComputeGameCameraProjection(*camObj->camera, SceneService::Type(), w / h);
        const auto project = [&](const XMFLOAT3& wp, float& sx, float& sy) {
            const XMVECTOR v = XMVector3TransformCoord(XMLoadFloat3(&wp), view);
            if (XMVectorGetZ(v) <= 0.05f) return false;
            const XMVECTOR c = XMVector3TransformCoord(v, proj);
            sx = (XMVectorGetX(c) * 0.5f + 0.5f) * w;
            sy = (0.5f - XMVectorGetY(c) * 0.5f) * h;
            return true;
        };

        UiDrawList& ui = UiService::Frame();
        // Las orbitas: la de la Tierra alrededor del Sol y la de la Luna alrededor de la Tierra.
        const auto ring = [&](const XMFLOAT3& center, float radius, int steps, const UiColor& c) {
            float px = 0, py = 0;
            bool have = false;
            for (int i = 0; i <= steps; ++i) {
                const float a = 2.0f * 3.14159265f * static_cast<float>(i) / static_cast<float>(steps);
                float sx, sy;
                const bool ok = project(XMFLOAT3(center.x + radius * std::cos(a), center.y, center.z + radius * std::sin(a)), sx, sy);
                if (ok && have) ui.AddLine(px, py, sx, sy, c, 1.0f);
                have = ok;
                px = sx;
                py = sy;
            }
        };
        ring(XMFLOAT3(0, 0, 0), std::sqrt(earth.x * earth.x + earth.z * earth.z), 96, UiColor::Bytes(120, 140, 190, 70));
        ring(earth, 8.0f, 40, UiColor::Bytes(170, 170, 190, 80));

        struct Marker { BodyId body; XMFLOAT3 pos; float sx, sy; bool ok; };
        Marker m[3] = {{BodyId::Sun, sun, 0, 0, false}, {BodyId::Earth, earth, 0, 0, false}, {BodyId::Moon, moon, 0, 0, false}};
        for (Marker& k : m) k.ok = project(k.pos, k.sx, k.sy);

        // El mas cercano al puntero, si esta dentro del blanco.
        int hover = -1;
        float best = kPickRadius * kPickRadius;
        if (in.CursorInside() && !state.pointerOverUiLast) {
            for (int i = 0; i < 3; ++i) {
                if (!m[i].ok) continue;
                const float dx = in.CursorX() - m[i].sx, dy = in.CursorY() - m[i].sy;
                if (dx * dx + dy * dy < best) {
                    best = dx * dx + dy * dy;
                    hover = i;
                }
            }
        }
        if (state.worldClick && hover >= 0) {
            const BodyId b = m[hover].body;
            if (!GoToBody(b)) Toast(std::string(InfoOf(b).name) + " is not open yet", 1.6f);
            state.worldClick = false;   // el clic fue de la camara, no del mundo
        }

        for (int i = 0; i < 3; ++i) {
            if (!m[i].ok) continue;
            const bool open = game.IsBodyUnlocked(m[i].body);
            const UiColor base = open ? kit::Text() : kit::Faint();
            std::string label = InfoOf(m[i].body).name;
            if (!open) label += " (LOCKED)";
            else if (const int n = game.OwnedOn(m[i].body); n > 0) label += "  x" + std::to_string(n);
            // El nombre junto al cuerpo, cada uno por su lado para que no se pisen: el del Sol arriba de su disco,
            // el de la Tierra a su izquierda y el de la Luna a su derecha.
            const float lw = kit::TextW(label);
            float lx = m[i].sx - lw * 0.5f, ly = m[i].sy - 18.0f;
            if (m[i].body == BodyId::Sun) ly = m[i].sy - 42.0f;
            if (m[i].body == BodyId::Earth) { lx = m[i].sx - lw - 12.0f; ly = m[i].sy - 4.0f; }
            if (m[i].body == BodyId::Moon) { lx = m[i].sx + 14.0f; ly = m[i].sy - 4.0f; }
            ui.AddText(std::floor(lx), std::floor(ly), label, i == hover ? kit::Text() : base.WithAlpha(0.85f), 1);
            if (i == hover) {
                const float r = 11.0f;
                ui.AddRectOutline(kit::Rect(std::floor(m[i].sx - r), std::floor(m[i].sy - r), r * 2, r * 2), open ? kit::Red() : kit::Dim(), 1.0f);
            }
        }
        ui.AddText(std::floor(UiService::VisibleRect().x + UiService::VisibleRect().width * 0.5f - kit::TextW("CLICK A BODY TO GO THERE") * 0.5f),
                   std::floor(UiService::VisibleRect().Bottom() - 50.0f), "CLICK A BODY TO GO THERE", kit::Dim(), 1);
    }

    GameObject* m_cam = nullptr;
    GameObject* m_stars = nullptr;
    GameObject* m_earth = nullptr;
    GameObject* m_moon = nullptr;
    GameObject* m_sun = nullptr;
    GameObject* m_rock0 = nullptr; // la cinematica: dos de las rocas grandes del cinturon (Services/Cine.h)
    GameObject* m_rock1 = nullptr;
    Pose m_pose;
    bool m_snap = true;
    XMFLOAT3 m_bodyCenter{0, 0, 0};
    float m_sway = 0.0f;  // 0..1: cuanto del balanceo esta puesto
    float m_swayT = 0.0f; // la fase del balanceo, en radianes
};

STRACE_REGISTER_SCRIPT(CameraRig)
