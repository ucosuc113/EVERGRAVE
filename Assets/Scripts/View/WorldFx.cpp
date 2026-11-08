// OJO (7/11/2026): LO MAS IMPORTANTE YA NO SE DIBUJA AQUI. Los satelites, espejos, relays, laseres, el anillo, la estacion y el ascensor de la Tierra son
// OBJETOS DE LA ESCENA (View/Fleet.cpp) y el anillo y los colectores del Sol tambien (View/SunWorks.cpp): reciben la luz, el brillo, el desenfoque y el
// grade de verdad. Lo que sigue en UI es lo fijo a la superficie (luces, plantas, ciudades), el polvo, los pulsos entre satelites y lo que no tiene
// objeto. Con `FxBus().fleetInScene` / `sunWorksInScene` apagados (una escena sin esos nodos) todo se dibuja como UI, como antes. Ver docs/plan-visual.md, seccion 9.
//
// WorldFx: lo que se COMPRO, a la vista alrededor del planeta. Satelites que giran,
// luces sobre la Tierra, anillos alrededor del Sol, puntos que se alejan.
//
// Es presentacion pura: lee cuantas hay de cada instalacion y las dibuja, y nada de
// aqui cambia la partida. Que cada tipo se vea como un tipo de cosa (Orbit, Surface,
// Sun, Far) lo dice la hoja (columna Look); aqui solo se sabe dibujar esas cuatro.
//
// COMO SE DIBUJA. No son objetos de la escena: son puntos de la UI puestos donde la
// camara del juego veria un punto del mundo (Tierra y Sol vienen de la escena, el resto
// se calcula). Una escena con cientos de instalaciones seria cientos de objetos que
// animar; asi son un bucle de rectangulos, y la cuenta de cuantas se dibujan la acota la
// hoja (`Visible cap`).
//   - Orbit: cada punto va en una orbita inclinada alrededor de la Tierra. Las orbitas
//     que pasan DETRAS del planeta se tapan con su disco, para que de verdad giren
//     alrededor y no encima. NO se dibujan las lineas de la orbita (cruzaban el planeta como
//     una maraña): la estructura se lee en una estela y en la sombra del planeta, que apaga
//     lo que pasa por detras de el (un eclipse).
//   - Surface: luces fijas a la superficie (giran con la Tierra, que es la matriz de su
//     nodo). Se ven mas fuertes del lado de noche, que es donde una luz se ve.
//   - Sun: lo mismo que Orbit, alrededor del Sol (tambien cuando se mira otro cuerpo).
//   - Far: puntos que salen del planeta hacia el borde de la pantalla y se apagan.
//
// QUE DIBUJO LLEVA CADA UNIDAD lo dice `ArtOf` (por id): una silueta (satelite, espejo, relay,
// sonda), un efecto (humo, destellos, haz, pulso) o la cinta del ascensor. Una unidad que no
// este en la tabla cae en el punto de siempre, segun su Look. Lo que cada cosa debe ser y como
// crece con la cantidad esta en la hoja Families del Excel y en docs/plan-visual.md.
//
// Corre justo despues de GameRoot y antes del HUD y los paneles: la interfaz va encima.
//
// EL ARBOL DE TECNOLOGIAS de la Tierra (eras 1 a 4: la red electrica, los reactores, los laseres, la estacion, el
// anillo...) lo dibuja EarthFx.h, llamado desde aqui; las utilidades de dibujo comunes estan en FxKit.h.

#include "FxKit.h"
#include "EarthFx.h"
#include "MoonFx.h"
#include "SunFx.h"
#include "../Services/Cine.h"
#include "../Services/FxClock.h"

class WorldFx : public IScript {
public:
    void OnStart() override {
        m_earth = FindByPath(SceneService::MutableRoots(), kEarthPath);
        m_moon = FindByPath(SceneService::MutableRoots(), kMoonPath);
        m_sun = FindByPath(SceneService::MutableRoots(), kSunPath);
        m_fleetInScene = FindByPath(SceneService::MutableRoots(), "Fleet") != nullptr;
        // Para pruebas: EVERGRAVE_FXTIME=<segundos> CONGELA el reloj de los efectos en ese instante, asi una captura
        // de un cohete en pleno vuelo se puede repetir (a mano seria cuestion de suerte).
#ifdef _WIN32 // _dupenv_s es de MSVC: el clang de Android no lo declara (solo es una ayuda de pruebas en Windows)
        char* buf = nullptr;
        std::size_t len = 0;
        if (_dupenv_s(&buf, &len, "EVERGRAVE_FXTIME") == 0 && buf) {
            m_time = static_cast<float>(std::atof(buf));
            m_frozen = true;
            std::free(buf);
        }
#endif
    }

