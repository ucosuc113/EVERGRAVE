// Fleet: los satelites, espejos y relays de la Tierra COMO OBJETOS DE LA ESCENA.
//
// POR QUE. Hasta aqui todo lo que se compraba se dibujaba como UI (WorldFx): rectangulos encima de la imagen, despues del bloom, del
// desenfoque y del ajuste de color del motor. Habia que IMITAR cada uno (halos de discos, bokeh de aros, paleta), y una imitacion en
// rectangulos tiene techo: nunca es un desenfoque real ni recibe la luz del Sol. Un objeto de la escena recibe todo eso sin pedirlo:
// el Sol lo ilumina (el lado de noche es silueta de verdad), la profundidad lo desenfoca (lo de delante y lo de atras del foco),
// el brillo lo alcanza y el grade lo tine. Es el patron del cinturon de asteroides (AsteroidBelt.cpp): la ESCENA guarda las piezas
// (`Fleet/Sat00`, `Fleet/Mirror07`... las escribe Tools/gen_fleet.py), y este script solo decide cuantas estan encendidas y donde.
//
// QUE ANIMA
//  - Cuantas piezas hay: lo comprado (`Session().Owned`) tope `visibleCap`; las que sobran se apagan (`active = false`). Los satelites
//    nitidos son `kCrispSats` (40); el resto es polvo de UI en WorldFx.
//  - DONDE: la MISMA cuenta que WorldFx (`OrbitSlot` + `BandOf`, FxKit.h) con el MISMO reloj (`FxClock`): los pulsos de relay, que
//    siguen siendo UI, nacen y mueren sobre los satelites de la escena. Si una cuenta cambia, cambia en las dos.
//  - COMO MIRA: el satelite apunta al planeta (nadir), con sus paneles de lado. El espejo no apunta a un sitio fijo: su normal oscila
//    entre "hacia afuera" y la normal que le devuelve el Sol a la camara (el vector medio), de modo que cada tanto uno de ellos
//    atrapa la luz y destella - con el brillo especular del material, no con un sprite.
//  - EL TAMANO: se mide en mundo, asi que perspectiva y zoom son gratis; con la camara mas lejos que la de la toma por defecto
//    (`kRefDist`) las piezas crecen para no bajar de unos pocos pixeles (360 filas: algo de menos de dos pixeles parpadea al moverse).
//
// 3c: TAMBIEN SON DE LA ESCENA los laseres y haces (cilindros aditivos de un pool, estirados entre los dos puntos que piden EarthFx y
// WorldFx por el buzon `Services/FxBus.h`: la logica de CUANDO dispara sigue alla), el anillo orbital (una malla fina, la tapa el planeta por
// profundidad, no con una cuenta de pantalla), la estacion modular (cada modulo se enciende por su tecnologia), el panel solar espacial y el
// ascensor (una cinta aditiva con escaladores).
//
// QUE NO HACE (y se sabe): el eclipse. La luz del Sol no tiene sombra del planeta sobre ellos, asi que un satelite en la sombra de la
// Tierra sigue iluminado. Desde la camara casi siempre queda detras del disco; si molesta, se apaga su material aqui.
//
// Se activa SOLO en la vista de la Tierra (sin menu ni mapa): el resto de las vistas no tienen estas piezas.

#include "FxKit.h"
#include "../Services/Cine.h"
#include "../Services/FxClock.h"

#include "Engine/Rendering/Material.h"

#include <string>

