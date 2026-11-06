//el SOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOL!!!!!!

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

float4 PSMain(PSInput input) : SV_TARGET {
    const float granK = UserParams[0].x > 0.0 ? UserParams[0].x : 0.22;
    const float spotCut = UserParams[0].y > 0.0 ? UserParams[0].y : 0.76;
    const float boil = UserParams[0].z > 0.0 ? UserParams[0].z : 0.25;

    const float3 N = normalize(input.worldNormal);
    const float3 V = normalize(CameraWorldPosPad.xyz - input.worldPos);
    const float mu = saturate(dot(N, V));
    // EL SOL
    const float3 center = mul(float4(0, 0, 0, 1), World).xyz;
    const float3 q = normalize(input.worldPos - center);
    const float ang = Time.x * 0.05, sa = sin(ang), ca = cos(ang);
    const float3 p = float3(ca * q.x + sa * q.z, q.y, -sa * q.x + ca * q.z);
    const float t = Time.x * boil;
    const float gran = Fbm(p * 9.0 + float3(t, -t * 0.7, t * 0.4), 3) - 0.5;
    const float spotN = Fbm(p * 2.3 + float3(7.0, 0.0, Time.x * 0.01), 3);
    const float band = 1.0 - smoothstep(0.45, 0.75, abs(p.y));
    const float spot = (spotN - spotCut) * band;

    float v = pow(mu, 0.45) + gran * granK;
    v += (Bayer4(input.position.xy) - 0.5) * 0.12;

    float3 col;
    if (v < 0.35)      col = float3(0.93, 0.36, 0.08);
    else if (v < 0.55) col = float3(1.00, 0.55, 0.14);
    else if (v < 0.72) col = float3(1.00, 0.74, 0.26);
    else if (v < 0.88) col = float3(1.00, 0.88, 0.45);
    else               col = float3(1.00, 0.97, 0.78);

    if (mu > 0.25) {
        if (spot > 0.07)      col = float3(0.42, 0.14, 0.04);
        else if (spot > 0.0)  col = float3(0.86, 0.40, 0.10);
    }
    return float4(col, 1.0);
}