    void OnUpdate(float dt) override {
        if (!m_frozen) m_time += dt;
        if (Cine().active && !m_frozen) m_time = Cine().f.vt; // filmando, el reloj de los efectos ES el del mundo de la toma (repetible, y lento en el eclipse)
        FxClock().time = m_time; // Fleet (los satelites de la escena) lee este mismo reloj
        FxBus().beams.clear();   // los pedidos de este frame (lasers, haces) los llenan EarthFx y DrawSurface; Fleet los dibuja
        const GameState& game = Session();
        if (Ui().menu) return; // con el menu abierto no se dibujan las instalaciones
        // Sin unidades tambien se dibuja: los cohetes salen de las TECNOLOGIAS, no de la tienda.
        if (!m_earth || !m_moon || !m_sun) return;
        // En el mapa no se dibujan: lo construido se ve de cerca, en el cuerpo al que pertenece.
        if (Ui().mapView) return;
        const GameObject* camObj = FindActiveCamera(SceneService::Roots());
        if (!camObj || !camObj->camera) return;
        Cam cam;
        cam.w = UiService::Width();
        cam.h = UiService::Height();
        if (cam.w <= 0.0f || cam.h <= 0.0f) return;
        cam.view = ComputeGameCameraView(*camObj);
        cam.proj = ComputeGameCameraProjection(*camObj->camera, SceneService::Type(), cam.w / cam.h);

        // El cuerpo que se mira: lo construido alli se dibuja a su alrededor y encima de el.
        GameObject* focusObj = Ui().view == BodyId::Moon ? m_moon : Ui().view == BodyId::Sun ? m_sun : m_earth;
        // Del Sol solo hace falta DONDE esta (para saber que lado de un cuerpo es de noche): con la camara
        // del lado opuesto queda detras de ella y no se puede proyectar, y eso no es motivo para no dibujar.
        const Body focus = MakeBody(cam, *focusObj);
        if (!focus.ok) return;
        // El plano de enfoque de los efectos: el del cuerpo que se mira (lo mismo que hace CameraRig con la camara).
        g_focus.vz = focus.vz;
        g_focus.radius = focus.radius;
        g_mod = FxMod{}; // sin profundidad: lo que la fije la suelta al terminar, esto es por si una familia no lo hizo
        const XMVECTOR sunCenter = XMVector3TransformCoord(XMVectorZero(), ComputeWorldMatrix(*m_sun));
        // El Sol como cuerpo, para lo que se construye ALREDEDOR de el aunque se mire otro cuerpo (el anillo de espejos
        // de Mercurio): antes se dibujaba alrededor de la Tierra, que no es donde esta.
        const Body sunBody = MakeBody(cam, *m_sun);
        UiDrawList& ui = UiService::Frame();

        for (const InstallationDef& d : game.installations().All()) {
            if (d.body != Ui().view) continue;
            const int owned = game.Owned(d.id);
            if (owned <= 0) continue;
            const int shown = std::min(owned, d.visibleCap);
            const unsigned seed = HashString(d.id);
            const Art art = ArtOf(d.id);
            switch (d.look) {
                case InstallationLook::Orbit:
                case InstallationLook::Sun: {
                    const bool aroundSun = d.look == InstallationLook::Sun;
                    const bool sunFromAfar = aroundSun && Ui().view != BodyId::Sun;
                    if (sunFromAfar && !sunBody.ok) break; // el Sol esta detras de la camara
                    const Body& around = sunFromAfar ? sunBody : focus;
                    if (art == Art::Elevator) {
                        if (!m_fleetInScene) DrawElevator(ui, cam, around, d, owned, *focusObj); // en la escena lo dibuja Fleet
                    }
                    else DrawOrbit(ui, cam, around, sunCenter, d, art, owned, shown, seed, !aroundSun && Ui().view != BodyId::Sun);
                    if (art == Art::Relay) DrawRelayPulses(ui, cam, around, game, d, owned);
                    break;
                }
                case InstallationLook::Surface: DrawSurface(ui, cam, focus, sunCenter, d, art, owned, shown, seed, *focusObj); break;
                case InstallationLook::Far: DrawFar(ui, cam, focus, d, art, owned, shown, seed); break;
            }
        }
        if (Ui().view == BodyId::Earth) m_earthFx.Draw(ui, cam, focus, game, *focusObj, sunCenter, m_time);
        // El Sol, en las tres vistas donde se ve (la Tierra, la Luna y la suya): las tecnologias de las eras 5 y 8 lo rodean.
        if (sunBody.ok) m_sunFx.Draw(ui, cam, sunBody, game, m_time, Ui().view == BodyId::Sun, Ui().view == BodyId::Earth ? &focus : nullptr);
        if (Ui().view == BodyId::Moon) m_moonFx.Draw(ui, cam, focus, game, *focusObj, sunCenter, XMVector3TransformCoord(XMVectorZero(), ComputeWorldMatrix(*m_earth)), m_time);
    }

private:
    // Satelites (alrededor de la Tierra) o espejos y colectores (alrededor del Sol). SIN lineas de orbita: con 25 o mas
    // se dibujaban enteras y cruzaban el planeta como una maraña. La estructura se lee en una ESTELA corta, y larga
    // con muchas unidades, y en que los planos no se pisan.
    //
    // TRES PLANOS DE PROFUNDIDAD (DepthCue, FxKit.h): lo que esta DELANTE del plano de enfoque (mas cerca de la camara que el
    // planeta) se dibuja grande y desenfocado, un disco suave que pasa por delante de la imagen como en una toma con teleobjetivo;
    // lo que esta EN el plano (la franja alrededor del borde del planeta) es nitido y tiene su sprite; y lo que esta DETRAS
    // se achica a un punto, se enfria hacia el azul del espacio y se apaga. Es lo que da el "estas mirando un planeta con cosas
    // delante y detras" en vez de una capa unica de calcomanias.
    //
    // Y LUZ DE FASE: cada uno brilla segun cuanto de frente le da el Sol (el lado de dia se ve, el de noche es acero apagado),
    // ademas del eclipse. Antes todos tenian el mismo brillo salvo en la sombra del planeta.
    void DrawOrbit(UiDrawList& ui, const Cam& cam, const Body& around, const XMVECTOR& sunCenter, const InstallationDef& d, Art art, int owned, int shown,
                   unsigned seed, bool castShadow) const {
        // LOS SATELITES, ESPEJOS Y RELAYS DE LA TIERRA SON OBJETOS DE LA ESCENA (View/Fleet.cpp): reciben la luz del Sol, el desenfoque, el
        // brillo y el ajuste de color de verdad. Aqui solo queda el POLVO de los satelites (los que pasan de `kCrispSats`), que es un
        // punto de UI. Sin Fleet (otra escena) se dibuja todo como antes.
        const bool inScene = castShadow && m_fleetInScene;
        if (inScene && (art == Art::Mirror || art == Art::Relay)) return;
        const Band band = BandOf(art, castShadow);
        const float base = around.radius * band.base;
        const XMVECTOR toSun = castShadow ? XMVector3Normalize(XMVectorSubtract(sunCenter, around.center)) : XMVectorSet(0, 0, 1, 0);
        const int trail = owned >= 25 ? 6 : 3;
        const float spriteRadius = art == Art::Mirror ? 1.6f : art == Art::Relay ? 0.8f : 1.0f;

        for (int j = 0; j < shown; ++j) {
            const Slot s = FamilySlot(art, around.center, base, band.spread, seed, shown, j, m_time);
            float sx, sy, vz;
            if (!cam.Project(s.pos, sx, sy, vz)) continue;
            if (Occluded(around, sx, sy, vz)) continue;

            // Luz: el eclipse (la sombra del planeta lo apaga) por la fase (cuanto le da el Sol de frente).
            float lit = castShadow ? Eclipse(s.pos, around.center, toSun, around.radius) : 1.0f;
            if (castShadow) {
                const XMVECTOR r = XMVector3Normalize(XMVectorSubtract(s.pos, around.center));
                const float day = std::clamp(0.5f + 0.8f * XMVectorGetX(XMVector3Dot(r, toSun)), 0.0f, 1.0f);
                lit *= 0.42f + 0.58f * day;
            }
            if (inScene && art == Art::Sat) continue; // ya son objetos 3D (los nitidos) y el velo (el resto): Fleet
            const Cue cue = castShadow ? DepthCue(vz) : Cue{};
            const float a = cue.fade;
            const std::uint32_t hue = Hazed(d.color, cue.haze);
            const unsigned h = Hash(seed, static_cast<unsigned>(j));

            // El resto de los satelites: polvo tenue. Cuentan (la produccion) pero no compiten.
            if (art == Art::Sat && j >= kCrispSats) {
                Dot(ui, sx, sy, 1.0f, Shaded(hue, lit, 0.5f * a));
                continue;
            }
            // Fuera del plano de enfoque: un disco suave. Delante, mas grande; detras, mas chico y mas tenue.
            // Un punto de luz desenfocado toma el color de lo que lo origina: el metal al sol es casi blanco; de noche casi
            // no se ve (antes eran discos grises parejos, que parecian manchas en la imagen).
            if (cue.blur > 0.5f) {
                Bokeh(ui, sx, sy, cue.near ? spriteRadius : spriteRadius * 0.4f, cue.blur, Mixed(hue, 0xFFEFD8, 0.45f * lit),
                      (cue.near ? 1.0f : 0.8f) * a * (0.04f + 0.96f * lit * lit));
                continue;
            }
            // Detras del plano, pero todavia casi en foco: un punto, no el sprite.
            if (cue.depth > 0.9f) {
                Dot(ui, sx, sy, 1.0f, Shaded(hue, lit, 0.8f * a));
                continue;
            }

            // En el plano de enfoque: nitido, con su estela y su sprite. Un poco mas grande si esta bien adelante.
            for (int t = trail; t >= 1; --t) {
                float tx, ty, tz;
                if (!cam.Project(OrbitPoint(around.center, s.orbit, s.angle - s.orbit.speed * 0.05f * static_cast<float>(t)), tx, ty, tz)) continue;
                if (Occluded(around, tx, ty, tz)) continue;
                Dot(ui, tx, ty, 1.0f, Shaded(hue, lit, (0.45f - 0.06f * static_cast<float>(t)) * a));
            }
            const bool bigNear = cue.depth < -0.9f;
            switch (art) {
                case Art::Sat: DrawSprite(ui, owned >= 50 || bigNear ? kSatL : kSatS, sx, sy, hue, lit, a); break;
                case Art::Mirror: DrawSprite(ui, kMirror, sx, sy, hue, lit, a); break;
                case Art::Relay: {
                    // Un relay es un ACENTO: una baliza roja con su brillo, que late despacio. Solo los nitidos la llevan.
                    const float beat = 0.6f + 0.4f * std::sin(m_time * 2.0f + static_cast<float>(h & 0xFF));
                    Halo(ui, sx, sy, 4.0f, d.color, 0.8f * beat * a);
                    DrawSprite(ui, kRelay, sx, sy, hue, std::max(lit, 0.8f), a);
                    break;
                }
                case Art::Probe: DrawSprite(ui, kProbe, sx, sy, hue, lit, a); break;
                case Art::Collector:
                    if (owned >= 10) Halo(ui, sx, sy, 4.0f, d.color, 0.9f * a);
                    DrawSprite(ui, kCollector, sx, sy, hue, lit, a);
                    break;
                default:
                    if (owned >= 10) Halo(ui, sx, sy, 4.0f, d.color, 0.9f * a); // el brillo del hito
                    Dot(ui, sx, sy, 3.0f, Shaded(hue, lit, a));
                    Dot(ui, sx, sy, 1.0f, Shaded(0xFFFFFF, lit, a));
                    break;
            }
            // Un panel que atrapa el Sol un instante: raro (4% del tiempo) y solo a pleno sol.
            if ((art == Art::Sat || art == Art::Mirror) && lit > 0.85f && std::fmod(m_time * 0.25f + static_cast<float>(h & 0xFFFF) / 65536.0f, 1.0f) < 0.04f)
                Glint(ui, sx, sy, 0.9f * a);
        }
    }

