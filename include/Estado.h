#pragma once
#include "Transicion.h"
#include <cstddef>
#include <string>
#include <vector>

class Automata;

class Estado {
    friend class Automata;

    std::string nombre_;
    bool inicial_ = false;
    bool final_ = false;
    std::vector<Transicion> transiciones_;
    Automata* propietario_ = nullptr;

public:
    explicit Estado(std::string id, bool final = false);
    Estado(std::string id, bool inicial, bool final);
    Estado(const Estado&) = delete;
    Estado& operator=(const Estado&) = delete;

    const std::string& getId() const noexcept;
    void setId(const std::string& id);

    bool esEstadoInicial() const noexcept;
    bool isEstadoInicial() const noexcept { return esEstadoInicial(); }
    void setEstadoInicial(bool inicial);

    bool esEstadoFinal() const noexcept;
    bool isEstadoFinal() const noexcept { return esEstadoFinal(); }
    void setEstadoFinal(bool final);

    const std::vector<Transicion>& getTransiciones() const noexcept;
    void setTransiciones(const std::vector<Transicion>& transiciones);
    Transicion& agregarTransicion(const Transicion& transicion);
    Transicion& getTransicion(std::size_t indice);
    const Transicion& getTransicion(std::size_t indice) const;
    void eliminarTransicion(std::size_t indice);
    void limpiarTransiciones();
    std::size_t getCantidadTransiciones() const noexcept;

    void setEsInicial(bool v) { setEstadoInicial(v); }
    void setEsFinal(bool v) { setEstadoFinal(v); }
    const std::string& nombre() const noexcept { return getId(); }
    bool esFinal() const noexcept { return esEstadoFinal(); }
    const std::vector<Transicion>& transiciones() const noexcept { return getTransiciones(); }
};
