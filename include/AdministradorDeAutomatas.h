#pragma once
#include "Automata.h"
class AdministradorDeAutomatas {
public:
    void guardarAutomata(const Automata& automata, const std::string& ruta) const;
    Automata cargarAutomata(const std::string& ruta) const;
    void guardar(const Automata& a, const std::string& ruta) const { guardarAutomata(a, ruta); }
    Automata cargar(const std::string& ruta) const { return cargarAutomata(ruta); }
};