    // Un relay no es un punto que gira y ya: su trabajo es pasar luz de un satelite al siguiente. Cada relay lanza un
    // PULSO corto entre dos satelites, que viaja y se apaga; nada de lineas fijas cruzando el planeta.
    void DrawRelayPulses(UiDrawList& ui, const Cam& cam, const Body& around, const GameState& game, const InstallationDef& relay, int owned) const {
        const InstallationDef* sat = nullptr;
        for (const InstallationDef& d : game.installations().All())
            if (d.id == "satellite") {
                sat = &d;
                break;
            }
        if (!sat) return;
        const int satShown = std::min(game.Owned(sat->id), sat->visibleCap);
        if (satShown < 2) return;
        const unsigned satSeed = HashString(sat->id);
        // La misma banda y el mismo numero de satelites que DrawOrbit: un pulso tiene que salir de un satelite que se VE.
        const Band satBand = BandOf(Art::Sat, true);
        const float base = around.radius * satBand.base;
        const int crisp = std::min(satShown, kCrispSats);
        const int pulses = std::clamp(owned, 1, 10);

        for (int r = 0; r < pulses; ++r) {
            const float period = 2.2f + 1.6f * Hash01(static_cast<unsigned>(r), 41u);
            const float t = m_time / period + Hash01(static_cast<unsigned>(r), 77u);
            const unsigned n = static_cast<unsigned>(std::floor(t));
            const float u = (t - std::floor(t)) / 0.4f; // el pulso viaja el primer 40% del ciclo
            if (u >= 1.0f) continue;
            const int a = static_cast<int>(Hash(n, static_cast<unsigned>(r)) % static_cast<unsigned>(crisp));
            const int b = (a + 1 + static_cast<int>(Hash(n, 99u + static_cast<unsigned>(r)) % 5u)) % crisp;
            if (a == b) continue;
            const XMVECTOR pa = FamilySlot(Art::Sat, around.center, base, satBand.spread, satSeed, satShown, a, m_time).pos;
            const XMVECTOR pb = FamilySlot(Art::Sat, around.center, base, satBand.spread, satSeed, satShown, b, m_time).pos;
            float ax, ay, az, bx, by, bz, mx, my, mz;
            if (!cam.Project(pa, ax, ay, az) || !cam.Project(pb, bx, by, bz) || !cam.Project(XMVectorLerp(pa, pb, 0.5f), mx, my, mz)) continue;
            // Si el planeta tapa cualquiera de los tres puntos, el pulso va por detras: no se ve.
            if (Occluded(around, ax, ay, az) || Occluded(around, bx, by, bz) || Occluded(around, mx, my, mz)) continue;
            for (int k = 0; k < 5; ++k) {
                const float tt = u - 0.06f * static_cast<float>(k);
                if (tt < 0.0f) break;
                const float x = ax + (bx - ax) * tt, y = ay + (by - ay) * tt;
                const float al = 1.0f - 0.2f * static_cast<float>(k);
                if (k == 0) Halo(ui, x, y, 4.0f, relay.color, 0.9f); // la cabeza del pulso brilla; la cola, no
                Dot(ui, x, y, k == 0 ? 2.0f : 1.0f, k == 0 ? ColorOf(0xFFF4F0, al) : ColorOf(relay.color, al));
            }
        }
    }

