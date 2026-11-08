# Plan: que todo lo que se compra se VEA

Estado (6/11/2026): **pasos 1 a 6 hechos; solo quedan detalles menores de la Tierra y el brillo del Sol**. Lo demas es plan.

**Ojo con el skyhook**: aparece en este documento como EJEMPLO del patron (seccion 4), para explicar como se
comporta una tecnologia, su mejora, su unidad y su version en otra era. NO esta en el arbol ni en la tienda
y no se agrega. Cada familia real sale de las tecnologias que ya existen (hoja Families).

- Hechas en `Design/EVERGRAVE_Balance.xlsx`: la hoja **Families** (65 familias), la hoja **Visuals**
  (las 261 tecnologias y las 24 unidades, cada una con su familia, su rol y lo que agrega o cambia)
  y la tienda rediseñada (6 unidades nuevas, ver 5.3).
- Paso 2 hecho en `View/WorldFx.cpp`: sprites, eclipse, pulsos, ascensor (ver la seccion 7).
- **Decisiones tomadas** (las de la seccion 8): se conserva el esqueleto economico (costos, tiers,
  requisitos y efectos de las 261 tecnologias no se movieron); **las Keys no cambian nunca** (las
  partidas y las pruebas las usan: una tecnologia renombrada conserva su Key vieja); las unidades
  nuevas entran como `Designed`, o sea en la hoja pero sin exportar, hasta tener su visual.

La meta: cada unidad de la tienda y cada tecnologia del arbol, de la era 1 a la 8,
tiene que ser algo que aparece en el mundo y que se reconoce sin leer el nombre. Al
principio cosas chicas (un mastil, un espejo de dos pixeles); al final cosas enormes
(un anillo alrededor de la Tierra, anillos de Dyson alrededor del Sol).

---

## 1. Lo que esta mal hoy (la captura de la Tierra)

- **Todo es el mismo punto.** Satelites, espejos, relays, minas: un cuadrado de 3 px
  con un pixel blanco en el centro. Lo unico que los distingue es el color, y el color
  no significa nada (rojo, naranja, azul, blanco al azar).
- **Las lineas de orbita y los rayos cruzan todo.** Con 25 o mas unidades se dibujan
  las orbitas enteras, y los rayos laser unen satelites al azar: una maraña de lineas
  delante del planeta que no dice nada.
- **Se amontonan en los nodos.** Todas las orbitas cruzan cerca del mismo punto
  (abajo a la derecha en la captura): ahi se juntan veinte puntos y parece ruido.
- **La superficie son cuadrados rojos sueltos.** Laser Array, Ground Station y
  Smart City son "pins" que no parecen ni ciudades ni laboratorios.
- **No hay escala.** Un satelite de 15 puntos y un espejo orbital de un millon se ven
  del mismo tamano.
- **Sin luz.** Los puntos tienen color plano: no tienen lado de dia y lado de noche,
  asi que no parecen estar en el mismo espacio que la Tierra.
- **El arbol no muestra nada.** Ninguna de las 261 tecnologias cambia el mundo.
- **La tienda muestra poco.** De las 18 unidades, varias no se ven nunca o se ven
  donde no van (en `WorldFx.cpp`, `Look = Sun` se dibuja alrededor del cuerpo que se
  mira, no del Sol, y cada unidad solo aparece en la vista de su cuerpo).

---

## 2. Principios

1. **Silueta antes que color.** Cada cosa tiene una forma que se reconoce aun en un
   solo color: un satelite tiene paneles, un espejo es un hexagono, un gancho es una
   barra que gira. Si hay que leer la etiqueta para saber que es, esta mal.
2. **Escalera de tamanos.** El tamano en pantalla sube con la era (tabla abajo). Lo
   de la era 1 es diminuto a proposito, para que lo de la 4 se sienta enorme.
3. **Tres familias de visual:**
   - **Unidad** (la tienda): se compra muchas veces. Se ve cada una, y la cantidad
     cambia la FORMA del grupo (una -> formacion -> anillo -> otra cosa).
   - **Estructura** (el arbol): aparece UNA vez y es grande. La crea una tecnologia.
   - **Mejora** (el arbol): cambia algo que ya existe. Es mas largo, brilla, gana
     una pieza, se mueve distinto, suma un modulo.
4. **Cada tecnologia responde cinco preguntas** (es la regla que escala a las 261):
   *que aparece, donde, como se mueve, como crece con las mejoras, con que se
   conecta*. Si una no tiene respuesta para "que aparece", es una **Mejora** de una
   estructura que ya existe.
5. **Movimiento con fisica real.** Kepler (lo de cerca gira mas rapido), puntos de
   Lagrange quietos respecto a dos cuerpos, statites que no orbitan, un cable que
   gira. El movimiento es parte de la silueta.
6. **Juntar en vez de multiplicar.** Pasado un umbral, cien satelites no son cien
   puntos: son una capa, un anillo, una estructura. Es lo que mantiene la imagen
   limpia y hace que comprar mas siga viendose (ver 4.3).
