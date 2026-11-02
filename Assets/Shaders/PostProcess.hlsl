// El aspecto de AN0MALY: cuantizacion de color + dither ordenado.
//
// Port de `Proyectos godot/AN0MALY/shaders/pixel_quantize.gdshader`. Es el pase
// final sobre la imagen entera del juego (ver `Engine/Rendering/PostProcess.h`),
// asi que le llega TODO: la escena, las pantallas, el HUD y las franjas de
// cine. Eso es a proposito - un HUD sin cuantizar sobre una escena cuantizada
// no se lee como dos capas, se lee como que una de las dos esta mal.
//
// TRES PERILLAS QUE CONTROLAN COSAS DISTINTAS, y conviene no confundirlas:
//
//   los NIVELES   -> cuantas bandas tiene un degradado
//   el DITHER     -> como se mezclan dos bandas vecinas
//   la SATURACION -> cuanto color tiene la imagen (nada que ver con lo anterior)
//
//   UserParams[0]  x niveles por canal (12)   y fuerza del dither (0,3)
//                  z patron de Bayer: 2, 4 u 8   w texeles por celda (1)
//   UserParams[1]  x saturacion (1,3)  y contraste (1,45)  z nivel de negro (0,02)
//
// La imagen de entrada llega en `AlbedoTex`, que es donde el pase la ata.
//
// EL DITHER SE CALCULA POR TEXEL DE LA IMAGEN, NO POR PIXEL DE PANTALLA
//
// Es la linea que hace que esto funcione, y la que es facil de perder. El juego
// se dibuja a 320x180 y se muestra escalado por un factor entero; si el patron
// de Bayer se evaluara en coordenadas de pantalla, se veria DENTRO de cada
// pixel gordo - o sea un pixel de pixel art con cuatro tonos adentro, que es lo
// contrario de lo que se busca. De ahi que la celda salga de la UV multiplicada
// por el tamano de la imagen.

// Bayer en forma cerrada por recursion: sin arrays ni indexado dinamico, que en
// un shader de pixel salen mas caros que la cuenta.
float Bayer2(float2 a) {
    a = floor(a);
    return frac(a.x * 0.5 + a.y * a.y * 0.75);
}

float Bayer4(float2 a) { return Bayer2(a * 0.5) * 0.25 + Bayer2(a); }
float Bayer8(float2 a) { return Bayer4(a * 0.5) * 0.25 + Bayer2(a); }

float4 PSMain(PSInput input) : SV_TARGET {
    const float levelsPerChannel = max(UserParams[0].x, 2.0);
    const float ditherStrength = UserParams[0].y;
    const float ditherPattern = UserParams[0].z;
    const float ditherScale = max(UserParams[0].w, 1.0);
    const float saturation = UserParams[1].x;
    const float contrast = UserParams[1].y;
    const float blackLevel = UserParams[1].z;

    float3 col = AlbedoTex.Sample(AlbedoSampler, input.uv).rgb;

    // El texel de la imagen de ENTRADA. `Time.zw` no trae el tamano y el motor
    // no pasa uno, asi que se saca de la propia textura - que es el unico sitio
    // donde el dato es siempre correcto, incluso si alguien cambia la
    // resolucion del panel Juego a mitad de partida.
    float texWidth, texHeight;
    AlbedoTex.GetDimensions(texWidth, texHeight);
    float2 texel = floor(input.uv * float2(texWidth, texHeight));
    float2 cell = floor(texel / ditherScale);

    float d;
    float levels;
    if (ditherPattern <= 2.0) {
        d = Bayer2(cell);
        levels = 4.0;
    } else if (ditherPattern <= 4.0) {
        d = Bayer4(cell);
        levels = 16.0;
    } else {
        d = Bayer8(cell);
        levels = 64.0;
    }
    // Centrado en cero: si no, el dither desplaza el brillo medio de la imagen
    // entera y el juego se ve mas claro de lo que se pinto.
    float threshold = d - (levels - 1.0) / (2.0 * levels);

    // El grado va ANTES de cuantizar, para repartir bien los pocos niveles que
    // quedan. Al reves, se ajustaria el contraste de una imagen que ya perdio
    // la informacion.
    float luma = dot(col, float3(0.299, 0.587, 0.114));
    col = lerp(float3(luma, luma, luma), col, saturation);
    col = (col - 0.5) * contrast + 0.5;
    col = lerp(float3(blackLevel, blackLevel, blackLevel), float3(1.0, 1.0, 1.0), col);
    col = saturate(col);

    float steps = levelsPerChannel - 1.0;
    col = floor(col * steps + 0.5 + threshold * ditherStrength) / steps;

    return float4(saturate(col), 1.0);
}
