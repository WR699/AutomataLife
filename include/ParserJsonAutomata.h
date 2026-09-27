#pragma once
#include "Automata.h"
#include <string>

class ParserJsonAutomata {
public:
    Automata cargarDesdeJSON(const std::string& ruta) const;
    void guardarEnJSON(const Automata& automata, const std::string& ruta) const;
};