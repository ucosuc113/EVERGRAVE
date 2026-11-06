// the SUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUN!!!!!!!!!! (but in english... and this is the solar corona XDD)
float Hash11(float n) { return frac(sin(n * 127.1) * 43758.5453); }
float Noise1(float x) {
    const float i = floor(x), f = frac(x);
    const float u = f * f * (3.0 - 2.0 * f);
    return lerp(Hash11(i), Hash11(i + 1.0), u);
}
float Bayer2(float2 a) {
    a = floor(a);
    return frac(a.x * 0.5 + a.y * a.y * 0.75);
}
float Bayer4(float2 a) { return Bayer2(a * 0.5) * 0.25 + Bayer2(a); }

float4 PSMain(PSInput input) : SV_TARGET {
    const float shell = UserParams[0].x > 1.0 ? UserParams[0].x : 1.6;
    const float power = UserParams[0].y > 0.0 ? UserParams[0].y : 1.0;
    const float rays = UserParams[0].z > 0.0 ? UserParams[0].z : 0.45;
    const float fall = UserParams[0].w > 0.0 ? UserParams[0].w : 3.0;
    const float3 N = normalize(input.worldNormal);
    const float3 V = normalize(CameraWorldPosPad.xyz - input.worldPos);
    const float mu = saturate(dot(N, V));
    const float r = shell * sqrt(saturate(1.0 - mu * mu));
    const float outside = exp(-(r - 1.0) * fall) * (1.0 - smoothstep(shell * 0.85, shell, r));
    float glow = r >= 1.0 ? outside : 0.12;
    const float3 center = mul(float4(0, 0, 0, 1), World).xyz;
    const float3 up = normalize(float3(0, 1, 0) - V * dot(float3(0, 1, 0), V));
    const float3 right = cross(up, V);
    const float3 d = input.worldPos - center;
    const float a = atan2(dot(d, up), dot(d, right));
    const float s = a * 3.8197;
    float streak = 0.6 * Noise1(s + Time.x * 0.15) + 0.4 * Noise1(s * 2.0 - Time.x * 0.22 + 31.0);
    streak = lerp(1.0, 0.4 + 1.2 * streak, rays * saturate((r - 1.0) * 3.0));
    glow *= streak * power;
    const float g = floor(saturate(glow) * 4.0 + Bayer4(input.position.xy)) / 4.0;
    const float3 col = lerp(float3(1.0, 0.45, 0.12), float3(1.0, 0.80, 0.42), saturate(glow * 1.4));
    return float4(col, g);
}
