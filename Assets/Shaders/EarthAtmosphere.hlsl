// EarthAtmosphere: la capa de aire de la Tierra, vista desde fuera. La hermana chica de la corona del Sol
// (SunCorona.hlsl, que explica el truco entero): una esfera apenas mas grande que la Tierra (`Earth/Atmosphere`, hija
// suya, escala `UserParams[0].x` respecto a ella) con mezcla ADITIVA, que deja un filo azul por FUERA del disco.
//
// La Tierra ya aclaraba su borde por dentro (Earth.hlsl, "el borde de la atmosfera"), pero dentro del disco: la
// silueta seguia siendo un corte limpio contra el negro, y eso es lo que la hacia ver plana y "de foto". Esto pone el
// aire FUERA de la silueta, que es lo que se ve en las fotos de la Tierra desde el espacio: una pelicula fina y
// luminosa en el limbo.
//
// Lo que la distingue de la corona:
//  - es FINA: la cascara es un 8% mas grande (la corona, 60%) y la luz cae a cero en su borde;
//  - depende del SOL: del lado del dia brilla, del de noche queda un resto apenas visible (`nightFloor`). Se mide con
//    la direccion desde el centro de la Tierra hasta este pixel de la cascara, comparada con la del Sol;
//  - en el TERMINADOR se calienta: el aire que el Sol cruza rasante es el del amanecer y el atardecer, naranja;
//  - sin filamentos: el aire no tiene rayos.
// En escalones con dither, como todo lo demas.
//
// Mandos en UserParams[0]: x la escala de la cascara respecto a la Tierra (la del nodo: si cambia una, cambiar la
// otra), y intensidad, z el resto del lado de noche (0..1). En cero, por defecto.

float Bayer2(float2 a) {
    a = floor(a);
    return frac(a.x * 0.5 + a.y * a.y * 0.75);
}
float Bayer4(float2 a) { return Bayer2(a * 0.5) * 0.25 + Bayer2(a); }

float4 PSMain(PSInput input) : SV_TARGET {
    const float shell = UserParams[0].x > 1.0 ? UserParams[0].x : 1.08;
    const float power = UserParams[0].y > 0.0 ? UserParams[0].y : 0.9;
    const float nightFloor = UserParams[0].z > 0.0 ? UserParams[0].z : 0.12;

    const float3 N = normalize(input.worldNormal);
    const float3 V = normalize(CameraWorldPosPad.xyz - input.worldPos);
    const float mu = saturate(dot(N, V));
    const float r = shell * sqrt(saturate(1.0 - mu * mu));    // en radios de la Tierra: 1 = el borde del disco

    // Fuera del disco: lleno en el borde, cero en el de la cascara. Dentro, solo un resto pegado al limbo (la parte de
    // la pelicula que se ve delante del planeta).
    float glow;
    if (r >= 1.0) {
        const float t = saturate((r - 1.0) / (shell - 1.0));
        glow = (1.0 - t) * (1.0 - t);
    } else {
        glow = smoothstep(0.9, 1.0, r) * 0.35;
    }

    // Dia y noche: la direccion del centro a este pixel contra la del Sol.
    const float3 center = mul(float4(0, 0, 0, 1), World).xyz;
    const float3 toSun = LightCountPad.x > 0.5 ? -Lights[0].DirectionRange.xyz : float3(0, 1, 0);
    const float sunSide = dot(normalize(input.worldPos - center), toSun);
    const float day = lerp(nightFloor, 1.0, smoothstep(-0.25, 0.35, sunSide));
    glow *= day * power;
    // De fondo (vista de la Luna): se apaga con la Tierra. UserParams[1].x lo escribe SpaceWorld, como en Earth.hlsl.
    glow *= lerp(1.0, 0.4, saturate(UserParams[1].x));

    // El color: azul de dia, naranja en la franja del terminador.
    const float dusk = 1.0 - smoothstep(0.0, 0.35, abs(sunSide + 0.05));
    const float3 col = lerp(float3(0.42, 0.68, 1.0), float3(1.0, 0.58, 0.30), dusk * 0.7);

    // Tres escalones con dither.
    const float g = floor(saturate(glow) * 3.0 + Bayer4(input.position.xy)) / 3.0;
    return float4(col, g);
}
