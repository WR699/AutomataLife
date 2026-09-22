#pragma once
#include "Automata.h"
#include "ParserXmlAutomata.h"
#include <string>

class AdministradorDeAutomatas {
private:
    ParserXmlAutomata parserXml_;

    bool esExtensionXml(const std::string& ruta) const {
        return ruta.size() >= 4 && ruta.substr(ruta.size() - 4) == ".xml";
    }

public:
    void guardarAutomata(const Automata& automata, const std::string& ruta) const;
    Automata cargarAutomata(const std::string& ruta) const;

    void guardar(const Automata& a, const std::string& ruta) const { guardarAutomata(a, ruta); }
    Automata cargar(const std::string& ruta) const { return cargarAutomata(ruta); }
};