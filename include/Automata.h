#pragma once
#include "Estado.h"
#include "TipoAutomata.h"
#include <memory>
#include <set>
class Automata {
    std::vector<std::unique_ptr<Estado>> estados_;
    Estado* inicial_ = nullptr;
    std::set<std::string> alfabetoDeclarado_;
    void vincular() noexcept;
public:
    Automata() = default;
    Automata(const Automata&) = delete;
    Automata& operator=(const Automata&) = delete;
    Automata(Automata&& otro) noexcept;
    Automata& operator=(Automata&& otro) noexcept;
    Estado& agregarEstado(const std::string& id, bool final = false);
    Estado& agregarEstado(std::unique_ptr<Estado> estado); // Transfiere propiedad.
    Estado* buscarEstado(const std::string& id);
    const Estado* buscarEstado(const std::string& id) const;
    std::set<Estado*> getEstados();
    std::set<const Estado*> getEstados() const;
    void setEstados(std::vector<std::unique_ptr<Estado>> estados);
    bool contieneEstado(const Estado* estado) const noexcept;
    bool eliminarEstado(const std::string& id); // Quita tambien los punteros entrantes.
    std::size_t getCantidadEstados() const noexcept;
    Estado* getEstadoInicial() noexcept;
    const Estado* getEstadoInicial() const noexcept;
    void setEstadoInicial(Estado* estado); // nullptr permite construir sin inicial.
    void setEstadoInicial(const std::string& id);
    std::set<std::string> getAlfabeto() const;
    void setAlfabeto(const std::set<std::string>& alfabeto);
    void agregarSimbolo(const std::string& simbolo);
    bool eliminarSimbolo(const std::string& simbolo);
    TipoAutomata getTipo() const;
    // El tipo es calculado: solo acepta una clasificacion consistente.
    void setTipo(TipoAutomata tipo);
    bool esDeterminista() const;
    bool validarCadena(const std::string& cadena) const;
    bool validarCadena(const std::vector<std::string>& simbolos) const;
    void limpiar() noexcept;
    void validar() const;
    Automata clonar() const;
    // Compatibilidad con la primera version.
    const std::vector<std::unique_ptr<Estado>>& estados() const { return estados_; }
    const Estado* inicial() const { return getEstadoInicial(); }
    void establecerInicial(const std::string& id) { setEstadoInicial(id); }
    void establecerFinal(const std::string& id, bool final);
    void agregarTransicion(const std::string& origen, const std::string& simbolo,
                           const std::vector<std::string>& destinos);
    std::set<std::string> alfabeto() const { return getAlfabeto(); }
    bool esDeterministico() const { return esDeterminista(); }
    bool acepta(const std::vector<std::string>& palabra) const;
};
