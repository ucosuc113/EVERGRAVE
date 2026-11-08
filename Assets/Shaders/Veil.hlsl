float Hash21(float2 p) {
    p = frac(p * float2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return frac(p.x * p.y);
}
float Band(float3 d, float3 n) { return 1.0 - smoothstep(0.0, 0.085, abs(dot(d, normalize(n)))); }
// aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
float4 PSMain(PSInput input) : SV_TARGET {
    const float density = UserParams[0].x;
    const float power = UserParams[0].y;
    if (density <= 0.001) return float4(0, 0, 0, 0);
    const float3 center = mul(float4(0, 0, 0, 1), World).xyz;
    const float3 worldDir = normalize(input.worldPos - center);
    const float3 d = normalize(mul((float3x3)World, worldDir));
    const float band = max(max(Band(d, float3(0.0, 1.0, 0.0)), Band(d, float3(0.55, 0.8, 0.0))), Band(d, float3(-0.4, 0.75, 0.5)));
    if (band <= 0.0) return float4(0, 0, 0, 0);
    const float2 cell = floor(float2(atan2(d.z, d.x) * 57.29578, asin(clamp(d.y, -1.0, 1.0)) * 57.29578));
    const float h = Hash21(cell);
    if (h > density * 0.035 * band) return float4(0, 0, 0, 0);
    const float3 toSun = LightCountPad.x > 0.5 ? -Lights[0].DirectionRange.xyz : float3(0, 1, 0);
    const float day = smoothstep(-0.25, 0.55, dot(worldDir, toSun));
    const float brightness = lerp(0.22, 1.0, day) * (0.55 + 0.45 * frac(h * 91.7));
    return float4(AlbedoColor.rgb, saturate(brightness * power));
}