class Fleet : public IScript {
public:
    void OnStart() override {
        m_root = FindByPath(SceneService::MutableRoots(), "Fleet");
        m_earth = FindByPath(SceneService::MutableRoots(), kEarthPath);
        m_sun = FindByPath(SceneService::MutableRoots(), kSunPath);
        if (!m_root || !m_earth || !m_sun) {
            Log::Warn(L"EVERGRAVE: la escena no tiene `Fleet` (Tools/gen_fleet.py): los satelites se dibujan como UI");
            m_root = nullptr;
            return;
        }
        FxBus().fleetInScene = true;
        for (GameObject* c : ChildrenOf(*m_root)) {
            if (StartsWith(c->name, "Sat")) m_sats.push_back(Piece{c, c->transform.scale[0]});
            else if (StartsWith(c->name, "Mirror")) m_mirrors.push_back(Piece{c, c->transform.scale[0]});
            else if (StartsWith(c->name, "Relay")) m_relays.push_back(Piece{c, c->transform.scale[0]});
            else if (StartsWith(c->name, "Link")) m_links.push_back(Beam3{c, SceneService::UniqueMaterial(*c)});
            else if (c->name == "Veil") m_veil = Beam3{c, SceneService::UniqueMaterial(*c)};
            else if (StartsWith(c->name, "Beam")) m_beams.push_back(Beam3{c, SceneService::UniqueMaterial(*c)}); // su PROPIO material: el color cambia por haz
            else if (StartsWith(c->name, "RingPulse")) m_pulses.push_back(c);
            else if (c->name == "Ring") m_ring = c;
            else if (c->name == "Station") m_station = c;
            else if (c->name == "Wing") m_wing = c;
            else if (c->name == "ElevCable") m_cable = c;
            else if (c->name == "ElevWeight") m_weight = c;
            else if (StartsWith(c->name, "ElevClimber")) m_climbers.push_back(c);
        }
    }
    // Un OnDestroy que apague la bandera: si Fleet se va (otra escena, parar el Play), WorldFx tiene que volver a dibujar todo como UI.
    void OnDestroy() override { FxBus().fleetInScene = false; }

    void OnUpdate(float dt) override {
        if (!m_root) return;
        m_dt = dt;
        const UiState& ui = Ui();
        const bool show = !ui.menu && !ui.mapView && ui.view == BodyId::Earth;
        m_root->active = show;
        if (!show) return;

        const XMFLOAT3 centerF = ComputeWorldPosition(*m_earth);
        const XMVECTOR center = XMLoadFloat3(&centerF);
        const XMMATRIX em = ComputeWorldMatrix(*m_earth);
        const float radius = XMVectorGetX(XMVector3Length(XMVectorSubtract(XMVector3TransformCoord(XMVectorSet(1, 0, 0, 0), em), center)));
        const XMFLOAT3 sunF = ComputeWorldPosition(*m_sun);
        const XMVECTOR toSun = XMVector3Normalize(XMVectorSubtract(XMLoadFloat3(&sunF), center));
        XMVECTOR cam = XMVectorAdd(center, XMVectorSet(0, 0, kRefDist, 0));
        if (const GameObject* camObj = FindActiveCamera(SceneService::Roots())) {
            const XMFLOAT3 p = ComputeWorldPosition(*camObj);
            cam = XMLoadFloat3(&p);
        }
        // Crecer con la distancia a la camara para que el tamano en pantalla no baje de lo de la toma por defecto.
        const float grow = std::max(1.0f, XMVectorGetX(XMVector3Length(XMVectorSubtract(cam, center))) / kRefDist);
        const float time = FxClock().time;
        const GameState& game = Session();

        Place(m_sats, m_satState, game, "satellite", Art::Sat, center, radius, time, [&](const Slot& s, const Piece& p, int j) {
            // El satelite mira al planeta; sus paneles quedan de lado.
            AimZ(*p.node, XMVectorSubtract(center, s.pos));
            (void)j;
        }, grow, kCrispSats);

        Place(m_mirrors, m_mirrorState, game, "orbital_mirror", Art::Mirror, center, radius, time, [&](const Slot& s, const Piece& p, int j) {
            const XMVECTOR outward = XMVector3Normalize(XMVectorSubtract(s.pos, center));
            const XMVECTOR toCam = XMVector3Normalize(XMVectorSubtract(cam, s.pos));
            const XMVECTOR half = XMVector3Normalize(XMVectorAdd(toSun, toCam)); // la normal que le devuelve el Sol a la camara
            const float flash = std::pow(std::max(0.0f, std::sin(time * 0.35f + Hash01(static_cast<unsigned>(j), 311u) * 6.2832f)), 14.0f);
            AimY(*p.node, XMVector3Normalize(XMVectorLerp(outward, half, flash)));
        }, grow, 1 << 20);

        // Los relays son un ACENTO, no una nube: se ven pocos (`kCrispRelays`), brillantes y cada uno con su luz. Los demas cuentan pero no se dibujan.
        Place(m_relays, m_relayState, game, "laser_relay", Art::Relay, center, radius, time, [](const Slot&, const Piece&, int) {}, grow, kCrispRelays);

        UpdateVeil(game, center, radius, time, toSun);
        UpdateLinks(time);
        UpdateBeams(grow);
        UpdateRing(game, center, radius, time, grow);
        UpdateStation(game, center, radius, time, grow);
        UpdateWing(game, center, radius, time, grow);
        UpdateElevator(game, em, center, radius, time, grow);
    }

private:
    struct Piece {
        GameObject* node;
        float baseScale;
    };
    struct Beam3 {
        GameObject* node;
        Material* mat;
    };