    // Una cinta fina del ecuador hacia arriba, con un contrapeso, y escaladores que suben y bajan. La altura es
    // estilizada (2,2 radios): la real, 5,6, se saldria de la pantalla. Fija al planeta, como una de verdad
    // (un ascensor es geoestacionario): gira con el.
    void DrawElevator(UiDrawList& ui, const Cam& cam, const Body& earth, const InstallationDef& d, int owned, const GameObject& bodyObj) const {
        const XMMATRIX m = ComputeWorldMatrix(bodyObj);
        const float lon = 0.9f;
        const XMVECTOR dir = XMVectorSet(std::cos(lon), 0.0f, std::sin(lon), 0.0f);
        auto at = [&](float h) { return XMVector3TransformCoord(XMVectorScale(dir, 1.0f + h), m); };
        const float top = 1.8f;
        const int N = 28;
        float px = 0, py = 0;
        bool havePrev = false;
        for (int i = 0; i < N; ++i) {
            float sx, sy, vz;
            const bool ok = cam.Project(at(top * static_cast<float>(i) / static_cast<float>(N - 1)), sx, sy, vz);
            const bool vis = ok && !Occluded(earth, sx, sy, vz);
            if (vis && havePrev) ui.AddLine(px, py, sx, sy, ColorOf(d.color, 0.45f), 1.0f);
            havePrev = vis;
            px = sx;
            py = sy;
        }
        float tx, ty, tz;
        if (cam.Project(at(top), tx, ty, tz) && !Occluded(earth, tx, ty, tz)) {
            Dot(ui, tx, ty, 3.0f, ColorOf(d.color));
            Dot(ui, tx, ty, 1.0f, ColorOf(0xFFFFFF));
        }
        const int climbers = std::min(owned, 8);
        for (int k = 0; k < climbers; ++k) {
            const float period = 9.0f + 3.0f * Hash01(static_cast<unsigned>(k), 5u);
            float u = std::fmod(m_time / period + Hash01(static_cast<unsigned>(k), 6u), 1.0f);
            u = u < 0.5f ? u * 2.0f : 2.0f - u * 2.0f; // sube y baja
            float sx, sy, vz;
            if (!cam.Project(at(top * u), sx, sy, vz) || Occluded(earth, sx, sy, vz)) continue;
            Dot(ui, sx, sy, 2.0f, ColorOf(0xFFFFFF, 0.9f));
        }
    }