7. **Nada de lineas permanentes cruzando el planeta.** Las orbitas se insinuan con
   estelas cortas; un rayo es un pulso que aparece y se va, no un cable fijo.
8. **Luz.** Todo lo que esta en el espacio tiene lado iluminado (hacia el Sol) y lado
   oscuro. Lo de la superficie se ve de noche (luces) o al reflejar el Sol (destellos).

### Escalera de tamanos (pixeles de juego, con la camara por defecto de cada cuerpo)

| Era | Que es lo tipico | Tamano |
|---|---|---|
| 1 | mastiles, plantas, granjas (superficie) | 1-2 px, o una mancha de luz |
| 2 | laboratorios, haces al cielo, primeros satelites | 2-4 px; un haz de 20-60 px |
| 3 | ciudades, redes, reactores, puertos | manchas y redes de luz que cubren continentes |
| 4 | estaciones, espejos, ascensor, anillo orbital | 8-40 px; el anillo rodea la Tierra entera |
| 5 | flotas, asteroides traidos, primeras cosas en el Sol | 4-12 px cerca; puntos lejos del Sol |
| 6-7 | domos, minas, rieles, anillo lunar, trafico Tierra-Luna | 3-30 px sobre la Luna |
| 8 | colectores (chicos al principio), anillos de Dyson, la burbuja | de 2 px a anillos que llenan la pantalla |

---

## 3. Como se dibuja (sin tocar el motor)

Todo esto es del lado del juego. El motor ya da lo necesario:

- **Mallas 3D para lo grande** (estructuras de la era 4 en adelante):
  ascensor, anillo orbital, modulos de estacion, paneles de Dyson. Se generan con
  Python como ya se hace con las rocas (`Tools/gen_rock_gltf.py`) o se arman con
  primitivas. Van en la escena como **nodos ocultos con nombre** que una tecnologia
  enciende (igual que el cinturon: la escena son datos, el script anima). Un script
  tambien puede agregar hijos en runtime (`GameObject::children` es publico) para lo
  que se repite.
- **Sprites de pixel art para lo chico** (unidades de la tienda): dibujos ASCII en
  una cabecera, como los glifos de `PixelFont`, en 2-3 tamanos y con lado de luz y
  de sombra. Se dibujan con `AddRect` como hoy los puntos: cientos cuestan lo mismo.
- **Capas de superficie** en el shader de la Tierra y de la Luna (luces de ciudad,
  red electrica, granjas solares, rieles, rutas de barcos): mascaras que se encienden
  con parametros (`userParams`) en vez de pins sueltos.
- **Datos primero**: una hoja nueva **Visuals** en el Excel (ver 6), exportada a C++
  junto al catalogo. Asi el aspecto de cada cosa se decide en la hoja, como el balance.

Riesgo a vigilar: cientos de mallas 3D a la vez. Por eso las mallas son para
estructuras (pocas), y las unidades repetidas van como sprites o se juntan (4.3). Si
hiciera falta dibujado por instancias, seria un paquete del motor aparte, a proponer.

---

## 4. El patron, con el ejemplo del skyhook

> **Es solo un ejemplo.** El skyhook no existe en el arbol de mejoras ni en la tienda, y no se agrega. Esta
> seccion muestra COMO se comporta una tecnologia (se crea, se mejora, se compra, sigue en la era siguiente)
> para aplicarlo a las que si existen.

### 4.1 Que es de verdad

Un **skyhook rotatorio** (Hans Moravec, 1977; estudiado por Boeing como HASTOL en
2000) es un cable de cientos de kilometros que orbita la Tierra y GIRA sobre su
centro, como una rueda que rueda sobre la atmosfera. Cuando la punta pasa por abajo,
va hacia atras respecto a la orbita: su velocidad respecto al suelo baja mucho, y un
avion hipersonico que sube hasta ahi la alcanza y se engancha. Media vuelta despues
la punta esta arriba, yendo hacia adelante, y suelta la nave con la velocidad de la
orbita MAS la de la punta: la lanza a una orbita alta o hacia la Luna sin cohete.

El precio: cada nave que lanza le roba energia, y el gancho baja de orbita. Se
recupera con un **cable electrodinamico** (empuja contra el campo magnetico de la
Tierra con electricidad) o atrapando carga que VUELVE de la Luna. En la Luna, sin
aire, la punta puede tocar el suelo.

### 4.2 Como se ve, y como escala

| Paso | Que lo da | Que se ve |
|---|---|---|
| Aparece | tecnologia **Skyhook** (era 4) | una barra fina en orbita baja que gira lento; la punta roza la atmosfera una vez por vuelta |
| Funciona | (sin compra) | cada tanto sube de la superficie un avion (2 px con estela), la punta lo agarra abajo, lo lleva media vuelta y lo suelta arriba: sale disparado con una estela larga |
| Mejora | **Graphene Cable** | el cable es mas largo y mas fino; las naves salen mas lejos |
| Mejora | **Electrodynamic Reboost** | tras cada lanzamiento el gancho baja un poco y despues brilla azul y vuelve a subir |
| Unidad | tienda: **Skyhook** | cada compra suma un gancho en la misma orbita, repartidos; con 3 o mas se pasan naves de uno a otro |
| Mejora | **Skyhook Relay** | la nave soltada por uno la agarra otro mas alto: una cadena hacia la Luna |
| Era 6 | **Lunar Skyhook** | en la Luna la punta TOCA el suelo y levanta carga desde la superficie |