    static void SetPos(GameObject& o, const XMVECTOR& p) {
        XMFLOAT3 f;
        XMStoreFloat3(&f, p);
        o.transform.position[0] = f.x;
        o.transform.position[1] = f.y;
        o.transform.position[2] = f.z;
    }
    static void SetScale(GameObject& o, float x, float y, float z) {
        o.transform.scale[0] = x;
        o.transform.scale[1] = y;
        o.transform.scale[2] = z;
    }
    static void SetUniform(GameObject& o, float s) { SetScale(o, s, s, s); }
    static GameObject* Part(GameObject& o, const char* name) {
        for (auto& c : o.children)
            if (c->name == name) return c.get();
        return nullptr;
    }

    // Un cilindro (eje Y) entre dos puntos del mundo: en el medio, girado hacia `b`, de largo |b - a| y `width` de diametro.
    static void Stretch(GameObject& o, const XMVECTOR& a, const XMVECTOR& b, float width) {
        const XMVECTOR d = XMVectorSubtract(b, a);
        const float len = XMVectorGetX(XMVector3Length(d));
        o.active = len > 1e-4f;
        if (!o.active) return;
        SetPos(o, XMVectorScale(XMVectorAdd(a, b), 0.5f));
        SetScale(o, width, len, width);
        AimY(o, d);
    }

    // ------------------------------------------------------------------ el velo
    // Pasados los satelites nitidos, el resto NO se dibuja suelto: es un VELO, una cascara de puntos finos sobre tres planos orbitales
    // (Veil.hlsl) que gira despacio. La densidad sale de cuantos hay (de 40 a ~260: de unas chispas a una cascara). Se mide por lo
    // COMPRADO y no por `visibleCap`: comprar el satelite 200 tiene que verse aunque el tope de objetos sea 60.
    void UpdateVeil(const GameState& game, const XMVECTOR& center, float radius, float time, const XMVECTOR& toSun) {
        (void)toSun;
        if (!m_veil.node) return;
        const int owned = game.Owned("satellite");
        const float t = std::clamp((static_cast<float>(owned) - static_cast<float>(kCrispSats)) / 220.0f, 0.0f, 1.0f);
        const bool has = owned > kCrispSats;
        m_veil.node->active = has;
        if (!has) return;
        SetPos(*m_veil.node, center);
        SetUniform(*m_veil.node, radius * 1.22f);
        m_veil.node->transform.rotation[0] = 0.0f;
        m_veil.node->transform.rotation[1] = std::fmod(time * 3.0f, 360.0f);
        m_veil.node->transform.rotation[2] = 0.0f;
        if (m_veil.mat) m_veil.mat->userParams[0].x = t * t * (3.0f - 2.0f * t);
    }

