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

float Fbm(float3 p, int octaves) {
    float a = 0.5, sum = 0.0, norm = 0.0;
    for (int i = 0; i < octaves; i++) {
        sum += a * VNoise3(p);
        norm += a;
        a *= 0.5;
        p *= 2.03;
    }
    return sum / norm;
}

float Bayer2(float2 a) {
    a = floor(a);
    return frac(a.x * 0.5 + a.y * a.y * 0.75);
}
float Bayer4(float2 a) { return Bayer2(a * 0.5) * 0.25 + Bayer2(a); }

// Los umbrales de los percentiles del ruido: el mar cubre ~68%, las nubes ~29%, y asi.
static const float kSea = 0.552, kDesert = 0.426, kForest = 0.522, kCloud = 0.590, kCloudDense = 0.645;
static const float kCity = 0.650, kCityBig = 0.714;

float4 PSMain(PSInput input) : SV_TARGET {
    const float lon = input.uv.x * 6.2831853;
    const float lat = (0.5 - input.uv.y) * 3.1415927;
    const float3 p = float3(cos(lat) * cos(lon), sin(lat), cos(lat) * sin(lon));
    const float al = abs(lat);

    // El relieve, la humedad y las nubes. Las nubes van en un ruido torcido por otro (para que no sean manchas
    // redondas) y giran un poco mas rapido que la superficie.
    const float e = 0.7 * Fbm(p * 1.7 + float3(3, 0, 0), 6) + 0.3 * Fbm(p * 4.0 + float3(9, 0, 0), 3);
    const float moist = Fbm(p * 2.4 + float3(1, 0, 0), 4);
    const float ca = Time.x * 0.012, cs = sin(ca), cc = cos(ca);
    const float3 pc = float3(cc * p.x + cs * p.z, p.y, -cs * p.x + cc * p.z);
    const float3 warp = float3(Fbm(pc * 2.0 + float3(5, 0, 0), 3), Fbm(pc * 2.0 + float3(0, 5, 0), 3), Fbm(pc * 2.0 + float3(0, 0, 5), 3)) - 0.5;
    const float cloud = Fbm(pc * 3.2 + warp * 1.6, 5);

    // El color de la superficie (de dia, sin sombrear).
    const bool land = e >= kSea;
    float3 c;
    bool ocean = false;
    // El casquete polar tiene el borde roto por el relieve: un corte limpio se leia como un disco pegado.
    if (al > 1.38 + (e - 0.5) * 0.5) {
        c = float3(0.84, 0.89, 0.94);
    } else if (!land) {
        ocean = true;
        const float depth = kSea - e;
        c = depth < 0.025 ? float3(0.20, 0.51, 0.70) : depth < 0.07 ? float3(0.13, 0.37, 0.60) : float3(0.10, 0.26, 0.49);
    } else {
        const float h = e - kSea;
        if (al > 1.25 || (h > 0.14 && al > 0.8)) c = float3(0.91, 0.94, 0.96);
        else if (h > 0.14) c = float3(0.48, 0.45, 0.40);
        else if (moist < kDesert && al < 1.0) c = float3(0.75, 0.63, 0.38);
        else c = moist > kForest ? float3(0.15, 0.38, 0.20) : float3(0.27, 0.53, 0.26);
    }
    const bool covered = cloud > kCloud;
    if (covered) c = cloud > kCloudDense ? float3(0.96, 0.98, 0.99) : float3(0.84, 0.88, 0.93);

    // La luz: el Sol es la primera luz de la escena. Sin ella (una escena sin luces) la Tierra queda de dia.
    const float3 N = normalize(input.worldNormal);
    const float3 V = normalize(CameraWorldPosPad.xyz - input.worldPos);
    const float3 toSun = LightCountPad.x > 0.5 ? -Lights[0].DirectionRange.xyz : float3(0, 1, 0);
    const float shade = dot(N, toSun);
    const float q = shade + (Bayer4(input.position.xy) - 0.5) * 0.10;
    const int lvl = q < 0.0 ? 0 : q < 0.16 ? 1 : q < 0.38 ? 2 : q < 0.62 ? 3 : 4;

    float3 col;
    if (lvl == 0) {
        // La noche: el mismo color muy oscuro y azulado, y las ciudades donde hay tierra y no hay nube encima.
        col = c * 0.12 + float3(0.012, 0.024, 0.063);
        if (land && !covered && al < 1.15) {
            const float city = Fbm(p * 18.0, 2);
            if (city > kCityBig) col = float3(1.0, 0.84, 0.47);
            else if (city > kCity) col = float3(0.69, 0.50, 0.24);
        }
    } else {
        const float tone[5] = {0.0, 0.50, 0.72, 0.88, 1.0};
        col = c * tone[lvl];
        // El borde del dia se calienta un poco (el atardecer).
        if (lvl == 1) col = col * 0.78 + float3(0.93, 0.50, 0.24) * 0.22;
        // El brillo del Sol sobre el mar, donde no hay nube.
        if (ocean && !covered && lvl >= 3) {
            const float sp = pow(max(0.0, dot(N, normalize(toSun + V))), 260.0);
            if (sp > 0.5) col = float3(1.0, 0.96, 0.86);
            else if (sp > 0.22) col = float3(0.66, 0.84, 0.93);
        }
    }

    // El borde de la atmosfera: mas fuerte del lado del dia.
    const float rim = 1.0 - saturate(dot(N, V));
    const float k = rim > 0.72 ? 0.50 : rim > 0.45 ? 0.24 : 0.0;
    col = lvl >= 1 ? lerp(col, float3(0.51, 0.73, 1.0), k) : lerp(col, float3(0.09, 0.17, 0.34), k * 0.6);
    return float4(col, 1.0);
}
