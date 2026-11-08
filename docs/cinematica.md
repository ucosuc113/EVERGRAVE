# La cinematica (regalo): Tierra, Luna y Sol con "You Are My Destiny"

Una animacion de 62,68 s hecha con el propio juego, grabada por el motor (no con una captura de pantalla).

## Repetirla

```powershell
.\Tools\cine_render.ps1 -Fps 30 -Name preview                    # prueba (unos minutos)
.\Tools\cine_render.ps1 -Fps 60 -Name final -Text "PARA TI"      # la buena
```

Deja `Build\Cinematica\<Name>.mp4` (y los PNG en `<Name>-frames`). Todo es repetible: el Editor avanza el juego EXACTAMENTE
`1/Fps` por frame (`--record`), asi que no depende de lo rapida que sea la maquina. La dedicatoria es ASCII (la fuente del motor
no tiene acentos ni enie).

## Como esta hecha

- **`Services/Cine.h`** (sin motor): el guion y la camara son UNA FUNCION DEL TIEMPO, `CineEvaluate(t)`. Cada toma es una clave de
  principio y otra de fin (distancia, giro, altura, FOV, enfoque, fase de la Luna, a cuanto se ve el Sol); la toma 7 lleva una clave
  intermedia que cae en el golpe de la musica. Los cuerpos (`CineWorldOf`) se calculan con las mismas formulas de `SpaceWorld`.
- Ganchos, todos con `Cine().active` (sin `EVERGRAVE_CINE` el juego es el de siempre): `GameRoot` (arranque, reloj, 540 filas),
  `CameraRig` (`DriveCine`), `SpaceWorld` (angulos, Sol, barras, texto, negro), `Fleet` (el hilo de luz), `WorldFx` (reloj de efectos),
  y HUD y paneles se ocultan con `Ui().cine`.
- **540 filas internas** (360 x 1,5): el SubViewport se sube en la copia de Play, y 540 x 2 = 1080p exacto (un pixel de juego = 2x2).
- **Encuadre alterno**: 16:9 e IMAX 1,43:1 (barras negras a los lados, enteras, que se deslizan en el segundo despues del corte,
  con un filo tenue mientras se mueven porque contra el espacio una barra negra no se veria).
- **Partida curada**: `Tests/cine.save` (local, fuera de git): abre el mapa, la Luna y el Sol y trae lo hermoso (satelites, espejos,
  anillo orbital, anillo de colectores del Sol), sin laseres rojos ni rejilla electrica. `maxed.save` tapa todo.

## La musica

`Paul Anka - You are my destiny (Lyrics).mp3`, 2:48, de volumen casi plano. Compas medido: 3,43 s (~70 pulsos), primer compas en el
segundo 0,94. A los **62,68 s** la cancion cambia de seccion (el ataque pasa de ~0,5 a 4-8): ahi se corta, con 1,2 s de entrada y 4,6 s
de salida (qsin). Cada corte de escena cae en una linea de compas; el eclipse cubre el Sol en el golpe del segundo 45,53.

| # | Segundos | Formato | Plano |
|---|---|---|---|
| 1 | 0 - 7,80 | 16:9 | El amanecer: el Sol asoma por el borde de la Tierra |
| 2 | 7,80 - 14,66 | IMAX | La Tierra de pie, del polo al ecuador, lado de noche |
| 3 | 14,66 - 21,52 | 16:9 | El entorno: satelites y la Luna |
| 4 | 21,52 - 28,38 | IMAX | La Luna de cerca con la Tierra detras |
| 5 | 28,38 - 35,24 | 16:9 | El hilo de luz de la Tierra a la Luna |
| 6 | 35,24 - 42,10 | IMAX | El Sol y su cinturon de asteroides |
| 7 | 42,10 - 52,39 | 16:9 | El eclipse |
| 8 | 52,39 - 62,68 | IMAX | La Tierra y la Luna quietas, la dedicatoria, y el negro |

## Mirar una escena sola

`EVERGRAVE_CINE=<segundo>` arranca ahi y `EVERGRAVE_CINE_FREEZE=1` congela el tiempo:

```powershell
$env:EVERGRAVE_CINE="45.6"; $env:EVERGRAVE_CINE_FREEZE="1"; $env:EVERGRAVE_SAVE="Tests\cine.save"
..\..\build\bin\Debug\Editor.exe EVERGRAVE.strproj --play --frames 120 --shot-game x.png --game-size 1920x1080
```

## Lo que costo aprender

- **El Sol del eclipse no puede estar "en su sitio"**: la camara esta sobre el plano y la Luna en el, asi que la paralaje los separa
  ~4 grados en pantalla. `sunAlign` pone el Sol sobre el rayo camara -> Luna, y `sunSlide` lo corre de lado: asi la Luna lo cruza.
  Hay que escribir tambien su ALTURA (no solo x y z) en `SpaceWorld`.
- `far` es una macro de `windows.h`: no se puede llamar asi a una variable en un `.h` de scripts.
