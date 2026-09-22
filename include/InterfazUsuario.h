#pragma once
#include "ConstruirAutomata.h"
#include "ConversorAFND.h"
#include "AdministradorDeAutomatas.h"
#include "MinimizadorAFD.h"
#include "TesterEquivalencia.h"
#include <iosfwd>
#include <utility>
class InterfazUsuario {
    ConstruirAutomata constructor_;
    ConversorAFND conversor_;
    AdministradorDeAutomatas archivos_;
    MinimizadorAFD minimizador_;
    TesterEquivalencia equivalencia_;
    Automata actual_;
    bool salir_ = false;
public:
    InterfazUsuario();
    void iniciar();
    void mostrarMenu() const;
    void mostrarMenu(std::ostream& salida) const;
    void procesarOpcion(int opcion);
    void procesarOpcion(int opcion, std::istream& entrada, std::ostream& salida);
    void ejecutar(std::istream& entrada, std::ostream& salida);
    void mostrar(std::ostream& salida) const;
    Automata& getAutomata() noexcept { return actual_; }
    const Automata& getAutomata() const noexcept { return actual_; }
    void setAutomata(Automata automata) { actual_ = std::move(automata); }
};
