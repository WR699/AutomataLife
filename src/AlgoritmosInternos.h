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

        for (const auto& t : e->transiciones()) {
            if (t.esEpsilon()) {
                const auto* d = t.getDestino();
                if (!d) throw std::logic_error("Transicion epsilon sin destino");
                if (estados.insert(d->nombre()).second) pendientes.push_back(d->nombre());
            }
        }
    }
    return estados;
}

inline Conjunto mover(const Automata& a, const Conjunto& estados, const std::string& simbolo) {
    Conjunto destinos;
    for (const auto& n : estados) {
        const auto* e = a.buscarEstado(n);
        if (!e) throw std::logic_error("Estado desconocido en mover");
        for (const auto& t : e->transiciones()) {
            if (t.simbolo() == simbolo && t.getDestino() && !a.esEstadoError(t.getDestino()))
                destinos.insert(t.getDestino()->nombre());
        }
    }
    return cierre(a, std::move(destinos));
}

inline bool final(const Automata& a, const Conjunto& estados) {
    for (const auto& n : estados)
        if (a.buscarEstado(n)->esFinal()) return true;
    return false;
}

inline const Estado* siguiente(const Estado* e, const std::string& s) {
    const Estado* encontrado = nullptr;
    for (const auto& t : e->transiciones()) {
        if (t.simbolo() != s) continue;
        if (!t.getDestino()) throw std::logic_error("Transicion sin destino");
        if (encontrado && encontrado != t.getDestino())
            throw std::logic_error("Se esperaba un AFD: hay mas de un destino para el simbolo");
        encontrado = t.getDestino();
    }
    if (!encontrado) throw std::logic_error("Se esperaba un AFD completo");
    return encontrado;
}
}