Lo que este ejemplo fija para todo el arbol:
- la tecnologia que la crea la pone en el mundo **una vez** y la deja funcionando;
- las mejoras **cambian lo que ya esta** (largo, color, ritmo, una pieza mas);
- la tienda **suma unidades** que se organizan entre si;
- y la familia **sigue en la era siguiente** con otra version (Luna, Sol).

### 4.3 Cantidad: de unidad a estructura

Cada unidad de la tienda tiene escalones visuales con los mismos hitos que ya dan
multiplicadores (10, 25, 50, 100, 200). Ejemplo, satelites:

| Tenes | Se ve |
|---|---|
| 1-9 | cada satelite, sprite con paneles, con su estela corta |
| 10 | se ordenan en una constelacion: un plano, espaciados parejos |
| 25 | segundo plano con otra inclinacion (nunca cruzan por el mismo nodo) |
| 50 | el sprite cambia a un satelite mas grande (antenas, paneles dobles) |
| 100 | los de atras del planeta se funden en una capa tenue, delante quedan 30 nitidos |
| 200 | una "cascara" de puntos finos y destellos: la Tierra con su propia atmosfera de metal |

Con esto el tope de dibujo (`Visible cap`) deja de ser "cuantos puntos" y pasa a ser
"cuantos nitidos + el resto juntado".

---

## 5. Inventario: cuanto se agrega

### 5.1 Familias visuales (las estructuras y sistemas del mundo)

Cada familia es una cosa del mundo que nace con una tecnologia y crece con otras.
Las 261 tecnologias se reparten entre ellas: cada una o **crea** una familia o
**mejora** una que ya existe.

| Era | Cuerpo | Familias (lo que se ve) |
|---|---|---|
| 1 | Tierra | Estaciones de medicion (mastiles con luz), Globos sonda, Plantas de vapor (penachos), Granjas solares (destellos), Parques eolicos (costas con parpadeo), Red electrica (lineas de luz de noche), Cohetes sonda (arcos cortos) |
| 2 | Tierra | Laboratorios laser, Haces al cielo, Estrella guia laser (el haz amarillo de los telescopios de verdad), Relays entre satelites (pulsos), Estaciones opticas en tierra, Observatorios |
| 3 | Tierra | Ciudades (luz que crece), Supergrid continental, Trenes y barcos (luces que se mueven), Reactores (domos con brillo), Tokamak (anillo azul), Megafabricas, Bases de lanzamiento |
| 4 | Tierra | Cohetes reusables (suben y vuelven), Estacion orbital modular (crece por modulos), Granja solar espacial + haz de microondas a una rectena, Espejos orbitales (hexagonos con reflejo en el mar), **Ascensor espacial** (cinta con escaladores), Lanzador magnetico, **Anillo orbital** (el final de la Tierra), primera base en la Luna |
| 5 | Tierra / sistema | Asteroide capturado en orbita, Flotas de velas, Tanqueros a los gigantes gaseosos (se van por el borde), Red interplanetaria (balizas en la linea Tierra-Sol), las pistas del enjambre (grupos lejanos, el statite, ver `Design`) |
| 6 | Luna | Domos (crecen a pueblo y a anillo de pueblos), Mineria de regolito (rovers + polvo), Fundiciones (brillo naranja), Hielo polar, Estaciones de Lagrange, Cinturon solar lunar |
| 7 | Luna | Lanzador lunar (riel 3D + disparos hacia el Sol), Ascensor Tierra-Luna, Autopista cislunar (trafico entre los dos), Exportacion de energia (haz a la Tierra) |
| 8 | Sol | Sondas solares, Colectores de Dyson (chicos y sueltos al principio, paneles enormes al final), **Anillo de Dyson**, Anillos inclinados, **Burbuja de Dyson** (statites quietos sobre los polos), cobertura (el Sol se puntea y se apaga), Elevamiento estelar (el penacho del polo), el haz hacia Proxima |

Total real, ya repartido en la hoja Families: **65 familias** (la lista de arriba era un resumen; el
detalle, con forma, tamaño, movimiento y ciencia de cada una, esta en la hoja).

### 5.2 Reparto de las 261 tecnologias (cuenta real de la hoja Visuals)

| Tipo | Cuantas | Que pide cada una |
|---|---|---|
| Crea una familia | 64 (la 65 es la de los satelites, que nace con su unidad) | un modelo o sprite nuevo + su movimiento |
| Mejora visible fuerte (pieza nueva, cambio de forma) | 123 | una variante del modelo o un escalon |
| Mejora visible leve (ritmo, brillo, color, densidad) | 74 | un parametro de la familia |
| Sin visual | 0 | — (la meta del pedido) |

