// LA TIERRA, lo minimo: mar y tierra, dos colores planos sobre un ruido.
// Es relleno a proposito (sin nubes, hielo ni atmosfera): se cambia por arte
// real mas adelante. Sale de la UV y no de la posicion para que los continentes
// giren con la malla. El sombreado lo pone el motor con la luz del Sol.

float Hash31(float3 p) {
    p = frac(p * 0.3183099 + float3(0.1, 0.2, 0.3));
    p *= 17.0;
    return frac(p.x * p.y * p.z * (p.x + p.y + p.z));
}

float VNoise3(float3 x) {
    float3 i = floor(x), f = frac(x);
    f = f * f * (3.0 - 2.0 * f);
    float a = lerp(lerp(Hash31(i), Hash31(i + float3(1, 0, 0)), f.x),
                   lerp(Hash31(i + float3(0, 1, 0)), Hash31(i + float3(1, 1, 0)), f.x), f.y);
    float b = lerp(lerp(Hash31(i + float3(0, 0, 1)), Hash31(i + float3(1, 0, 1)), f.x),
                   lerp(Hash31(i + float3(0, 1, 1)), Hash31(i + float3(1, 1, 1)), f.x), f.y);
    return lerp(a, b, f.z);
}

float4 PSMain(PSInput input) : SV_TARGET {
    const float lon = input.uv.x * 6.2831853;
    const float lat = (0.5 - input.uv.y) * 3.1415927;
    const float3 p = float3(cos(lat) * cos(lon), sin(lat), cos(lat) * sin(lon));
    const float h = VNoise3(p * 2.5 + 3.7) * 0.65 + VNoise3(p * 5.0 + 1.3) * 0.35;
    const float3 sea = float3(0.07, 0.25, 0.55);
    const float3 land = float3(0.18, 0.48, 0.22);
    return float4(StraceLighting(h > 0.52 ? land : sea, input), 1.0);
}