    // ------------------------------------------------------------------ enlaces entre relays
    // Los relays son pocos y su trabajo es hablar: entre dos vecinos se enciende un enlace fino, un rato corto y a su ritmo. No hay lineas
    // fijas (docs/plan-visual.md, principio 7): lo permanente cruzando el planeta es justo el desorden que se quita.
    void UpdateLinks(float time) {
        int active = 0;
        for (const Piece& r : m_relays)
            if (r.node->active) ++active;
        for (std::size_t k = 0; k < m_links.size(); ++k) {
            Beam3& l = m_links[k];
            l.node->active = false;
            if (active < 2 || static_cast<int>(k) >= active) continue;
            const int a = static_cast<int>(k), b = (static_cast<int>(k) + 1) % active;
            const float period = 3.0f + 2.2f * Hash01(static_cast<unsigned>(k), 91u);
            const float u = std::fmod(time / period + Hash01(static_cast<unsigned>(k), 92u), 1.0f) / 0.3f; // enciende el 30% del ciclo
            if (u >= 1.0f) continue;
            const XMFLOAT3 pa(m_relays[static_cast<std::size_t>(a)].node->transform.position[0], m_relays[static_cast<std::size_t>(a)].node->transform.position[1],
                              m_relays[static_cast<std::size_t>(a)].node->transform.position[2]);
            const XMFLOAT3 pb(m_relays[static_cast<std::size_t>(b)].node->transform.position[0], m_relays[static_cast<std::size_t>(b)].node->transform.position[1],
                              m_relays[static_cast<std::size_t>(b)].node->transform.position[2]);
            Stretch(*l.node, XMLoadFloat3(&pa), XMLoadFloat3(&pb), 0.012f);
            if (l.mat) {
                l.mat->albedoColor = XMFLOAT4(0.6f, 0.92f, 1.0f, 1.0f);
                l.mat->userParams[0].y = 1.1f * std::sin(kPi * u);
            }
        }
    }

    // ------------------------------------------------------------------ haces
    // Los pedidos del buzon, en orden, a los cilindros del pool. Un haz que no entra (mas pedidos que cilindros) se pierde: son 12 y el
    // juego pide como mucho 3 laboratorios por 5 diodos y las estaciones opticas de noche.
    void UpdateBeams(float grow) {
        // LA CINEMATICA pide un hilo de luz de la Tierra a la Luna (toma 5). Se agrega AQUI y no en el script de la toma porque WorldFx
        // vacia el buzon al principio de su frame, despues de la camara y antes que esto.
        if (Cine().active && Cine().f.beam != 0) {
            const CineWorld& w = Cine().world;
            // El hilo va a la Luna (1) o, cuando la historia mira al Sol, hacia el Sol que se ve (2): de la Tierra hasta su borde.
            const bool toSun = Cine().f.beam == 2;
            const CineV3 to = toSun ? w.sun : w.moon;
            const float dx = to.x - w.earth.x, dz = to.z - w.earth.z;
            const float len = std::max(0.001f, std::sqrt(dx * dx + dz * dz));
            const float ux = dx / len, uz = dz / len;
            // Nace en el borde de la Tierra y viaja hasta su destino en unos segundos.
            const float reach = CineSmooth((Cine().f.shotT - 1.2f) / (toSun ? 3.6f : 2.6f)); // el primer segundo es negro de Tierra: sin hilo todavia
            const float x0 = w.earth.x + ux * 1.9f, z0 = w.earth.z + uz * 1.9f;
            const float x1 = to.x - ux * (toSun ? 5.6f : 0.62f), z1 = to.z - uz * (toSun ? 5.6f : 0.62f);
            WorldBeam wb{};
            wb.a[0] = x0; wb.a[1] = 0.0f; wb.a[2] = z0;
            wb.b[0] = x0 + (x1 - x0) * reach; wb.b[1] = 0.0f; wb.b[2] = z0 + (z1 - z0) * reach;
            wb.width = 0.07f;
            wb.rgb[0] = 1.0f; wb.rgb[1] = 0.80f; wb.rgb[2] = 0.52f;
            wb.power = 1.25f + 0.3f * std::sin(Cine().f.t * 2.4f);
            if (reach > 0.01f) FxBus().beams.push_back(wb); // recien nacido tendria largo cero: no se estira
        }
        const std::vector<WorldBeam>& req = FxBus().beams;
        for (std::size_t k = 0; k < m_beams.size(); ++k) {
            Beam3& b = m_beams[k];
            if (k >= req.size()) {
                b.node->active = false;
                continue;
            }
            const WorldBeam& r = req[k];
            Stretch(*b.node, XMVectorSet(r.a[0], r.a[1], r.a[2], 0), XMVectorSet(r.b[0], r.b[1], r.b[2], 0), r.width * grow);
            if (b.mat) {
                b.mat->albedoColor = XMFLOAT4(r.rgb[0], r.rgb[1], r.rgb[2], 1.0f);
                b.mat->userParams[0].y = r.power;
            }
        }
    }