    // Lo fijo a la superficie (la matriz del nodo ya lleva el giro). Cada tipo tiene su dibujo; los que no, el punto de
    // siempre. Lo que es de luz (mastiles, plantas, ciudades) se ve de noche; lo que es de sol (paneles) de dia.
    void DrawSurface(UiDrawList& ui, const Cam& cam, const Body& earth, const XMVECTOR& sunCenter, const InstallationDef& d, Art art, int owned, int shown,
                     unsigned seed, const GameObject& bodyObj) const {
        const XMMATRIX m = ComputeWorldMatrix(bodyObj);
        const XMVECTOR vc = XMVector3TransformCoord(earth.center, cam.view);
        const XMVECTOR toSun = XMVector3Normalize(XMVectorSubtract(sunCenter, earth.center));
        for (int j = 0; j < shown; ++j) {
            const unsigned uj = static_cast<unsigned>(j);
            // Latitud y longitud fijas por (tipo, numero): la que se compra primero queda donde esta.
            const float lat = (Hash01(seed, uj) - 0.42f) * 2.1f;
            const float lon = Hash01(seed, 500u + uj) * 2.0f * kPi;
            const XMVECTOR local = XMVectorSet(std::cos(lat) * std::cos(lon), std::sin(lat), std::cos(lat) * std::sin(lon), 0.0f);
            const XMVECTOR world = XMVector3TransformCoord(local, m);

            const XMVECTOR pv = XMVector3TransformCoord(world, cam.view);
            const XMVECTOR n = XMVectorSubtract(pv, vc);
            if (XMVectorGetX(XMVector3Dot(n, XMVectorNegate(pv))) <= 0.0f) continue; // del otro lado del planeta
            float sx, sy, vz;
            if (!cam.Project(world, sx, sy, vz)) continue;

            // Del lado de noche (lejos del Sol) la luz se ve; del de dia apenas.
            const XMVECTOR nw = XMVector3Normalize(XMVectorSubtract(world, earth.center));
            const float lit = XMVectorGetX(XMVector3Dot(nw, toSun)); // 1 = mediodia, -1 = medianoche
            const float night = std::clamp(0.5f - lit * 0.5f, 0.0f, 1.0f);
            const float phase = Hash01(seed, 900u + uj) * 6.28f;
            const float twinkle = 0.75f + 0.25f * std::sin(m_time * 2.2f + phase);
            const float a = (0.28f + 0.72f * night) * twinkle;
            // El punto `h` radios por encima de este, en pantalla: hacia donde "sube" lo que sale del suelo.
            auto up = [&](float h, float& ux, float& uy) {
                float uz;
                return cam.Project(XMVectorAdd(world, XMVectorScale(nw, earth.radius * h)), ux, uy, uz);
            };

            switch (art) {
                case Art::Mast: { // un mastil con una luz que parpadea
                    ui.AddRect(kit::Rect(std::floor(sx), std::floor(sy) - 2.0f, 1.0f, 3.0f), ColorOf(0xB8C8E0, 0.35f + 0.4f * night));
                    if (std::fmod(m_time * 0.7f + phase, 1.0f) < 0.3f)
                        ui.AddRect(kit::Rect(std::floor(sx), std::floor(sy) - 3.0f, 1.0f, 1.0f), ColorOf(0xFFFFFF, 0.5f + 0.5f * night));
                    break;
                }
                case Art::Plume: { // brasa en la base y una columna de humo que sube y se deshace
                    Dot(ui, sx, sy, 2.0f, ColorOf(d.color, 0.25f + 0.75f * night));
                    float tx, ty;
                    if (!up(0.12f + 0.06f * Hash01(seed, 31u + uj), tx, ty)) break;
                    for (int i = 0; i < 4; ++i) {
                        const float f = (static_cast<float>(i) + std::fmod(m_time * 0.4f + phase, 1.0f)) / 4.0f;
                        const float x = sx + (tx - sx) * f + std::sin(m_time + phase + f * 3.0f) * 1.2f * f, y = sy + (ty - sy) * f;
                        Dot(ui, x, y, f < 0.4f ? 2.0f : 1.0f, ColorOf(0xE6E6EE, (1.0f - f) * 0.55f));
                    }
                    break;
                }
                case Art::Solar: { // un parche azul oscuro que de dia atrapa destellos
                    const float day = 1.0f - 0.8f * night;
                    ui.AddRect(kit::Rect(std::floor(sx), std::floor(sy), 2.0f, 2.0f), ColorOf(0x2A4A8A, day));
                    if (lit > 0.5f && std::sin(m_time * 1.7f + phase) > 0.92f) Glint(ui, sx + 0.5f, sy + 0.5f, 0.9f);
                    break;
                }
                case Art::LaserGrid: { // una cuadricula de agujas que se encienden en ola
                    for (int row = 0; row < 2; ++row)
                        for (int col = 0; col < 3; ++col) {
                            const float wave = std::fmod(m_time * 0.8f - static_cast<float>(col) * 0.2f - static_cast<float>(row) * 0.1f + phase, 1.0f);
                            const float on = wave < 0.3f ? 1.0f : 0.35f;
                            ui.AddRect(kit::Rect(std::floor(sx) + static_cast<float>(col) * 2.0f, std::floor(sy) + static_cast<float>(row) * 2.0f, 1.0f, 1.0f),
                                       ColorOf(d.color, std::min(1.0f, a * on)));
                        }
                    break;
                }
                case Art::City: { // un distrito: cuatro luces cerca, doradas, de noche
                    for (int i = 0; i < 4; ++i) {
                        const unsigned k = uj * 7u + static_cast<unsigned>(i);
                        const float ox = (Hash01(seed, 1000u + k) - 0.5f) * 5.0f;
                        const float oy = (Hash01(seed, 2000u + k) - 0.5f) * 3.0f;
                        Dot(ui, sx + ox, sy + oy, 1.0f, ColorOf(d.color, a * (0.6f + 0.4f * Hash01(seed, 3000u + k))));
                    }
                    if (owned >= 10) Halo(ui, sx, sy, 5.0f, d.color, 0.5f * a); // el resplandor de una ciudad grande
                    break;
                }
                case Art::Beam: { // una estacion optica: un punto y un haz hacia arriba, solo de noche y a rafagas
                    Dot(ui, sx, sy, 2.0f, ColorOf(d.color, a));
                    float tx, ty;
                    if (night > 0.5f && std::fmod(m_time * 0.5f + phase, 1.0f) < 0.5f) {
                        if (m_fleetInScene) {
                            // El haz es un cilindro aditivo de la escena (Fleet): sube de la estacion un tercio de radio. Lo tapa el planeta solo,
                            // por la profundidad, asi no hace falta el `NearSide` de arriba (que si deja a la estacion de la cara oculta sin haz).
                            const XMVECTOR tip = XMVectorAdd(world, XMVectorScale(nw, earth.radius * 0.3f));
                            WorldBeam wb{};
                            XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(wb.a), world);
                            XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(wb.b), tip);
                            wb.width = 0.035f;
                            wb.rgb[0] = static_cast<float>((d.color >> 16) & 0xFF) / 255.0f;
                            wb.rgb[1] = static_cast<float>((d.color >> 8) & 0xFF) / 255.0f;
                            wb.rgb[2] = static_cast<float>(d.color & 0xFF) / 255.0f;
                            wb.power = 1.2f;
                            FxBus().beams.push_back(wb);
                        } else if (up(0.3f, tx, ty)) {
                            ui.AddLine(sx, sy, tx, ty, ColorOf(d.color, 0.7f), 1.0f);
                            Dot(ui, tx, ty, 2.0f, ColorOf(0xFFFFFF, 0.8f));
                        }
                    }
                    break;
                }
                case Art::Pit: { // un hoyo oscuro con polvo
                    ui.AddRect(kit::Rect(std::floor(sx), std::floor(sy), 2.0f, 2.0f), ColorOf(0x14161C, 0.9f));
                    float tx, ty;
                    if (up(0.05f, tx, ty)) Dot(ui, tx, ty - std::fmod(m_time * 2.0f + phase, 2.0f), 1.0f, ColorOf(0xC8D0DC, 0.5f));
                    break;
                }
                case Art::Forge: { // un horno que parpadea en la noche lunar
                    const float flick = 0.6f + 0.4f * std::sin(m_time * 9.0f + phase) * std::sin(m_time * 5.3f + phase * 2.0f);
                    Halo(ui, sx, sy, 4.0f, d.color, 0.6f * flick * (0.4f + 0.6f * night));
                    Dot(ui, sx, sy, 2.0f, ColorOf(d.color, (0.35f + 0.65f * night) * flick));
                    break;
                }
                case Art::Dome: {
                    DrawSprite(ui, kDome, sx, sy, d.color, 0.6f + 0.4f * (1.0f - night), 0.9f);
                    if (night > 0.5f) Dot(ui, sx, sy + 0.5f, 1.0f, ColorOf(0xFFE6A0, 0.8f * twinkle));
                    break;
                }
                default:
                    if (owned >= 10) Halo(ui, sx, sy, 4.0f, d.color, 0.6f * a); // brillo del hito
                    Dot(ui, sx, sy, 2.0f, ColorOf(d.color, a));
                    break;
            }
        }
    }

    // Puntos que salen de la Tierra hacia el borde de la pantalla y se apagan.
    void DrawFar(UiDrawList& ui, const Cam& cam, const Body& earth, const InstallationDef& d, Art art, int owned, int shown, unsigned seed) const {
        (void)owned;
        const UiRect vis = UiService::VisibleRect();
        // Pocos y con rumbo: 24 naves saliendo en todas las direcciones eran confeti. Con 8 (10 velas) se lee un trafico, no una nube.
        const int visible = std::min(shown, art == Art::Sail ? 10 : 8);
        for (int j = 0; j < visible; ++j) {
            const float dir = Hash01(seed, static_cast<unsigned>(j)) * 2.0f * kPi;
            const float period = 7.0f + 6.0f * Hash01(seed, 300u + static_cast<unsigned>(j));
            const float u = std::fmod(m_time / period + Hash01(seed, 700u + static_cast<unsigned>(j)), 1.0f);
            // Hasta el borde mas lejano en esa direccion.
            const float reach = std::max(vis.width, vis.height) * 0.6f;
            const float r = earth.sr * 1.2f + u * u * reach;
            const float x = earth.sx + std::cos(dir) * r, y = earth.sy + std::sin(dir) * r * 0.62f;
            if (!vis.Contains(x, y)) continue;
            const float a = u < 0.15f ? u / 0.15f : 1.0f - (u - 0.15f) / 0.85f;
            const float cx = std::cos(dir), cy = std::sin(dir) * 0.62f;
            if (art == Art::Sail) {
                DrawSprite(ui, kSail, x, y, d.color, 1.0f, a * 0.8f);
            } else {
                // Una nave: cabeza y estela larga detras (cuatro puntos que se apagan).
                Dot(ui, x, y, 2.0f, ColorOf(d.color, a));
                for (int t = 1; t <= 4; ++t)
                    Dot(ui, x - cx * 3.0f * static_cast<float>(t), y - cy * 3.0f * static_cast<float>(t), 1.0f, ColorOf(d.color, a * (0.5f - 0.1f * static_cast<float>(t))));
            }
        }
        (void)cam;
    }

    GameObject* m_earth = nullptr;
    GameObject* m_moon = nullptr;
    GameObject* m_sun = nullptr;
    float m_time = 0.0f;
    bool m_frozen = false;
    bool m_fleetInScene = false;
    EarthFx m_earthFx;
    MoonFx m_moonFx;
    SunFx m_sunFx;
};

STRACE_REGISTER_SCRIPT(WorldFx)
