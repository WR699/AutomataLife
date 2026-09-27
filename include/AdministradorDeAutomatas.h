#pragma once
#include "Automata.h"
#include "ParserXmlAutomata.h"
#include "ParserJsonAutomata.h"
#include <string>
#include <vector>

class AdministradorDeAutomatas {
private:
    ParserXmlAutomata parserXml_;
    ParserJsonAutomata parserJson_;

    bool esExtensionXml(const std::string& ruta) const {
        return ruta.size() >= 4 && ruta.substr(ruta.size() - 4) == ".xml";
    }

    bool esExtensionJson(const std::string& ruta) const {
        return ruta.size() >= 5 && ruta.substr(ruta.size() - 5) == ".json";
    }

public:
    void guardarAutomata(const Automata& automata, const std::string& ruta) const;
    Automata cargarAutomata(const std::string& ruta) const;

    std::vector<std::string> cargarCadenas(const std::string& ruta) const;
    void guardarCadenas(const std::vector<std::vector<std::string>>& palabras, const std::string& ruta) const;

    void guardar(const Automata& a, const std::string& ruta) const { guardarAutomata(a, ruta); }
    Automata cargar(const std::string& ruta) const { return cargarAutomata(ruta); }
};