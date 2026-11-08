// Glow: un resplandor ADITIVO sobre una esfera. El mismo truco que la atmosfera de la Tierra (EarthAtmosphere.hlsl) y la corona del Sol
// (SunCorona.hlsl): una esfera siempre muestra su cara de frente a la camara, asi que no hace falta un billboard. El borde de la esfera
// cae a cero y el centro brilla: un punto de luz redondo y suave que, por ser un OBJETO de la escena, recibe el bloom, el desenfoque
// y el ajuste de color como cualquier otra cosa (a diferencia de los discos de UI de FxKit::Halo, que se dibujan despues de todo eso).
//
// Se usa para las luces de los satelites, la baliza de un relay y, de aqui en adelante, todo lo que emite luz en la escena.
//
// Mandos en UserParams[0]:  x la caida (1 = lineal, 3 = un nucleo chico y un borde largo)   y la intensidad (0..2)
//                           z los escalones del dither (3-5: el aspecto de pixel art)         w el modo:
//                             0 constante, 1 destello corto (una luz de navegacion), 2 pulso lento (una baliza)
// El color es el albedo del material. Cada resplandor tiene su fase: sale de la POSICION del objeto, asi dos relays del mismo material
// no laten a la vez sin que haga falta un material por pieza.

float Bayer2(float2 a) {
    a = floor(a);
    return frac(a.x * 0.5 + a.y * a.y * 0.75);
}
float Bayer4(float2 a) { return Bayer2(a * 0.5) * 0.25 + Bayer2(a); }

float4 PSMain(PSInput input) : SV_TARGET {
    const float falloff = max(UserParams[0].x, 0.5);
    const float power = UserParams[0].y;
    const float steps = max(UserParams[0].z, 1.0);
    const float mode = UserParams[0].w;

    const float3 N = normalize(input.worldNormal);
    const float3 V = normalize(CameraWorldPosPad.xyz - input.worldPos);
    float g = pow(saturate(dot(N, V)), falloff) * power;

    const float3 center = mul(float4(0, 0, 0, 1), World).xyz;
    const float phase = frac(dot(center, float3(0.137, 0.291, 0.173)) * 7.0);
    if (mode > 1.5) g *= 0.65 + 0.35 * sin(Time.x * 2.0 + phase * 6.2832);
    else if (mode > 0.5) g *= 0.12 + 0.88 * step(frac(Time.x * 0.45 + phase), 0.14);

    const float q = floor(saturate(g) * steps + Bayer4(input.position.xy)) / steps;
    return float4(AlbedoColor.rgb, q);
}
