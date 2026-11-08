#pragma once

// LA MUSICA: dos archivos que son LA MISMA cancion y suenan a la vez, siempre.
//
//   "you are my destiny main menu.mp3"   solo la melodia (la voz principal): lo que suena en el titulo y el menu
//   "you are my destiny.mp3"             la cancion entera (melodia + acompanantes): lo que suena jugando
//
// Como los dos duran lo mismo y llevan la melodia en el mismo compas, no hace falta cortar de uno a otro ni volver
// a buscar "el segundo en que ibamos": los dos arrancan juntos al abrir el juego y avanzan en paralelo, y pasar
// del menu a la partida es MOVER DOS VOLUMENES - la melodia no cambia, los acompanantes aparecen.
//
// Tres decisiones que no se ven en el codigo:
//
//  - **Arrancan con el mezclador en pausa**, y se reanuda con las dos ya puestas. Si no, la primera voz puede
//    mezclarse un bloque de audio (~10 ms) antes que la segunda, y desde ahi las dos melodias quedan desfasadas:
//    un fundido entre dos copias retrasadas de la misma cosa suena a filtro de peine, y esto es justo un fundido.
//  - **El fundido es COMPLEMENTARIO y LINEAL en ganancia** (menu = 1 - f, partida = f), no en decibelios como
//    `AudioService::FadeBus`: la melodia esta en las dos, y dos copias coherentes suman en amplitud, asi que
//    1 - f + f = 1 la deja exactamente igual. En dB habria un bache a mitad del fundido, y la melodia es lo unico
//    que no puede moverse. (Por eso se escribe el dB del bus a mano cada frame y no con un tween.)
//  - **No es la "pista" del motor** (`PlayTrack`): esa es UNA y marca el reloj, y aqui hacen falta dos voces. Se
//    usan los buses Music y Sfx - Sfx porque el juego aun no tiene efectos - uno por capa. El dia que haya
//    efectos de sonido, la capa completa tiene que mudarse a un bus propio (hay que anadirlo al motor).

#include "Engine/Audio/AudioService.h"
#include "Engine/Diagnostics/Log.h"
#include "Engine/Scene/SceneService.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <chrono>
#include <string>
#include <thread>

