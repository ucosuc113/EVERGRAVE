// El aspecto de EVERGRAVE: pixel art con cara de cine. Cuatro cosas en un solo pase, EN ESTE ORDEN:
//
//   1. el ENFOQUE      (profundidad de campo: lo lejano y lo cercano al foco se desenfocan)
//   2. el BRILLO       (lo que pasa el umbral se derrama alrededor: bloom ancho)
//   3. la VINETA       (los bordes del cuadro caen hacia el negro, el ojo va al centro)
//   4. la CUANTIZACION (color a N niveles con dither de Bayer: el pixel art)
//
// Antes solo estaba la cuantizacion (port de `AN0MALY/shaders/pixel_quantize.gdshader`). El enfoque y el brillo
// van ANTES de cuantizar a proposito: un desenfoque de una imagen que ya perdio sus matices da bandas, y el
// dither los reparte despues como pixeles, que es lo que se busca.
//
// POR QUE EL BRILLO ESTA AQUI Y NO EN EL SUBVIEWPORT. El bloom del motor (`SubViewport::bloom`) limpia la
// profundidad de la escena, y sin profundidad el motor apaga el enfoque (FrameComposer: `depthSurvives`).
// Las dos cosas a la vez solo caben en el post, como en The Freak Place.
//
// ESTE PASE NO PUEDE ALCANZAR LA INTERFAZ: la escena tiene `uiOverPost` encendido, asi que el HUD y los efectos
// del mundo (satelites, luces, haces: son UI) se dibujan DESPUES. Con la UI debajo, el enfoque la desenfocaria
// segun lo que hay detras de ella. Los efectos del mundo hacen su propio enfoque y su propio brillo
// (View/FxKit.h: `Bokeh`, `Halo`, `DepthCue`), con la misma idea que este shader.
//
// TRES PERILLAS DE LA CUANTIZACION, que controlan cosas distintas:
//
//   los NIVELES   -> cuantas bandas tiene un degradado
//   el DITHER     -> como se mezclan dos bandas vecinas
//   la SATURACION -> cuanto color tiene la imagen (nada que ver con lo anterior)
//
//   UserParams[0]  x niveles por canal   y fuerza del dither   z patron de Bayer: 2, 4 u 8   w texeles por celda
//   UserParams[1]  x saturacion   y contraste   z nivel de negro   w fuerza de la vineta (0 = ninguna)
//   UserParams[2]  x escala del enfoque (1 = el que da la apertura de la camara)
//                  y cuanto se desenfoca el CIELO (las estrellas) respecto a lo demas, 0..1
//                  z intensidad del brillo (0 = apagado)   w umbral del brillo
//   UserParams[3]  x radio del brillo, en pixeles a 1080p   y tinte del brillo hacia el azul (0..1)
//
// El enfoque lo manda la CAMARA (apertura, distancia de foco): `CameraRig` mueve el foco al cuerpo que se mira.
//
// LA CUANTIZACION SE CALCULA POR TEXEL DE LA IMAGEN, NO POR PIXEL DE PANTALLA. El juego se dibuja a 360 filas y se
// muestra escalado por un factor entero; si el patron de Bayer se evaluara en coordenadas de pantalla, se veria
// DENTRO de cada pixel gordo. De ahi que la celda salga de la UV multiplicada por el tamano de la imagen.

// Bayer en forma cerrada por recursion: sin arrays ni indexado dinamico, que en
// un shader de pixel salen mas caros que la cuenta.
float Bayer2(float2 a) {
    a = floor(a);
    return frac(a.x * 0.5 + a.y * a.y * 0.75);
}

float Bayer4(float2 a) { return Bayer2(a * 0.5) * 0.25 + Bayer2(a); }
float Bayer8(float2 a) { return Bayer4(a * 0.5) * 0.25 + Bayer2(a); }

float3 Fetch(float2 uv) { return AlbedoTex.Sample(AlbedoSampler, uv).rgb; }

// ------------------------------------------------------------------ el enfoque
// El tamano del desenfoque (en pixeles a 1080p) crece con lo lejos que esta un punto del foco RELATIVO a su
// distancia, que es como desenfoca un lente. El cielo (las estrellas, a 80 unidades) se trata aparte: desenfocarlo
// del todo las apagaria, y un campo de estrellas nitido detras de lo borroso es parte de la imagen.
float FocusBlur(float d, float scale, float sky) {
    float f = max(CameraFocus.z, 0.05);
    float c = CameraFocus.w * 14.0 * scale * saturate(abs(d - f) / max(d, 0.01) * 2.0);
    return d > 60.0 ? c * sky : c;
}

