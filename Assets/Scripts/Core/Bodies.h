#pragma once

// LOS CUERPOS CELESTES: donde se juega. El arbol de tecnologias es universal -una
// tecnologia vale en cualquier lado-, pero la TIENDA no: cada cuerpo tiene sus propias
// instalaciones (Installations.h lleva el `body` de cada una), y la persona elige en
// cual esta mirando.
//
// El juego va en tres etapas, y cada una vive en un cuerpo: la Tierra (43% del juego),
// la Luna (33%) y el Sol (23%). Empieza SIEMPRE en la Tierra; el resto se abre con
// tecnologias (un efecto UnlockBody, ver Tech.h), y el MAPA del sistema -la vista de
// arriba desde la que se elige- tambien.
//
// `Dyson` es el final: una esfera que fabrica estrellas entre Proxima Centauri y el
// sistema solar. Esta RESERVADO a proposito: existe como cuerpo para que el selector y
// el guardado ya tengan su sitio, pero no tiene contenido ni nada lo abre todavia.
//
// El orden de los valores es el que se guarda y el que recorre el selector: al FINAL
// al agregar uno.

#include <string>

namespace evergrave {

enum class BodyId : int { Earth, Moon, Sun, Dyson };
inline constexpr int kBodyCount = 4;

// El Dyson (el final) esta ESCONDIDO de la interfaz: no sale en el selector de vista. Todo lo demas (el enum, el guardado, la
// tecnologia, las mallas) sigue intacto para retomarlo despues de la entrega: basta poner esto en true.
inline constexpr bool kDysonVisible = false;

struct BodyInfo {
    const char* key;   // estable: es lo que sale de la hoja
    const char* name;
};

inline const BodyInfo& InfoOf(BodyId b) {
    static const BodyInfo infos[kBodyCount] = {{"earth", "EARTH"}, {"moon", "MOON"}, {"sun", "SUN"}, {"dyson", "DYSON"}};
    return infos[static_cast<int>(b)];
}

// De la clave de la hoja ("moon") al cuerpo. Falso si no es un cuerpo (por ejemplo "map").
inline bool BodyFromKey(const std::string& key, BodyId& out) {
    for (int i = 0; i < kBodyCount; ++i)
        if (key == InfoOf(static_cast<BodyId>(i)).key) {
            out = static_cast<BodyId>(i);
            return true;
        }
    return false;
}

// "map" no es un cuerpo sino la VISTA del sistema; un efecto UnlockBody la abre igual.
inline const std::string kMapKey = "map";

} // namespace evergrave
