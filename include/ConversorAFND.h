#pragma once
#include "Automata.h"
class ConversorAFND {
public:
    bool esAFND(const Automata& automata) const noexcept;
    Automata convertirA_AFD(const Automata& afnd) const;
    std::set<Estado*> clausuraEpsilon(const std::set<Estado*>& estados) const;
    std::set<Estado*> mover(const std::set<Estado*>& estados, const std::string& simbolo) const;
    Automata convertir(const Automata& origen,
                       const std::set<std::string>& alfabetoAdicional = {}) const;
};
