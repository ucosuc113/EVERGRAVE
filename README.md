# EVERGRAVE (prototipo inicial)

Clicker de progresion tecnologica sobre S.T.R.AC.E. Precuela de AN0MALY. La
Tierra vista desde el espacio es el mundo; las instalaciones y megaestructuras
son hitos del arbol, no cosas que se vean construir.

Es una base funcional minima, no un juego terminado: el arte es de relleno.

## Que hay

- **Mundo**: Tierra (esfera de 9k triangulos, shader de dos colores), Sol, camara
  a 62 grados, la Tierra gira y orbita (el pivote gira con la camara; el fondo
  de estrellas deriva). Pixelado a 360 filas (`SubViewport`, 2x AN0MALY).
- **Clicker**: clic en la Tierra (o Espacio) da puntos. El Sol esta desde el
  principio pero **bloqueado**: el cursor dice `SUN - LOCKED` y un clic avisa que
  tecnologia hay que investigar. Solar Collectors lo abre. Ruby Laser abre un
  boton `Laser Pulse` en el HUD. Todo sale de datos, ver abajo.
- **Arbol** (tecla T): 10 tecnologias, requisitos, condiciones de desbloqueo
  (lo ganado en total), costos, efectos. Seleccionar, ver por que esta bloqueada,
  investigar.
- **Guardado**: F5 guarda, F9 carga (`%LOCALAPPDATA%\EVERGRAVE\save.txt`, solo
  Windows).

## El aspecto pixel-art (importado de AN0MALY)

Tres piezas, las mismas que AN0MALY: el `SubViewport` (360 filas, escala
entera), el post-proceso `Assets/Shaders/PostProcess.hlsl` (copia del de AN0MALY:
12 niveles por canal, dither de Bayer 8x8 calculado por texel de la imagen
interna, saturacion/contraste/negro) y `environment.postProcess` en la escena.
Los mandos estan en `Assets/Materials/PostProcess.strmat`; aqui el contraste es
1.15 (en AN0MALY 1.45 aplasta a negro el lado de noche de la Tierra).

**El recorte y el HUD**: con 360 filas la escala se redondea y, en ventanas como
un panel de 610 px (escala 2), la imagen se recorta ~55 px por lado. El HUD, el
panel del arbol y los avisos se anclan a `UiService::VisibleRect()` (el area que
siempre se ve entera, ver CLAUDE.md del motor), asi que no se pierden. El mundo si
se recorta; la Tierra queda centrada y lejos de los bordes. Resoluciones exactas
(sin recorte): 1280x720, 1920x1080, 2560x1440.

## Estructura (`Assets/Scripts/`)

```
Core/        el dominio, SIN motor: Resources, Tech, Interactions, GameState, SaveData
Data/        TechCatalog.generated.h (SALE DE LA HOJA, no se edita) y GameContent
Services/    Session (el GameState unico + estado de UI), Controls, UseFeedback
View/        Hud, TechPanel, SpaceWorld + TechLayout/Format/UiKit (sin motor, probables)
Direction/   GameRoot: el reloj de la partida
EvergraveCheck.cpp   pruebas que necesitan un frame
```

## La hoja de balance (`Design/EVERGRAVE_Balance.xlsx`)

Es la fuente de verdad del arbol y de las interacciones. Se edita en Excel y se
exporta a C++; el balance se disena en la hoja y no a ojo en el codigo.

| Hoja | Para que |
|---|---|
| Leeme | Como se usa, leyenda de colores, supuestos, que es canon y que es relleno |
| Resumen | Por era: tiempo total, espera media contra el objetivo, compras SLOW/FAST |
| Params | Costo inicial, salto entre eras, clics por segundo, umbrales |
| Eras | Las 8 etapas del brief: costo base, razon de costo por tier, objetivo por compra |
| Techs | Una fila por tecnologia (hasta 200): era, rama, tier, requisitos, condicion, costo manual o sugerido, hasta 3 efectos, estado, `Valido` |
| Interactions | Lo que el jugador puede hacer; `Desbloqueada por` y `Alcanzable` se calculan |
| Simulation | Una partida comprando en orden: cuanto espera cada compra y si se sale del objetivo de su era |
| Lists | Valores de los desplegables |

Azul = entrada, negro = formula. **Agregar una tecnologia**: una fila nueva en
Techs (los requisitos, SIEMPRE en filas de arriba), `Estado = Implemented`.
**Agregar una interaccion**: una fila en Interactions y, si hay que ganarsela, un
efecto `UnlockInteraction` en una tecnologia.

```bash
pip install openpyxl                      # una vez
python Tools/export_catalog.py            # hoja -> Assets/Scripts/Data/TechCatalog.generated.h
python Tools/export_catalog.py --check    # falla si el .h no coincide con la hoja
```

El exportador valida (id repetido, requisito que no esta arriba, efecto u objetivo
inexistente, interaccion que nunca se desbloquea) y no escribe nada si hay un
problema. Solo exporta las filas `Implemented`: una `Needs feature` (un efecto que
el juego aun no tiene) se guarda en la hoja sin romper nada.

La simulacion cuenta solo clics en la Tierra mas produccion pasiva (no el Sol ni
el pulso laser) y supone N clics por segundo (Params): es una hipotesis, no un
dato. Con las 10 tecnologias de hoy el arbol entero se completa en ~1.5 minutos y
casi todo sale FAST: ese es el desbalance que la hoja deja ver. El costo
"sugerido" y el "manual" difieren hasta 1000x en la era 2 por la misma razon.

## Pruebas

```bash
# dominio, layout y formato: sin motor ni ventana (133 comprobaciones)
g++ -std=c++20 -I. Tests/core_tests.cpp -o core_tests && ./core_tests
```

```powershell
# con frame (23): escena, pixelado, camara, picking, reloj, rotacion y orbita
$env:EVERGRAVE_CHECK = "1"
Editor.exe EVERGRAVE.strproj --play --play-seconds 5 --frames 4000 --game-size 1280x720
```

Se buscan `OK` / `FALLA` en `%LOCALAPPDATA%\STRACE\logs\Editor.log`. Sin
`EVERGRAVE_CHECK=1` el objeto `Pruebas` no hace nada (si no, reiniciaria la
partida de quien juega).

**Clics reales** con el Editor abierto: `strace play start <proyecto> --instances 2`
y `strace play click <proyecto> 2 <x> <y>` sobre la partida **2** (en la 1 el
Editor sobrescribe el cursor programado con el raton real). Las coordenadas son
pixeles de la imagen de la partida (1280x720 con `--game-size`), no internos.

## Limitaciones conocidas

- El arbol no se desplaza en horizontal: una fila de mas de ~3 nodos no cabe en
  pantallas angostas (el ancho interno depende del aspecto).
- Una flecha entre filas no consecutivas puede pasar por detras de un nodo
  (ver `View/TechLayout.h`).
- La fuente del motor es ASCII.
- Guardar/cargar solo en Windows.
- No hay produccion automatica mas alla de `PassiveAdd`; la arquitectura de
  interacciones la admite, pero no se construyo una economia idle.
- Progresion posterior (instalaciones industriales, escala solar, esfera de
  Dyson, estrella artificial, Proxima Centauri): no esta; entran como filas.
