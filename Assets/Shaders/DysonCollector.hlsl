// DysonCollector: la piel del colector de Dyson (el modelo `dyson collector.fbx`). Una pieza del enjambre que rodea al Sol: cada cara se ilumina por
// cuanto MIRA AL SOL (no por la luz de la escena: va del origen a la Tierra, y el Sol que se ve esta en otra parte), la que lo mira arde y la de atras es
// metal frio. Escalones con dither, como todo lo demas.
//
// Se usa en DOS materiales, uno por submalla del modelo: `CollectorPanel` (Material.001, la estructura gris) y `CollectorGold` (Material.002, el detalle
// dorado). Cambia solo el color base (el albedo) y el calor.
//
// La posicion del Sol la escribe SunWorks cada frame en UserParams[1].xyz (w = 1: "hay dato"); sin el dato, el origen del mundo.
// Mandos en UserParams[0]:  x los escalones (3-5)   y el calor (cuanto arde la cara que mira al Sol)   z la intensidad general

float Bayer2(float2 a) {
    a = floor(a);
    return frac(a.x * 0.5 + a.y * a.y * 0.75);
}
float Bayer4(float2 a) { return Bayer2(a * 0.5) * 0.25 + Bayer2(a); }

float4 PSMain(PSInput input) : SV_TARGET {
    const float steps = max(UserParams[0].x, 2.0);
    const float heat = UserParams[0].y;
    const float power = UserParams[0].z;

    const float3 sunPos = UserParams[1].w > 0.5 ? UserParams[1].xyz : float3(0, 0, 0);
    const float3 N = normalize(input.worldNormal);
    const float3 toSun = normalize(sunPos - input.worldPos);
    const float lit = saturate(dot(N, toSun));                  // 1 = de frente al Sol
    const float3 V = normalize(CameraWorldPosPad.xyz - input.worldPos);
    const float rim = pow(1.0 - saturate(dot(N, V)), 2.0);      // el filo

    // La luz en escalones: un minimo (la cara de atras no es negra: refleja el Sol de los demas) y el maximo de frente.
    const float k = lerp(0.2, 1.0, lit);
    const float q = floor(k * steps + Bayer4(input.position.xy)) / steps;

    // El color propio por la luz, mas el calor del Sol en la cara que lo mira y un filo caliente.
    float3 col = AlbedoColor.rgb * q * power;
    col += float3(1.0, 0.55, 0.2) * pow(lit, 3.0) * heat * 0.55;
    col += float3(1.0, 0.7, 0.35) * rim * lit * heat * 0.35;
    return float4(col, 1.0);
}
