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

## Estructura (`Assets/Scripts/`)

```
Core/        el dominio, SIN motor: Resources, Tech, Interactions, GameState, SaveData
Data/        TechCatalog (tecnologias) y GameContent (interacciones): solo datos
Services/    Session (el GameState unico + estado de UI), Controls, UseFeedback
View/        Hud, TechPanel, SpaceWorld + TechLayout/Format/UiKit (sin motor, probables)
Direction/   GameRoot: el reloj de la partida
EvergraveCheck.cpp   pruebas que necesitan un frame
```

**Agregar una tecnologia**: un `tree.Add({...})` en `Data/TechCatalog.h`.
**Agregar una interaccion**: una fila en `Data/GameContent.h`, y si hay que
ganarsela, un efecto `UnlockInteraction` en una tecnologia. El panel, el HUD y el
mundo no cambian. `GameState::ContentProblems()` delata un nombre mal escrito.

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