    // ------------------------------------------------------------------ anillo orbital
    // La misma inclinacion y el mismo radio (1,16) que dibujaba la UI. Es una malla fina (Tools/gen_ring_gltf.py) de radio 1: la escala
    // la lleva al del planeta. Tres pulsos corren por el, como antes.
    void UpdateRing(const GameState& game, const XMVECTOR& center, float radius, float time, float grow) {
        const bool has = game.tree().IsResearched("orbital_ring");
        if (m_ring) m_ring->active = has;
        for (GameObject* p : m_pulses) p->active = has;
        if (!has || !m_ring) return;
        SetPos(*m_ring, center);
        SetUniform(*m_ring, radius * 1.16f);
        m_ring->transform.rotation[0] = 0.35f * 180.0f / kPi;
        m_ring->transform.rotation[1] = 0.6f * 180.0f / kPi;
        m_ring->transform.rotation[2] = 0.0f;
        const XMMATRIX rot = XMMatrixRotationX(0.35f) * XMMatrixRotationY(0.6f);
        for (std::size_t k = 0; k < m_pulses.size(); ++k) {
            const float a = time * 0.5f + static_cast<float>(k) * 2.1f;
            const XMVECTOR v = XMVector3TransformNormal(XMVectorSet(std::cos(a), 0.0f, std::sin(a), 0.0f), rot);
            SetPos(*m_pulses[k], XMVectorAdd(center, XMVectorScale(v, radius * 1.16f)));
            SetUniform(*m_pulses[k], 0.09f * grow);
        }
    }

    // ------------------------------------------------------------------ estacion modular y panel solar
    // La orbita y la inclinacion son las de la UI (misma semilla): una sola pieza, lenta, que crece por modulos tecnologia a tecnologia.
    void UpdateStation(const GameState& game, const XMVECTOR& center, float radius, float time, float grow) {
        if (!m_station) return;
        const TechTree& t = game.tree();
        const bool has = t.IsResearched("orbital_assembly");
        m_station->active = has;
        if (!has) return;
        const Slot s = OrbitSlot(center, radius * 1.3f, 5151u, 1, 0, time * 0.25f);
        SetPos(*m_station, s.pos);
        SetUniform(*m_station, grow);
        AimZ(*m_station, XMVectorSubtract(center, s.pos));
        const bool complex = t.IsResearched("orbital_complex");
        const auto on = [&](const char* part, bool value) {
            if (GameObject* o = Part(*m_station, part)) o->active = value;
        };
        on("Lab", t.IsResearched("microgravity_labs"));
        on("Foundry", t.IsResearched("orbital_foundries"));
        on("Spar", t.IsResearched("zero_g_manufacturing"));
        on("Hab", t.IsResearched("orbital_habitats"));
        const bool hub = t.IsResearched("orbital_hub");
        on("HubA", hub);
        on("HubB", hub);
        for (const char* n : {"S1", "S2", "S3", "S4"}) on(n, complex);
        on("Glow", complex);
        if (GameObject* hab = Part(*m_station, "Hab")) hab->transform.rotation[1] = std::fmod(time * 40.0f, 360.0f); // el anillo habitado gira
    }