// Un disco de 16 lecturas (espiral de angulo aureo). Cada muestra aporta solo si SU propio desenfoque llega hasta
// aqui (una cosa nitida no se desparrama), y lo que esta DETRAS de este pixel no puede desenfocarse mas que el:
// asi el fondo borroso no se come el borde de una figura enfocada delante.
float3 DepthOfField(float2 uv, float3 col, float2 texel, float scale, float sky) {
    float dc = StraceSceneDepth(uv);
    if (dc <= 0.0) return col;
    float cc = FocusBlur(dc, scale, sky);
    if (cc < 0.45) return col;
    float3 acc = col;
    float wsum = 1.0;
    [unroll] for (int i = 0; i < 16; i++) {
        float fi = (float)i + 0.5;
        float r = sqrt(fi / 16.0) * cc;
        float a = fi * 2.39996323;
        float2 o = float2(cos(a), sin(a)) * r * texel;
        float ds = StraceSceneDepth(uv + o);
        float cs = FocusBlur(ds, scale, sky);
        if (ds > dc) cs = min(cs, cc);
        float w = saturate(cs - r + 1.0);
        acc += Fetch(uv + o) * w;
        wsum += w;
    }
    return acc / wsum;
}

float4 PSMain(PSInput input) : SV_TARGET {
    const float levelsPerChannel = max(UserParams[0].x, 2.0);
    const float ditherStrength = UserParams[0].y;
    const float ditherPattern = UserParams[0].z;
    const float ditherScale = max(UserParams[0].w, 1.0);
    const float saturation = UserParams[1].x;
    const float contrast = UserParams[1].y;
    const float blackLevel = UserParams[1].z;
    const float vignette = UserParams[1].w;

    float3 col = Fetch(input.uv);

    // El texel de la imagen de ENTRADA. `Time.zw` no trae el tamano y el motor
    // no pasa uno, asi que se saca de la propia textura - que es el unico sitio
    // donde el dato es siempre correcto, incluso si alguien cambia la
    // resolucion del panel Juego a mitad de partida.
    float texWidth, texHeight;
    AlbedoTex.GetDimensions(texWidth, texHeight);
    const float2 texelSize = 1.0 / float2(texWidth, texHeight);
    const float scale = texHeight / 1080.0;
    float2 texel = floor(input.uv * float2(texWidth, texHeight));
    float2 cell = floor(texel / ditherScale);

    // 1. EL ENFOQUE.
    if (CameraFocus.w > 0.001) col = DepthOfField(input.uv, col, texelSize, scale * UserParams[2].x, UserParams[2].y);

    // 2. EL BRILLO: 24 muestras en espiral (angulo aureo) dentro de un radio ancho, solo de lo que pasa el umbral y
    // con peso que cae hacia afuera. Una espiral no deja los anillos que dejan dos circulos fijos de muestras.
    const float glow = UserParams[2].z;
    if (glow > 0.001) {
        const float threshold = UserParams[2].w;
        const float radius = max(UserParams[3].x, 1.0) * scale;
        float3 acc = 0;
        float wsum = 0;
        [unroll] for (int i = 0; i < 24; i++) {
            float fi = (float)i + 0.5;
            float r = sqrt(fi / 24.0);
            float a = fi * 2.39996323;
            float2 o = float2(cos(a), sin(a)) * r * radius * texelSize;
            float3 s = Fetch(input.uv + o);
            float wgt = 1.0 - r * 0.7;
            acc += max(s - threshold, 0.0) * wgt;
            wsum += wgt;
        }
        float3 halo = acc / wsum;
        // Un halo un poco azulado: la luz que se derrama en el aire de un planeta es fria, y asi el brillo no
        // quema las luces calidas (las ciudades) hacia el blanco.
        halo = lerp(halo, halo * float3(0.78, 0.9, 1.15), UserParams[3].y);
        col += halo * glow;
    }

    // 3. LA VINETA: elipse del tamano del cuadro, sin tocar el centro.
    if (vignette > 0.001) {
        float2 q = (input.uv - 0.5) * float2(texWidth / texHeight, 1.0) * 1.15;
        float v = smoothstep(0.55, 1.25, length(q));
        col *= 1.0 - vignette * v;
    }

    // 4. LA CUANTIZACION.
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