namespace evergrave {

constexpr float kMusicMasterDb = -5.0f; // era -11: se oia flojo. Sigue por debajo de 0 para dejar margen a los efectos que vengan
constexpr float kMusicFadeSeconds = 3.0f;
// EL ARRANQUE: el archivo de la melodia es silencio digital exacto hasta el segundo 9,03 (ahi entra la primera nota; medido,
// no a ojo). Empezar en 0:00 dejaba 9 s de pantalla de titulo sin sonido, asi que al ABRIR el juego las dos capas arrancan
// un poco antes de esa nota (la misma cantidad en las dos, para que sigan alineadas): el titulo suena de verdad. Solo al
// abrir: el bucle vuelve a 0:00 como siempre, y una partida directa (sin menu) tampoco salta nada.
constexpr double kMelodyEntersAt = 9.03;
constexpr double kMelodyPreRoll = 0.05;

struct MusicState {
    strace::VoiceId melodyVoice = strace::kInvalidVoice; // solo la melodia
    strace::VoiceId fullVoice = strace::kInvalidVoice;   // la cancion entera
    float mix = 0.0f;   // 0 = solo la melodia, 1 = la cancion entera
    float from = 0.0f, to = 0.0f, t = 0.0f, duration = 0.0f;
    float driftClock = 0.0f;
    bool driftWarned = false;
};
inline MusicState& MusicS() {
    static MusicState s;
    return s;
}

// Empuja el mix a los dos buses. -80 dB o menos es silencio exacto.
inline void MusicApply() {
    using namespace strace;
    const float m = std::clamp(MusicS().mix, 0.0f, 1.0f);
    *AudioService::BusVolumeDbTarget(AudioBus::Master) = kMusicMasterDb;
    *AudioService::BusVolumeDbTarget(AudioBus::Music) = std::max(-80.0f, GainToDecibels(1.0f - m));
    *AudioService::BusVolumeDbTarget(AudioBus::Sfx) = std::max(-80.0f, GainToDecibels(m));
}

// Adelanta un stream recien abierto leyendo (y tirando) `frames` cuadros ANTES de ponerlo a sonar. AudioStream no tiene
// seek publico, y esto no necesita tocar el motor: mientras la voz no esta en el mezclador, el hilo del juego es el unico
// que lee el anillo, y el decodificador lo va rellenando. Son ~9 s de mp3 (unos centenares de ms); si el decodificador
// se atrasa se espera, con tope, y si no llega se arranca desde donde se llego.
inline long long MusicSkip(strace::AudioStream& stream, long long frames) { // devuelve cuantos cuadros adelanto de verdad
    const long long asked = frames;
    static float scratch[2048 * 2];
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (frames > 0 && std::chrono::steady_clock::now() < deadline) {
        const int want = static_cast<int>(std::min<long long>(frames, 2048));
#if STRACE_AUDIO_NO_THREADS
        stream.PumpDecode(); // Web: no hay hilo decodificador, lo bombea el juego (aqui y en AudioService::Update)
#endif
        const int got = stream.ReadFrames(scratch, want);
        frames -= got;
        if (got < want) std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return asked - frames;
}

// Abre las dos y las pone a sonar a la vez. `full`: arrancar ya con la cancion entera (una partida sin menu).
inline void MusicStart(bool full) {
    using namespace strace;
    MusicState& m = MusicS();
    if (m.melodyVoice != kInvalidVoice && AudioService::Mixer().Playing(m.melodyVoice)) return; // ya suena
    const std::wstring dir = SceneService::ProjectRoot() + L"/Assets/Audio/";
    AudioStreamPtr melody = AudioService::OpenStream(dir + L"you are my destiny main menu.mp3", true);
    AudioStreamPtr song = AudioService::OpenStream(dir + L"you are my destiny.mp3", true);
    if (!melody || !song) Log::Warn(L"EVERGRAVE: falta uno de los dos audios de la musica (Assets/Audio): no habra fundido");

    // Las dos se adelantan EXACTAMENTE lo mismo (sino la melodia y la cancion quedarian desfasadas para siempre).
    if (!full && melody && song) {
        const long long skip = static_cast<long long>((kMelodyEntersAt - kMelodyPreRoll) * melody->SampleRate());
        const long long a = MusicSkip(*melody, skip);
        long long b = MusicSkip(*song, a); // la segunda, lo mismo que consiguio la primera (si una se atraso, no se desfasan)
        if (a != skip || b != a)
            Log::Warn(L"EVERGRAVE: el arranque de la musica no llego al segundo " + std::to_wstring(kMelodyEntersAt - kMelodyPreRoll) + L" (melodia " + std::to_wstring(a) + L", cancion " + std::to_wstring(b) + L" cuadros)");
        else
            Log::Info(L"EVERGRAVE: la musica arranca en el segundo " + std::to_wstring(kMelodyEntersAt - kMelodyPreRoll) + L" (" + std::to_wstring(a) + L" cuadros), donde entra la melodia");
    }

    const bool wasPaused = AudioService::Paused();
    AudioService::SetPaused(true);
    m.melodyVoice = melody ? AudioService::Mixer().PlayStream(melody, AudioBus::Music) : kInvalidVoice;
    m.fullVoice = song ? AudioService::Mixer().PlayStream(song, AudioBus::Sfx) : kInvalidVoice;
    AudioService::SetPaused(wasPaused);

    // Si solo abrio uno, ese suena entero y no hay nada que fundir.
    m.mix = (full || !melody) ? 1.0f : 0.0f;
    if (!song) m.mix = 0.0f;
    m.from = m.to = m.mix;
    m.t = m.duration = 0.0f;
    m.driftWarned = false;
    MusicApply();
}

// Del menu a la partida: los acompanantes entran en `seconds`, la melodia no se mueve.
inline void MusicToFull(float seconds = kMusicFadeSeconds) {
    MusicState& m = MusicS();
    m.from = m.mix;
    m.to = 1.0f;
    m.t = 0.0f;
    m.duration = std::max(0.001f, seconds);
}

// Una vez por frame. Avanza el fundido y vigila que las dos voces sigan en el mismo cuadro.
inline void MusicUpdate(float dt) {
    using namespace strace;
    MusicState& m = MusicS();
    if (m.duration > 0.0f) {
        m.t = std::min(m.duration, m.t + dt);
        const float k = m.t / m.duration;
        const float s = k * k * (3.0f - 2.0f * k); // smoothstep: sin golpe al empezar ni al llegar
        m.mix = m.from + (m.to - m.from) * s;
        if (m.t >= m.duration) m.duration = 0.0f;
        MusicApply();
    }
    // Si un decodificador se atrasara, esa voz se quedaria atras para siempre (su posicion no avanza mientras no
    // tiene datos) y la melodia sonaria doble. No se puede corregir sin buscar en el stream, pero SI avisar.
    m.driftClock += dt;
    if (m.driftClock >= 2.0f && !m.driftWarned && m.melodyVoice != kInvalidVoice && m.fullVoice != kInvalidVoice) {
        m.driftClock = 0.0f;
        const long long a = static_cast<long long>(AudioService::Mixer().VoiceFrame(m.melodyVoice));
        const long long b = static_cast<long long>(AudioService::Mixer().VoiceFrame(m.fullVoice));
        if (a >= 0 && b >= 0 && std::llabs(a - b) > 441) { // mas de 10 ms
            m.driftWarned = true;
            Log::Warn(L"EVERGRAVE: las dos capas de la musica se desfasaron " + std::to_wstring(std::llabs(a - b)) + L" cuadros");
        }
    }
}

} // namespace evergrave
