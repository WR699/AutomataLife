#pragma once
#include "Estado.h"
#include "TipoAutomata.h"
#include <memory>
#include <set>
#include <string>
#include <vector>

class Automata {
    friend class Estado;
    friend class Transicion;

    std::vector<std::unique_ptr<Estado>> estados_;
    Estado* inicial_ = nullptr;
    std::set<std::string> alfabetoDeclarado_;

    void vincular() noexcept;
    Estado& asegurarEstadoError();
    Transicion& agregarAristaInterna(Estado& origen, const std::string& simbolo, Estado* destino);

public:
    static constexpr const char* ID_ESTADO_ERROR = "SR";

    Automata() = default;
    Automata(const Automata&) = delete;
    Automata& operator=(const Automata&) = delete;
    Automata(Automata&& otro) noexcept;
    Automata& operator=(Automata&& otro) noexcept;

    Estado& agregarEstado(const std::string& id, bool final = false);
    Estado& agregarEstado(std::unique_ptr<Estado> estado);
    Estado* buscarEstado(const std::string& id);
    const Estado* buscarEstado(const std::string& id) const;
    std::set<Estado*> getEstados();
    std::set<const Estado*> getEstados() const;
    void setEstados(std::vector<std::unique_ptr<Estado>> estados);
    bool contieneEstado(const Estado* estado) const noexcept;
    bool eliminarEstado(const std::string& id);
    std::size_t getCantidadEstados() const noexcept;

    Estado* getEstadoInicial() noexcept;
    const Estado* getEstadoInicial() const noexcept;
    void setEstadoInicial(Estado* estado);
    void setEstadoInicial(const std::string& id);

    std::set<std::string> getAlfabeto() const;
    void setAlfabeto(const std::set<std::string>& alfabeto);
    void agregarSimbolo(const std::string& simbolo);
    bool eliminarSimbolo(const std::string& simbolo);

    // Materializa el estado sumidero SR y completa todo simbolo faltante con -> SR.
    // Tambien elimina fallbacks -> SR cuando ya existe una transicion real para ese simbolo.
    void completarEstadoError();
    Estado* getEstadoError() noexcept { return buscarEstado(ID_ESTADO_ERROR); }
    const Estado* getEstadoError() const noexcept { return buscarEstado(ID_ESTADO_ERROR); }
    bool esEstadoError(const Estado* estado) const noexcept;

    TipoAutomata getTipo() const;
    void setTipo(TipoAutomata tipo);

    // Con el automata completo, por palomar:
    // mas transiciones que simbolos => algun simbolo se repite o existe epsilon => AFND.
    bool esDeterminista() const;

    bool validarCadena(const std::string& cadena) const;
    bool validarCadena(const std::vector<std::string>& simbolos) const;
    void limpiar() noexcept;
    void validar() const;
    Automata clonar() const;

    const std::vector<std::unique_ptr<Estado>>& estados() const { return estados_; }
    const Estado* inicial() const { return getEstadoInicial(); }
    void establecerInicial(const std::string& id) { setEstadoInicial(id); }
    void establecerFinal(const std::string& id, bool final);

    // Cada destino de la lista se convierte en una Transicion independiente.
    void agregarTransicion(const std::string& origen, const std::string& simbolo,
                           const std::vector<std::string>& destinos);

    std::set<std::string> alfabeto() const { return getAlfabeto(); }
    bool esDeterministico() const { return esDeterminista(); }
    bool acepta(const std::vector<std::string>& palabra) const;
};
