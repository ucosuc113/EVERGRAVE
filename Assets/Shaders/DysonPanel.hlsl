// DysonPanel: la piel del anillo de Dyson (el modelo `dyson ring.fbx`). Un anillo de colectores que rodea al Sol: lo que se ve es una banda de
// paneles que ATRAPAN la luz de dentro (la cara que mira al Sol arde, la de afuera es metal oscuro), con sus juntas, un pulso de carga que
// le da la vuelta, y el filo caliente de la silueta. Todo en escalones con dither de Bayer, como el resto del juego.
//
// El anillo esta modelado en el plano XY (su eje es Z). Aqui se trabaja en el espacio del OBJETO, que se recupera del mundo con la matriz
// (rotacion por escala uniforme: multiplicar como columna es la inversa salvo la escala, que se divide).
//
// Mandos en UserParams[0]:  x escalones (4-6)   y paneles alrededor (96)   z intensidad general   w velocidad del pulso de carga
//           UserParams[1]:  x calor (0 = el anillo apenas encendido, 1 = al rojo blanco)   y paneles a lo ancho (4)

float Bayer2(float2 a) {
    a = floor(a);
    return frac(a.x * 0.5 + a.y * a.y * 0.75);
}
float Bayer4(float2 a) { return Bayer2(a * 0.5) * 0.25 + Bayer2(a); }
float Hash21(float2 p) {
    p = frac(p * float2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return frac(p.x * p.y);
}

float4 PSMain(PSInput input) : SV_TARGET {
    const float steps = max(UserParams[0].x, 2.0);
    const float around = max(UserParams[0].y, 8.0);
    const float power = UserParams[0].z;
    const float pulseSpeed = UserParams[0].w;
    const float heat = saturate(UserParams[1].x);
    const float across = max(UserParams[1].y, 1.0);

    const float3 center = mul(float4(0, 0, 0, 1), World).xyz;
    const float s2 = max(dot(World[0].xyz, World[0].xyz), 1e-6);
    const float3 p = mul((float3x3)World, input.worldPos - center) / s2; // posicion en el espacio del modelo
    const float ang = atan2(p.y, p.x) / 6.2831853 + 0.5;                 // 0..1 alrededor del anillo
    const float rad = length(p.xy);                                      // la distancia al eje (el radio del anillo)

    // La cara que mira al Sol: la normal contra la direccion al centro (el Sol esta en el centro del anillo).
    const float3 N = normalize(input.worldNormal);
    const float3 toCenter = normalize(center - input.worldPos);
    const float inward = saturate(dot(N, toCenter));                     // 1 = mirando al Sol
    const float3 V = normalize(CameraWorldPosPad.xyz - input.worldPos);
    const float mu = saturate(dot(N, V));

    // Los paneles: una cuadricula alrededor y a lo ancho (el ancho es p.z, de -0,13 a 0,13), con una junta oscura entre uno y otro.
    const float2 g = float2(ang * around, (p.z / 0.254 + 0.5) * across);
    const float2 cell = floor(g);
    const float2 f = frac(g);
    const float seam = step(0.07, f.x) * step(0.07, f.y) * step(f.x, 0.93) * step(f.y, 0.93);
    const float tint = 0.65 + 0.35 * Hash21(cell);                       // cada panel, un poco distinto

    // El pulso de carga: un frente que le da la vuelta al anillo, con cola.
    const float front = frac(Time.x * pulseSpeed);
    const float behind = frac(front - ang);
    const float pulse = pow(saturate(1.0 - behind * 3.0), 2.0);

    // El brillo: el interior arde, el exterior es metal que refleja apenas; el filo (mu bajo) se calienta.
    const float rim = pow(1.0 - mu, 2.0);
    float e = lerp(0.2, 0.75 + 0.5 * heat, inward) * tint * lerp(0.25, 1.0, seam) + 0.9 * pulse * inward + 0.45 * rim * heat;
    e *= power;

    // Color por calor: rojo oscuro -> naranja -> amarillo blanco.
    const float3 cold = float3(0.45, 0.12, 0.05), warm = float3(1.0, 0.55, 0.16), hot = float3(1.0, 0.92, 0.7);
    float3 col = lerp(cold, warm, saturate(e * 1.4));
    col = lerp(col, hot, saturate(e - 0.7) * 1.6);

    const float q = floor(saturate(e) * steps + Bayer4(input.position.xy)) / steps;
    return float4(col * max(q, 0.0) , 1.0);
}
