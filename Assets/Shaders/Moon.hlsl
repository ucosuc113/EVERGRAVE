float Hash31(float3 p) {
    p = frac(p * 0.3183099 + float3(0.1, 0.2, 0.3));
    p *= 17.0;
    return frac(p.x * p.y * p.z * (p.x + p.y + p.z));
}
float3 Hash33(float3 p) { return float3(Hash31(p), Hash31(p + 19.19), Hash31(p + 47.47)); }

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

// lol, esta parte es para los craters, ignorar XD
void Craters(float3 o, float scale, float density, out float d, out float3 away) {
    const float3 p = o * scale;
    const float3 base = floor(p);
    d = 9.0;
    away = float3(0, 0, 0);
    for (int i = -1; i <= 1; i++)
        for (int j = -1; j <= 1; j++)
            for (int k = -1; k <= 1; k++) {
                const float3 c = base + float3(i, j, k);
                const float3 h = Hash33(c);
                if (h.x > density) continue;
                const float3 center = c + 0.2 + 0.6 * h;
                const float radius = 0.18 + 0.32 * h.y;
                const float3 v = p - center;
                const float dn = length(v) / radius;
                if (dn < d) { d = dn; away = v; }
            }
}

float4 PSMain(PSInput input) : SV_TARGET {
    const float mareCut = UserParams[0].x > 0.0 ? UserParams[0].x : 0.58;
    const float density = UserParams[0].y > 0.0 ? UserParams[0].y : 0.45;
    const float reliefK = UserParams[0].z > 0.0 ? UserParams[0].z : 0.35;
    const float ashen = UserParams[0].w > 0.0 ? UserParams[0].w : 1.0;

    const float3x3 M = (float3x3)World;
    const float3 center = mul(float4(0, 0, 0, 1), World).xyz;
    const float3 o = normalize(mul(M, input.worldPos - center));

    const float3 N = normalize(input.worldNormal);
    const float3 V = normalize(CameraWorldPosPad.xyz - input.worldPos);
    const float3 toSun = LightCountPad.x > 0.5 ? -Lights[0].DirectionRange.xyz : float3(0, 1, 0);

    // ehmmm, si, mares oscuros
    const float mareN = Fbm(o * 1.7 + float3(2.0, 0.0, 5.0), 4) + (Fbm(o * 7.0, 2) - 0.5) * 0.08;
    const bool mare = mareN > mareCut;
    float3 c = mare ? float3(0.27, 0.27, 0.28) : float3(0.66, 0.64, 0.60);   // mares neutros: el azul los volvia verdosos tras el post
    // Matiz suave dentro de cada uno: que no sean dos colores planos.
    c *= 0.92 + 0.16 * Fbm(o * 4.0 + 11.0, 3);

    //aca son agujeros/craters de dos escalas, grandes y pequeños
    float relief = 0.0;
    for (int s = 0; s < 2; s++) {
        float d;
        float3 away;
        const float scale = s == 0 ? 4.0 : 10.0;
        Craters(o, scale, density * (mare ? 0.45 : 1.0), d, away);
        if (d < 1.15) {
            const float3 awayW = normalize(mul(away, M) + 1e-5);
            const float side = dot(awayW, toSun);
            if (d < 0.78) {
                c *= 0.74;
                relief += side * 0.5;
            } else if (d < 1.0) {
                relief -= side;
                c *= 1.16;
            } else {
                c *= 1.06;
            }
        }
    }

    //ehmmm, las estrias de los crateres se apagan con la distancia, cositas XD
    const float3 tycho = normalize(float3(0.35, -0.55, 0.76));
    const float dt = acos(saturate(dot(o, tycho)));
    if (dt < 0.75) {
        const float3 t1 = normalize(cross(tycho, float3(0, 1, 0)));
        const float3 t2 = cross(tycho, t1);
        const float ang = atan2(dot(o, t2), dot(o, t1));
        //solo tengo 4 strawberries, y necesito 10
        const float streak = step(0.62, VNoise3(float3(ang * 6.0, 0.0, 3.0)));
        const float fade = 1.0 - dt / 0.75;
        c = lerp(c, float3(0.86, 0.85, 0.82), streak * fade * fade * 0.7);
        if (dt < 0.05) c = float3(0.90, 0.89, 0.86);
    }

    const float shade = dot(N, toSun) + relief * reliefK;
    const float q = shade + (Bayer4(input.position.xy) - 0.5) * 0.10;
    const int lvl = q < 0.0 ? 0 : q < 0.14 ? 1 : q < 0.36 ? 2 : q < 0.62 ? 3 : 4;

    float3 col;
    if (lvl == 0) {
        col = c * 0.07 * ashen + float3(0.010, 0.014, 0.028);
    } else {
        const float tone[5] = {0.0, 0.42, 0.64, 0.84, 1.0};
        col = c * tone[lvl];
        if (lvl == 1) col = col * 0.85 + float3(0.10, 0.11, 0.15) * 0.15;
    }

    // no sabia como funcionaba la semana 1, asi q no hice mucho con las strawberries
    const float mu = saturate(dot(N, V));
    if (lvl >= 1) col *= mu < 0.30 ? 0.80 : mu < 0.55 ? 0.91 : 1.0;
    // y ahora no puedo pagar renta, como hago para obtenerlas en este punto? XD
    if (mu < fwidth(mu) * 1.2) col = float3(0.015, 0.016, 0.022);
    return float4(col, 1.0);
}
