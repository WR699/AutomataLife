#pragma once
#include <cstddef>
#include <string>
#include <vector>

class Estado;
class Automata;

// Una Transicion representa UNA arista del automata:
// un simbolo y exactamente un destino cuando esta insertada en un Automata.
class Transicion {
    friend class Automata;
    friend class Estado;

    std::string simbolo_;
    Estado* destino_ = nullptr; // Puntero observador, nunca propietario.
    Automata* propietario_ = nullptr;

    void comprobar(Estado* destino) const;

public:
    explicit Transicion(std::string simbolo = "");
    Transicion(std::string simbolo, Estado* destino);

    // Compatibilidad con codigo viejo. Solo acepta cero o un destino.
    // Las listas con varios destinos se separan en Automata::agregarTransicion().
    Transicion(std::string simbolo, const std::vector<Estado*>& destinos);

    Transicion(const Transicion& otra);
    Transicion(Transicion&& otra) noexcept;
    Transicion& operator=(const Transicion& otra);
    Transicion& operator=(Transicion&& otra) noexcept;

    const std::string& getSimbolo() const noexcept;
    void setSimbolo(const std::string& simbolo);

    Estado* getDestino() const noexcept;
    void setDestino(Estado* destino);

    // Compatibilidad de lectura con la API anterior: devuelve 0 o 1 elemento.
    std::vector<Estado*> getDestinos() const;
    void setDestinos(const std::vector<Estado*>& destinos);
    void agregarDestino(Estado* destino);
    bool eliminarDestino(Estado* destino);
    bool contieneDestino(const Estado* destino) const noexcept;
    void limpiarDestinos();
    std::size_t getCantidadDestinos() const noexcept;

    bool esEpsilon() const noexcept;
    bool isEpsilon() const noexcept { return esEpsilon(); }
    const std::string& simbolo() const noexcept { return getSimbolo(); }
    std::vector<Estado*> destinos() const { return getDestinos(); }
};
