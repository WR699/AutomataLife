#pragma once
#include "Automata.h"
class ConversorAFND {
public:
    Automata convertirA_AFD(const Automata& afnd) const;
    std::set<Estado*> clausuraEpsilon(const std::set<Estado*>& estados) const;
    std::set<Estado*> mover(const std::set<Estado*>& estados, const std::string& simbolo) const;
    Automata convertir(const Automata& origen,
                       const std::set<std::string>& alfabetoAdicional = {}) const;
};