    void UpdateWing(const GameState& game, const XMVECTOR& center, float radius, float time, float grow) {
        if (!m_wing) return;
        const bool has = game.tree().IsResearched("space_solar_arrays");
        m_wing->active = has;
        if (!has) return;
        const Slot s = OrbitSlot(center, radius * 1.5f, 6262u, 1, 0, time * 0.2f);
        SetPos(*m_wing, s.pos);
        SetUniform(*m_wing, grow);
        AimZ(*m_wing, XMVectorSubtract(center, s.pos));
    }

    // ------------------------------------------------------------------ ascensor espacial
    // Una cinta del ecuador hacia arriba (1,8 radios de altura, estilizada: la real, 5,6, se saldria del cuadro) con un contrapeso y hasta 8
    // escaladores que suben y bajan. Fija al planeta (es geoestacionario): sale de la matriz del nodo de la Tierra, asi que gira con el.
    void UpdateElevator(const GameState& game, const XMMATRIX& earthM, const XMVECTOR& center, float radius, float time, float grow) {
        (void)center;
        (void)radius;
        if (!m_cable) return;
        const InstallationDef* def = game.installations().Find("space_elevator");
        const int owned = def ? game.Owned("space_elevator") : 0;
        const bool has = owned > 0;
        m_cable->active = has;
        if (m_weight) m_weight->active = has;
        for (GameObject* c : m_climbers) c->active = false;
        if (!has) return;
        const XMVECTOR dir = XMVectorSet(std::cos(0.9f), 0.0f, std::sin(0.9f), 0.0f);
        const auto at = [&](float h) { return XMVector3TransformCoord(XMVectorScale(dir, 1.0f + h), earthM); };
        const float top = 1.8f;
        Stretch(*m_cable, at(0.0f), at(top), 0.025f * grow);
        if (m_weight) {
            SetPos(*m_weight, at(top));
            SetUniform(*m_weight, 0.05f * grow);
        }
        const int climbers = std::min({owned, 8, static_cast<int>(m_climbers.size())});
        for (int k = 0; k < climbers; ++k) {
            const float period = 9.0f + 3.0f * Hash01(static_cast<unsigned>(k), 5u);
            float u = std::fmod(time / period + Hash01(static_cast<unsigned>(k), 6u), 1.0f);
            u = u < 0.5f ? u * 2.0f : 2.0f - u * 2.0f; // sube y baja
            GameObject* c = m_climbers[static_cast<std::size_t>(k)];
            c->active = true;
            SetPos(*c, at(top * u));
            SetUniform(*c, 0.04f * grow);
        }
    }

    static bool StartsWith(const std::string& s, const char* prefix) { return s.rfind(prefix, 0) == 0; }

    static std::vector<GameObject*> ChildrenOf(GameObject& o) {
        std::vector<GameObject*> out;
        for (auto& c : o.children) out.push_back(c.get());
        return out;
    }

    // Pone la familia `art`: `pieces.size()` es el pool; se encienden las primeras `min(comprado, cap)`.
    // El ultimo que se compro CRECE un instante (1 -> 2,4 -> 1 en `kBuyPulse` segundos), asi comprar algo se ve aunque el cuerpo sea un punto
    // entre cuarenta: sin esto, con las formaciones, una compra nueva se pierde en el tren.
    struct FamilyState {
        int prev = -1; // cuantas estaban encendidas el frame anterior (-1: todavia no se vio ninguna: la carga de una partida no cuenta)
        int newest = -1;
        float pulse = 99.0f;
    };
    static constexpr float kBuyPulse = 0.9f;

