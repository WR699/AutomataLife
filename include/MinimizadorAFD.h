#pragma once
#include "Automata.h"
class MinimizadorAFD {
public:
    Automata minimizar(const Automata& afd) const;
    // Particion estable final. Incluye todos los estados del AFD recibido.
    // Punteros observadores al argumento, no a una copia temporal.
    std::vector<std::set<Estado*>> particionarEstados(Automata& afd) const;
    std::vector<std::set<const Estado*>> particionarEstados(const Automata& afd) const;
    Automata eliminarEstadosInaccesibles(const Automata& automata) const;
};
