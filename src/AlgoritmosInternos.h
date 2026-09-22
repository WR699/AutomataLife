#pragma once
#include "Automata.h"
#include <map>
#include <stdexcept>
namespace detalle {
using Conjunto = std::set<std::string>;
inline Conjunto cierre(const Automata& a, Conjunto estados) {
    std::vector<std::string> pendientes(estados.begin(), estados.end());
    for (std::size_t i = 0; i < pendientes.size(); ++i) {
        const auto* e = a.buscarEstado(pendientes[i]);
        if (!e) throw std::logic_error("Estado desconocido en cierre");
        for (const auto& t : e->transiciones()) if (t.esEpsilon())
            for (const auto* d : t.destinos())
                if (estados.insert(d->nombre()).second) pendientes.push_back(d->nombre());
    }
    return estados;
}
inline Conjunto mover(const Automata& a, const Conjunto& estados, const std::string& simbolo) {
    Conjunto destinos;
    for (const auto& n : estados)
        for (const auto& t : a.buscarEstado(n)->transiciones())
            if (t.simbolo() == simbolo)
                for (const auto* d : t.destinos()) destinos.insert(d->nombre());
    return cierre(a, std::move(destinos));
}
inline bool final(const Automata& a, const Conjunto& estados) {
    for (const auto& n : estados) if (a.buscarEstado(n)->esFinal()) return true;
    return false;
}
inline const Estado* siguiente(const Estado* e, const std::string& s) {
    for (const auto& t : e->transiciones())
        if (t.simbolo() == s && !t.destinos().empty()) return t.destinos().front();
    throw std::logic_error("Se esperaba un AFD completo");
}
}