    template <class Orient>
    void Place(std::vector<Piece>& pieces, FamilyState& st, const GameState& game, const char* id, Art art, const XMVECTOR& center, float radius, float time,
               Orient&& orient, float grow, int crisp) {
        const InstallationDef* def = game.installations().Find(id);
        const int owned = def ? game.Owned(id) : 0;
        const int shown = def ? std::min(owned, def->visibleCap) : 0; // el MISMO `shown` que WorldFx: de el depende donde cae cada pieza
        const int lit = std::min({shown, crisp, static_cast<int>(pieces.size())});
        const Band band = BandOf(art, true);
        const unsigned seed = def ? HashString(def->id) : 0u;
        // Filmando NO hay pulso de compra: la cinematica hace nacer las cosas escondidas en un corte, y un satelite que crece un instante
        // en la toma siguiente delataria justo el "pop" que el corte tapo.
        if (st.prev >= 0 && lit > st.prev && !Cine().active) {
            st.newest = lit - 1;
            st.pulse = 0.0f;
        }
        st.prev = lit;
        st.pulse += m_dt;
        for (int j = 0; j < static_cast<int>(pieces.size()); ++j) {
            Piece& p = pieces[static_cast<std::size_t>(j)];
            const bool on = j < lit;
            p.node->active = on;
            if (!on) continue;
            const Slot s = FamilySlot(art, center, radius * band.base, band.spread, seed, shown, j, time);
            XMFLOAT3 pos;
            XMStoreFloat3(&pos, s.pos);
            p.node->transform.position[0] = pos.x;
            p.node->transform.position[1] = pos.y;
            p.node->transform.position[2] = pos.z;
            float sc = p.baseScale * grow;
            if (j == st.newest && st.pulse < kBuyPulse) sc *= 1.0f + 1.4f * std::sin(kPi * st.pulse / kBuyPulse);
            p.node->transform.scale[0] = p.node->transform.scale[1] = p.node->transform.scale[2] = sc;
            orient(s, p, j);
        }
    }

    // Gira `o` para que su eje local Z apunte a `dir` (como una camara). RollPitchYaw: el pitch positivo inclina el +Z hacia abajo.
    static void AimZ(GameObject& o, FXMVECTOR dir) {
        const XMVECTOR d = XMVector3Normalize(dir);
        const float x = XMVectorGetX(d), y = XMVectorGetY(d), z = XMVectorGetZ(d);
        o.transform.rotation[0] = -std::asin(std::clamp(y, -1.0f, 1.0f)) * 180.0f / kPi;
        o.transform.rotation[1] = std::atan2(x, z) * 180.0f / kPi;
        o.transform.rotation[2] = 0.0f;
    }

    // Lo mismo para el eje local Y (un cilindro o un disco: su cara mira hacia `dir`). Con la misma convencion, (0,1,0) -> (sin p sin w, cos p, sin p cos w).
    static void AimY(GameObject& o, FXMVECTOR dir) {
        const XMVECTOR d = XMVector3Normalize(dir);
        const float x = XMVectorGetX(d), y = XMVectorGetY(d), z = XMVectorGetZ(d);
        o.transform.rotation[0] = std::acos(std::clamp(y, -1.0f, 1.0f)) * 180.0f / kPi;
        o.transform.rotation[1] = std::atan2(x, z) * 180.0f / kPi;
        o.transform.rotation[2] = 0.0f;
    }

    static constexpr int kCrispRelays = 10;
    static constexpr float kRefDist = 10.5f; // la distancia de la toma por defecto de la Tierra (CameraRig::kEarthDist)

    GameObject* m_root = nullptr;
    GameObject* m_earth = nullptr;
    GameObject* m_sun = nullptr;
    std::vector<Piece> m_sats, m_mirrors, m_relays;
    std::vector<Beam3> m_beams, m_links;
    Beam3 m_veil{nullptr, nullptr};
    FamilyState m_satState, m_mirrorState, m_relayState;
    float m_dt = 0.0f;
    std::vector<GameObject*> m_pulses, m_climbers;
    GameObject* m_ring = nullptr;
    GameObject* m_station = nullptr;
    GameObject* m_wing = nullptr;
    GameObject* m_cable = nullptr;
    GameObject* m_weight = nullptr;
};

STRACE_REGISTER_SCRIPT(Fleet)
