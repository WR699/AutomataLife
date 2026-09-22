#pragma once
#include <string>
#include <vector>
#include <cstddef>
class Estado;
class Automata;
class Transicion {
    friend class Automata;
    friend class Estado;
    std::string simbolo_;
    std::vector<Estado*> destinos_; // Punteros observadores, nunca propietarios.
    Automata* propietario_ = nullptr;
    void comprobar(const std::vector<Estado*>& destinos) const;
public:
    explicit Transicion(std::string simbolo = "");
    Transicion(std::string simbolo, Estado* destino);
    Transicion(std::string simbolo, const std::vector<Estado*>& destinos);
    Transicion(const Transicion& otra); // Copia de valor, inicialmente sin propietario.
    Transicion& operator=(const Transicion& otra); // Conserva y valida el propietario receptor.
    const std::string& getSimbolo() const noexcept;
    void setSimbolo(const std::string& simbolo);
    Estado* getDestino() const; // nullptr si vacia; lanza si hay mas de un destino.
    void setDestino(Estado* destino); // nullptr vacia los destinos.
    const std::vector<Estado*>& getDestinos() const noexcept;
    void setDestinos(const std::vector<Estado*>& destinos);
    void agregarDestino(Estado* destino);
    bool eliminarDestino(Estado* destino);
    bool contieneDestino(const Estado* destino) const noexcept;
    void limpiarDestinos() noexcept;
    std::size_t getCantidadDestinos() const noexcept;
    bool esEpsilon() const noexcept;
    bool isEpsilon() const noexcept { return esEpsilon(); }
    const std::string& simbolo() const noexcept { return getSimbolo(); }
    const std::vector<Estado*>& destinos() const noexcept { return getDestinos(); }
};
