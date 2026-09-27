#pragma once
#include "Automata.h"
#include <map>
class ConversorAFND {
public:
    Automata convertirA_AFD(const Automata& afnd,
                          const std::set<std::string>& alfabetoAdicional = {}) const;
    void quitarTransicionesEpsilon(Automata& automata) const;
    // CODIGO VIEJO: los conjuntos compuestos se identificaban concatenando IDs con "+".
    // std::map<std::string, std::string> agregarEstadosCompuestos(Automata& automata) const;
    // void reemplazarTransicionesCompuestas(Automata& automata, const std::map<std::string, std::string>& nuevosEstados) const;
    std::map<std::set<std::string>, std::string> agregarEstadosCompuestos(Automata& automata) const;
    void reemplazarTransicionesCompuestas(Automata& automata, const std::map<std::set<std::string>, std::string>& nuevosEstados) const;
    std::set<Estado*> clausuraEpsilon(const std::set<Estado*>& estados) const;
    // CODIGO VIEJO: mover pertenecia al conversor. Ahora es una operacion de Automata.
    // std::set<Estado*> mover(const std::set<Estado*>& estados, const std::string& simbolo) const;
    // CODIGO VIEJO: convertir() era otra entrada que terminaba haciendo lo mismo.
    // Ahora toda la conversion vive directamente en convertirA_AFD().
    /*
    Automata convertir(const Automata& origen,
                       const std::set<std::string>& alfabetoAdicional = {}) const;
    */
};
