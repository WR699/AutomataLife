#pragma once
#include "Automata.h"
#include <string>

class ParserXmlAutomata {
public:
    // Lee un archivo XML y construye un objeto Automata (AFND o AFD)
    Automata cargarDesdeXML(const std::string& rutaArchivo) const;

    // Guarda un Automata en un archivo XML
    void guardarEnXML(const Automata& automata, const std::string& rutaArchivo) const;
};