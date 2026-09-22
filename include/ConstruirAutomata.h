#pragma once
#include "Automata.h"
class ConstruirAutomata {
public:
    Automata crear() const { return Automata{}; }
    Estado& agregarEstado(Automata& a, const std::string& n, bool final = false) const;
    void establecerInicial(Automata& a, const std::string& n) const;
    void establecerFinal(Automata& a, const std::string& n, bool final) const;
    void agregarTransicion(Automata& a, const std::string& origen,
                           const std::string& simbolo, const std::vector<std::string>& destinos) const;
    Automata ejemplo() const;
};