**Una mejora que llega ANTES que su creadora** (son 14, la columna "Waits for creator" de Visuals:
por ejemplo Graphene Cable antes del ascensor, o Carbon Nanotubes antes del ascensor) no se pierde: se
guarda y la estructura nace ya mejorada cuando aparece.

### 5.3 La tienda nueva

**Hecho: 24 unidades** (las 18 de antes, con su "What you see" reescrito, mas 6 nuevas con
Status = `Designed`, en las eras 2, 3, 5, 6, 7 y 8). La era 1 se queda con sus 4. Cada una con su sprite o
malla y sus 6 escalones de cantidad (4.3). Las nuevas tienen costo y produccion INTERPOLADOS entre sus
vecinas de la tienda, no calibrados: antes de pasarlas a `Implemented` hay que repetir la calibracion
(sumar produccion en medio de una era cambia su ritmo).

Las 6 nuevas: Observatory Dome (2), Fission Reactor (3), Light Sail Fleet (5), Lunar Dome (6),
Cislunar Freighter (7), Statite (8). La tabla de abajo era el borrador previo:

| Era | Unidades (borrador) |
|---|---|
| 1 | Estacion de medicion, Planta de vapor, Granja solar |
| 2 | Satelite, Relay laser, Estacion optica |
| 3 | Ciudad inteligente, Reactor, Puerto de lanzamiento |
| 4 | Espejo orbital, Escalador del ascensor |
| 5 | Remolcador de asteroides, Vela solar, Tanquero de gas |
| 6 | Domo lunar, Rover minero, Fundicion |
| 7 | Lanzador lunar, Panel del cinturon lunar, Carguero cislunar |
| 8 | Sonda solar, Colector de Dyson, Statite |

### 5.4 Arte a producir (estimado)

| Tipo | Cuantos |
|---|---|
| Sprites de pixel art (2-3 tamanos cada uno) | ~35 |
| Mallas 3D (generadas o de primitivas) | ~20 |
| Capas de superficie en shader (Tierra y Luna) | ~8 |
| Efectos (estelas, pulsos, destellos, penachos) | ~10 tipos |

---

## 6. La hoja nueva: Visuals

Una fila por tecnologia y por unidad de la tienda. Columnas:

| Columna | Ejemplo (el skyhook, solo de muestra) |
|---|---|
| ID / Key | la de Techs o Installations |
| Familia | `skyhook` |
| Rol | Crea / Mejora fuerte / Mejora leve / Unidad |
| Cuerpo | Tierra |
| Capa | Malla 3D / Sprite / Superficie / Efecto |
| Forma | barra de 2 px que gira, con naves en la punta |
| Tamano (px) | 40 |
| Movimiento | orbita baja + giro propio, la punta baja al aire |
| Escalado | +1 gancho por compra; 3+ se pasan naves |
| Conecta con | Reusable Rockets (los aviones), Lunar Skyhook |
| Ciencia | Moravec 1977, HASTOL 2000 |

Las columnas "Around the Sun (what you see)" y "Real science" que ya tiene Techs se
mudan aca y se generalizan a todos los cuerpos.

---

## 7. Orden de trabajo propuesto

1. **HECHO. Hojas Families y Visuals + tienda nueva en el Excel.** Se conservo el esqueleto
   economico (cuantas tecnologias por era, tiers, costos, efectos), asi la partida
   calibrada no se movio. Pendiente de este paso: calibrar las 6 unidades `Designed`.
2. **HECHO (en `View/WorldFx.cpp`). Limpieza de lo de la captura**: sprites en vez de
   puntos (satelite chico y grande, espejo, relay, sonda, colector, domo, vela), sombra de
   eclipse, sin lineas de orbita, estelas, relays como PULSOS entre dos satelites, ascensor
   como cinta con escaladores, y los anillos "alrededor del Sol" dibujados alrededor del Sol.
   Pendiente de este paso: los escalones por cantidad (10/25/50/100/200) solo estan en el
   satelite (modelo grande a 50); el resto de las familias los traen en los pasos 3 a 6.
