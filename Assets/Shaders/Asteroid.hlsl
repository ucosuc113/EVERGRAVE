// Asteroid: la roca del cinturon. La luz de siempre (StraceLighting, con la cara de noche OSCURA a proposito: ver
// View/AsteroidBelt.cpp) y un borde de luz a CONTRALUZ, como el anillo de un eclipse.
//
// LO QUE IMITA: cuando una roca tapa el Sol, lo que se ve no es la roca iluminada sino su silueta negra con un filo
// encendido alrededor - la luz que pasa rozando el contorno. Por eso el borde es un producto, y ningun factor solo
// alcanza:
//  - `radial`: un DEGRADADO RADIAL alrededor del disco del Sol, tal como lo ve la camara. Se mide el angulo entre el
//    rayo de este pixel y el rayo al centro del Sol: las rocas que lo tapan son las mas encendidas, y segun se alejan
//    hacia los bordes del anillo el filo cae en campana hasta un PISO de casi nada (`floor`) - no cero: con atencion
//    se sigue viendo que todo el anillo esta a contraluz. Una campana y no una recta, para que el centro sea un
//    resplandor y no un cono con punta;
//  - `backlit`: que la camara mire de verdad HACIA la luz. Sin esto, en el mapa (desde arriba) las rocas cerca del
//    Sol en pantalla tambien se encenderian, y ahi no estan delante de el sino a su lado. Va con la direccion de la
//    luz y un umbral suave: a contraluz vale 1, desde arriba 0;
//  - `edge` (Fresnel): el contorno, no el centro. Un borde que pintara la cara entera se leeria como una roca
//    iluminada de frente, que es justo lo que no es. Cerca del Sol el filo es un poco mas ancho (la luz "se come" el
//    borde de lo que la tapa, como en un eclipse); lejos, un hilo.
// El tono es el del Sol (la luz 0), mas calido y saturado: el filo de un eclipse es mas naranja que el disco.
//
// El centro del degradado es la posicion del Sol QUE SE VE, que el script del cinturon escribe en UserParams[1] (w = 1)
// cada frame. Sin ella (w = 0: en el Editor fuera de Play) se usa la direccion de la luz, que cae unos grados corrida.
//
// Las normales de la malla son PLANAS (Tools/gen_rock_gltf.py), asi que el "borde" son las facetas casi de perfil:
// un filo quebrado, de piedra, y no un aro liso de esfera.
//
// Mandos en UserParams[0] (el .strmat): x intensidad en el centro, y ancho del degradado (grados: la desviacion de
// la campana), z lo fino del borde (potencia del Fresnel), w el piso de los bordes (0..1 de la intensidad). En cero,
// valores por defecto - salvo w, donde cero es cero.
//
// EXPERIMENTO: MASCARA VERTICAL (UserParams[2], x = 1 la enciende). Encima del degradado radial, cada roca lleva su
// filo entero ARRIBA y se apaga hacia su MITAD: abajo, nada. Como si la luz llegara rozando desde encima del disco.
// "Arriba" es el de la PANTALLA, no el del mundo: el eje vertical del mundo enderezado contra la direccion de vista
// (asi sigue valiendo con la camara inclinada). La altura se mide desde el centro de la roca (su World) y se divide por
// lo que la roca mide en esa direccion (escala no uniforme y girada incluida), asi una roca chata y una alta se cortan
// igual por su mitad. y = donde empieza a encenderse (0 = la mitad), z = donde ya esta entera (1 = el tope).

float4 PSMain(PSInput input) : SV_TARGET {
    const float3 albedo = AlbedoColor.rgb;
    float3 c = StraceLighting(albedo, input);

    const float strength = UserParams[0].x > 0.0 ? UserParams[0].x : 1.6;
    const float sigmaDeg = UserParams[0].y > 0.0 ? UserParams[0].y : 4.5;
    const float thin = UserParams[0].z > 0.0 ? UserParams[0].z : 3.5;
    const float floorK = saturate(UserParams[0].w);

    const float3 N = normalize(input.worldNormal);
    const float3 V = normalize(CameraWorldPosPad.xyz - input.worldPos);   // del pixel a la camara
    // Sin luces no hay Sol que tapar: sin borde.
    if (LightCountPad.x > 0.5) {
        const float3 toLight = -Lights[0].DirectionRange.xyz;
        const float3 toSunSeen = UserParams[1].w > 0.5 ? normalize(UserParams[1].xyz - CameraWorldPosPad.xyz) : toLight;

        const float angle = degrees(acos(clamp(dot(-V, toSunSeen), -1.0, 1.0)));
        const float bell = exp(-(angle / sigmaDeg) * (angle / sigmaDeg));
        const float radial = lerp(floorK, 1.0, bell);
        const float backlit = smoothstep(0.35, 0.8, dot(-V, toLight));

        const float edge = pow(1.0 - saturate(dot(N, V)), lerp(thin * 1.6, thin * 0.7, bell));
        const float3 rim = Lights[0].ColorIntensity.rgb * float3(1.0, 0.78, 0.45);
        float vmask = 1.0;
        if (UserParams[2].x > 0.5) {
            const float3 up = normalize(float3(0, 1, 0) - V * dot(float3(0, 1, 0), V));   // arriba de la pantalla
            const float3 center = mul(float4(0, 0, 0, 1), World).xyz;
            const float reach = max(1e-4, length(mul((float3x3)World, up)));             // media altura de la roca
            const float h = dot(input.worldPos - center, up) / reach;                     // -1 abajo, 0 mitad, 1 arriba
            const float from = UserParams[2].y, to = UserParams[2].z > from ? UserParams[2].z : from + 0.85;
            vmask = smoothstep(from, to, h);
        }
        c += rim * (strength * radial * backlit * edge * vmask);
    }
    return float4(c, AlbedoColor.a);
}