3. **HECHO. Rebanada piloto con una familia REAL: `rockets`** (Sounding Rockets 26, Steel Framework 22,
   Orbital Launch Sites 64, Reusable Rockets 71), en `View/WorldFx.cpp` (`DrawRockets`). Es la primera familia que sale
   del ARBOL y no de la tienda:
   - 26: una plataforma; un cohete sube en arco y cae. 22: la plataforma gana su torre de acero y una baliza roja.
   - 64: cuatro plataformas con calendario; el cohete llega a la orbita y se une a los satelites (se desvanece).
   - 71: la primera etapa se separa, vuelve y aterriza en su plataforma con un destello.
   Lo que valido para las otras 64 familias: (a) lo que las tecnologias ponen en el mundo se lee con
   `tree.IsResearched`, sin pasar por la tienda; (b) **las ramas sueltas funcionan solas**: la 64 no pide la 26, asi que
   cada tecnologia de la familia prende su parte aunque la "creadora" no este investigada (la regla de "espera a su
   creadora" solo vale cuando hay una cadena); (c) para capturar un momento de un efecto, `EVERGRAVE_FXTIME=<s>` congela el
   reloj de los efectos y `EVERGRAVE_CAMYAW` gira la camara. Tamaños: a la escala de la era 1 un cohete es un punto de
   3 px con una estela de 16; se lee, pero chico, que es lo que pide el plan.
4. **HECHO EN PARTE: Tierra, eras 1-4** (`View/EarthFx.h`, llamado desde `WorldFx.cpp`; las utilidades comunes
   se mudaron a `View/FxKit.h`). Lo que las tecnologias construyen, encendido por `tree.IsResearched`:
   - **La red** (22 nodos en espiral dorada y las 38 aristas mas cortas): red electrica (nace local con Power Grid, se
     vuelve cobre, ganan pulsos con los condensadores y el balanceo, una columna vertebral con alta tension, azul-blanca con
     superconductores, y cruza mares con la red continental), transporte (trenes, barcos, trenes rapidos), industria
     (brillo ambar, hornos, bloques, nubes, cinturones), reactores y anillos de fusion, parques eolicos, laboratorios
     laser (de un haz rojo timido a rayas blancas hasta la orbita) y observatorios.
   - **La orbita:** remolcadores (3, luego 9), la estacion modular (crece de un nucleo a un complejo), el panel solar
     espacial con su pulso y su rectena, el anillo orbital, y el lanzador magnetico en tierra.
   - **El sistema visto desde la Tierra** (eras 4 y 5, tambien en `EarthFx.h`): balizas en los puntos de Lagrange con pulsos
     (al azar, o en un solo giro con la IA del sistema) y compuertas que lanzan carga; una antena de espacio profundo; la botella
     de antimateria; un astillero que se duplica con su escoria, su nave en construccion y sus lanzaderas; la sonda que va al
     cinturon, la roca capturada con sus sondas, su red y su refineria; el gigante gaseoso con el petrolero, el cometa, las
     fabricas flotantes y la estacion; el telescopio con su barrido; la linea de energia hacia el Sol; y los destellos de los
     espejos en el mar con la nave empujada por su haz.
   - **Pendiente de la Tierra:** los drones de las estaciones meteorologicas (17, 18), las mejoras de las plantas de vapor
     (12, 13, 15, 23), los seguidores solares (11, 20), las mejoras de ciudad (44, 47, 57, 63, 67) y las del ascensor (73, 87).
5. **HECHO EN PARTE: Luna, eras 6-7** (`View/MoonFx.h`; la red de nodos que comparte con la Tierra se mudo a
   `BodyNet`, en `View/FxKit.h`). Mapeo (orbitador con barrido, parches, red sismica, rejilla simulada), alunizajes y
   rovers, minas (hoyos, polvo, panal, hielo polar), regolito (ladrillos, pista, montones), hornos (de naranja a
   blanco, robots, filas bajo domos, federacion), domos y pueblos (anillo, megabase, brillo nocturno), red electrica
   dorada (con pulsos y luego toda la noche iluminada), riel, reactores y anillo de fusion, refinado, energia solar
   (parches, haces, cinturon ecuatorial, anillos), observatorios, lanzador de masa con la carga rumbo al Sol, laseres,
   estaciones alrededor de la Luna, el trafico Tierra-Luna (transbordadores, cable, haz de energia) y la linea de
   suministro del enjambre.
   - **Pendiente de la Luna:** lo que solo se ve desde la Tierra (la base y la estacion junto a la Luna, 96 y 141, y el
     punto L1, 166); el ascensor y los transbordadores dependen de que la Tierra este en cuadro; "Lunar Master Plan"
     (182), "Self-Sealing Habitats" (175) y "Base Autonomy" (216) no tienen cambio propio.
6. **HECHO EN PARTE: Sol, eras 5 a 8** (`View/SunFx.h`). El Sol no tiene suelo: lo que lo rodea son ORBITAS (con Kepler:
   lo de dentro gira mas rapido). Se dibuja en las tres vistas donde se ve (Tierra, Luna y la suya); en la suya los radios se
   comprimen (4 pasa a 2,3) porque el Sol llena la pantalla.
   - **Era 5, las pistas:** tres puntos de 2 px lejos del Sol, otro grupo con otra inclinacion, grupos chicos en 3 o 4
     orbitas, Mercurio con su sonda y chispas de metal, Venus con luces, el statite quieto sobre el polo, y el fantasma
     punteado del anillo futuro. Desde la Tierra se ve mirando hacia el Sol desde el lado nocturno (`EVERGRAVE_CAMYAW=180`
     y `EVERGRAVE_CAMPITCH=15`): el Sol esta a 150 unidades y detras del planeta.
   - **Eras 6 y 7:** el enjambre se vuelve 3D (planos repartidos) y nace el anillo interior; la linea de suministro desde la
     Luna ya se dibuja en la vista de la Luna.
   - **Era 8:** el grupo interior rapido, radiadores rojizos, la nube de 60 colectores, el anillo de Dyson solido con pulsos y
     dos anillos mas inclinados, la burbuja de statites en los dos polos, sondas en espiral (de 3 a 40, por carriles, que
     resisten mas cerca), la corona (cuenta, bucle del viento, hub), forjas, espejos, balizas y carriles, el penacho del
     levantamiento estelar, el haz a Proxima y, al final, el disco punteado con un halo naranja.
   - **Del sistema visto desde la Tierra** (era 5): naves de transito (con llama azul y estelas largas) y velas solares con
     el haz del suelo.
   - **Pendiente del Sol:** el Sol no se apaga de verdad: la cobertura es una trama oscura encima, no un cambio de brillo
     del shader. (Las familias de la era 5 que no eran del Sol ya estan, ver el paso 4.)
7. **Mirarlo todo** con capturas de cada era (`--shot-game` con una partida avanzada,
   como `Tests/maxed.save`).

---

## 8. Decisiones abiertas

- **Partidas guardadas.** Las partidas guardan la Key de cada tecnologia y unidad.
  Cambiar todo con Keys nuevas deja las partidas viejas sin lo comprado (el juego
  ignora las Keys que no conoce, no se rompe). O se conservan las Keys donde el tema
  se parezca. Recomendado: Keys nuevas y empezar de cero, si nadie tiene partidas que
  importen.
- **Conservar el esqueleto economico** (recomendado) o rediseñar tambien numeros.
- **Que se ve del Sol desde la Tierra** en la era 5: hoy el Sol de esa vista esta
  lejos y chico. Para que se vean los grupos lejanos haria falta el mismo truco de
  perspectiva del cinturon (acercarlos a la camara).
- **3 unidades por era**, o mas para las eras largas (5 a 8 tienen 40 tecnologias).

---

## 9. La mirada de cine (6/11/2026): bloom, ajuste de color y enfoque

El problema de la captura de la Tierra no era de contenido sino de PLANOS: todo se dibujaba nitido, del mismo tamano y con
colores a tope, y encima de la imagen. Los efectos del mundo son UI, y la UI va despues del bloom, del ajuste de color y del
enfoque del motor: no les llegaba nada de eso, asi que 200 satelites se leian como UNA calcomania.

Lo que se hizo (todo del lado del juego, sin tocar el motor):

- **El post** (`Assets/Shaders/PostProcess.hlsl`) hace ahora, antes de cuantizar: enfoque (profundidad de campo con
  `StraceSceneDepth`), brillo (24 muestras, umbral) y vineta. Mandos en `PostProcess.strmat` (comentados en el .hlsl).
  **El brillo esta en el post y no en el `SubViewport.bloom`** a proposito: el bloom del motor limpia la profundidad y el
  motor apaga el enfoque (`FrameComposer`: `depthSurvives`). Las dos cosas solo caben juntas aqui (como en The Freak Place).
- **`uiOverPost: true`** en la escena: el HUD y los efectos se dibujan DESPUES del post. Con la UI debajo, el enfoque
  desenfocaria el HUD segun lo que hay detras. Consecuencia: el HUD ya no pasa por el dither ni por el +20% de saturacion
  (se subio un poco `kit::Red`/`RedDark` para compensar).
- **Ajuste de color** en `Space/OrbitPivot/Camara` (`camera.grade`): sombras hacia verde azulado, luces hacia calido,
  saturacion 0,9, luces -0,12.
- **El foco sigue al cuerpo** (`CameraRig::UpdateFocus`): la distancia de foco es la superficie cercana del cuerpo que se
  mira y la apertura se suaviza (0 en el mapa). Aperture 0,6 (menu 0,7, Sol 0,35).
- **Los efectos hacen su propio brillo y enfoque** (`FxKit.h`): `Halo` (bloom con discos tenues), `Bokeh` (punto de luz fuera
  de foco), `Beam` (haz con nucleo y estela), `DepthCue` (donde esta algo respecto al plano de enfoque: delante = disco grande,
  en el plano = nitido, detras = punto frio y apagado). La paleta pasa por `Tame` (22% menos saturada).
- **Los satelites, relays y espejos tienen CAPA** (`WorldFx::BandOf`): satelites pegados al planeta, relays encima, espejos
  arriba; 40 satelites nitidos y el resto polvo; brillo por fase (lado de dia / de noche) ademas del eclipse. El relay es el
  acento rojo (baliza con brillo).
- Arreglo de paso: `MainMenu` dejaba el titulo pintado sobre el juego cuando el estado nacia con el menu cerrado (corridas
  con `EVERGRAVE_SAVE`).

- **Profundidad en lo que dibuja EarthFx, MoonFx y SunFx** (`g_mod` en `FxKit.h`): quien dibuja dice una vez "esto esta a tal
  profundidad" (`Depth(vz)`, `AirPt`, `SunFx::P`) y todo color que pasa por `Tame` se enfria y se apaga hasta el aviso siguiente.
  Estacion, panel solar, anillo (cada punto: lo de adelante brilla, lo de atras se apaga), balizas y sistema, estaciones de la
  Luna y el enjambre del Sol (solo en la vista del Sol). Los remolcadores, que son chicos y numerosos, ademas pasan a `Bokeh`
  cuando estan delante del foco. `EarthFx::Run` resetea `g_mod` entre familias.

### 9.1 Plan "cinematografico de verdad" (fases 0-2 hechas, 6/11/2026)

Plan completo en `~/.claude/plans/idempotent-jingling-donut.md` (referencia: ciencia ficcion dura; FX importantes a la escena 3D;
agrupar en estructuras). Hecho: **Fase 0** (seis tomas canonicas), **Fase 1** (composicion: Tierra a 10,5 en vez de 16, encuadre en el
area libre a la derecha de la tienda con `CameraRig::FramingPan`, yaw 72 / pitch 18 por defecto en `Session.h`, el giro automatico
se cambio por un balanceo de +-9 grados cada 70 s) y **Fase 2** (grade, vineta y brillo por vista en `CameraRig::LookOf`, atmosfera
de la Tierra mas presente). Antes/despues: `docs/antes_despues_f1f2.png`. Falta: 3 (flota y brillos en 3D), 4, 5, 6, 7.

**Fase 3 (3a y 3b hechas)**: los satelites (40), espejos (24) y relays (10 de 30) de la Tierra son OBJETOS de la escena (`Fleet`,
generado por `Tools/gen_fleet.py`, animado por `View/Fleet.cpp` con el mismo `OrbitSlot`/`BandOf` y el reloj compartido `Services/FxClock.h`
que WorldFx). Luz del Sol, desenfoque, brillo y grade reales. `Shaders/Glow.hlsl` (esfera aditiva) hace las luces de los satelites y
la baliza de los relays; los espejos destellan con el especular del material. WorldFx solo dibuja el polvo de satelites y los pulsos de
relay. Pendiente de la fase 3: 3c (lasers, anillo, ascensor y estacion como mallas) y el eclipse. Antes/despues: `docs/antes_despues_f3.png`.

**Fase 3 completa (3c, 7/11/2026)**: tambien son de la escena los laseres y haces (12 cilindros aditivos con su propio material,
`Services/FxBus.h`: EarthFx/WorldFx piden "un haz de A a B" y `Fleet` lo dibuja; la logica de cuando dispara sigue en EarthFx), el anillo
orbital (`Assets/Meshes/ring.strmesh`, generado por `Tools/gen_ring_gltf.py`: la primitiva Torus tiene el tubo a 43% del radio y no sirve), la
estacion modular, el panel solar espacial y el ascensor. Con `FxBus().fleetInScene` WorldFx/EarthFx dejan de dibujar esas piezas como UI.
Medido (Editor de Debug, `maxed.save`, `--stats`): 10,7 ms por frame (94 FPS), la escena 1,7 ms. **Para capturar con otro Editor abierto**
en el mismo proyecto no se puede compilar (`Scripts.dll` en uso): se copia el proyecto a otra carpeta y se corre ahi.

**Fase 4 (agrupar en estructuras, 7/11/2026)**: `FxKit.h` tiene la UNICA cuenta de las formaciones (`FamilySlot`): satelites en TRENES de 10 (un hueco fijo
entre uno y otro, cada tren en su plano; uno nuevo se suma a la cola y los demas no se mueven), espejos en ARCOS de 12 en una orbita alta, relays
sin formacion pero con ENLACES (`Fleet::UpdateLinks`: un haz fino entre dos vecinos que se enciende el 30% del ciclo; sin lineas fijas). Lo que pasa
de los 40 satelites nitidos ya no es polvo de UI: es el VELO (`Shaders/Veil.hlsl`, una esfera aditiva con puntos de 1 grado sobre tres planos
orbitales que gira; densidad segun lo COMPRADO, de 40 a ~260). Lo ultimo comprado crece un instante (`kBuyPulse`). WorldFx ya no dibuja polvo si hay Fleet.
Antes/despues: `docs/antes_despues_f4.png`.

**Fase 5 (limpiar el confeti de UI, 7/11/2026)**: regla en `FxKit::Dot`: un punto de 1 px con menos de `kMinSpeck` (0,22) de opacidad no se dibuja. La red
electrica y los cinturones de la industria son DE NOCHE (peso de la noche al cuadrado; de dia no hay lineas: era una jaula de alambre sobre el
mar), de 1 px, y cada nodo de la red brilla con su `Halo`. `DrawSystemGrid` (la linea de puntos al Sol) es un rayo que se desvanece con la distancia.
Las balizas de Lagrange son un sprite con halo. Las naves que se van (`DrawFar`) son 8 como mucho (10 las velas). Antes/despues: `docs/antes_despues_f5.png`.
No se hizo: la paleta por rol (dorado/cian/rojo) como regla dura en `ColorOf`; sigue siendo `Tame` (menos saturacion) y los colores de la hoja.

**Fase 6 (Luna y Sol, 7/11/2026)**: **Sol**: nueva raiz `SunWorks` (`View/SunWorks.cpp`, generada por `Tools/gen_fleet.py`) con el MODELO DEL ARTISTA `Assets/Meshes/dyson ring.fbx`
(importado con `MeshTool.exe`, queda `dyson ring.strmesh` al lado; un disco en el plano XY de radio ~1,03 y +-0,13 de grosor, sin material) para el anillo
solido y los dos de `solar_assembly`; el fantasma (`megastructure_design`) usa la malla fina `ring.strmesh`. Cada anillo es un parent vacio (SunWorks lo
escala/inclina/gira) con un hijo `Model` acostado (X 90). Su piel es **`Shaders/DysonPanel.hlsl`** + `Materials/DysonPanel.strmat` (paneles con juntas, la cara
que mira al Sol arde, exterior de metal oscuro, pulso de carga que le da la vuelta; mandos en `userParams`, comentados en el .hlsl). Los 60 colectores
son cubos emisivos de la escena. SunFx deja de dibujar el anillo y la nube si SunWorks existe (`FxBus().sunWorksInScene`). **Luna**: misma toma que la Tierra (yaw
respecto al Sol, 72/18, `Session.h`/`CameraRig.cpp`; antes la fase dependia del momento del mes en que se entraba), a 3,6 de distancia (el 40% del alto), red electrica
solo de noche. Antes/despues: `docs/antes_despues_f6.png`. Pendiente: lo demas de SunFx (statites, sondas, forja, corona) y de MoonFx sigue siendo UI.

**Fase 7 (pulido y medicion, 7/11/2026)**: las seis tomas canonicas contra las de antes (la Tierra de dia, de noche, con todo comprado, la Luna, el Sol y el menu)
mejoran en todas; el menu no cambia a proposito. Medido en el Editor de Debug con `maxed.save` y `--stats`: Tierra 9,6 ms por frame (104 FPS, la escena 1,6 ms), Luna 5,9 ms
(170 FPS), Sol 7,1 ms (142 FPS). Suite: 28 OK, 0 FALLA; `strace validate` ok. **Conocido y sin hacer**: el eclipse de los objetos (la luz del Sol no tiene
sombra del planeta sobre ellos), la paleta por rol como regla dura, y el resto de SunFx/MoonFx (statites, sondas, forja, corona, minas...) que siguen en UI.
Para tocar el aspecto sin recompilar: `PostProcess.strmat` (niveles, saturacion, brillo, vineta, enfoque), `CameraRig::LookOf` (grade por vista) y los materiales
`Glow*`, `Veil`, `DysonPanel`, `Mirror`, `RingMetal`. Para regenerar las piezas de la escena: `python Tools/gen_fleet.py`.

**Colector de Dyson del artista (7/11/2026)**: `Assets/Meshes/dyson collector.fbx` (importado con MeshTool: 96 triangulos, dos submallas `Material.001` gris y `Material.002` dorada; disco de radio 1 en
XY, 0,49 de alto hacia +Z, origen en la base) reemplaza a los 60 cubos de `SunWorks`. Cada colector es un parent (SunWorks lo escala, apunta su +Z al Sol y lo hace rodar) con un hijo
`Model` y un material de shader por submalla: `CollectorPanel` y `CollectorGold`, ambos con `Shaders/DysonCollector.hlsl` (cada cara se ilumina por cuanto mira al Sol; SunWorks le pasa la
posicion del Sol en `userParams[1]`). Si la cara que debe arder es la contraria, se invierte el apuntado en `SunWorks.cpp` (`AimZ(*c, m_center - pos)` -> `pos - m_center`).

**Anillos de Dyson que se mueven (7/11/2026)**: los tres anillos de `SunWorks` ya no son estaticos: cada uno gira sobre su propio eje (`spin`, un nodo `Spin` entre el parent y el `Model`), su plano
precesa alrededor del Sol (`nodeRate`) y se inclina y desinclina (`wobble`), a ritmos y sentidos distintos (el de dentro, mas rapido y al reves; el de fuera, el mas inclinado). Son
vueltas de minutos. Estan en capas (radios 1,9 / 2,4 / 2,95 radios solares) y la banda del modelo va un 40% mas delgada (`Model` con escala Z 0,6) para que no se atraviesen al
inclinarse. Los pulsos de carga corren por el anillo solido siguiendo su plano de cada instante. Mandos en `kSolid`/`kRingA`/`kRingB`/`kGhost` de `SunWorks.cpp`. Imagen: `docs/anillos_dyson_en_el_tiempo.png` (t = 0, 45 y 90 s).
