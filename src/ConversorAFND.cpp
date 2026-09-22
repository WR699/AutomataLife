#include "ConversorAFND.h"
#include "AlgoritmosInternos.h"
#include <iostream>
#include <cstdio>

Automata ConversorAFND::convertir(const Automata& origen,
    const std::set<std::string>& adicional) const {
    origen.validar();

    // Verificación de si es AFND o AFD
    bool esAFND = !origen.esDeterminista();
    if (esAFND) {
        std::cout << "[ConversorAFND] El automata de entrada ES un AFND.\n";
    }
    else {
        std::cout << "[ConversorAFND] El automata de entrada NO es un AFND (ya es un AFD).\n";
    }

    auto alfabeto = origen.getAlfabeto();
    alfabeto.insert(adicional.begin(), adicional.end());
    alfabeto.erase("");
    Automata dfa;
    dfa.setAlfabeto(alfabeto);
    std::map<detalle::Conjunto, std::string> nombres;
    std::vector<detalle::Conjunto> pendientes;
    auto registrar = [&](const detalle::Conjunto& conjunto) -> std::string {
        auto it = nombres.find(conjunto);
        if (it != nombres.end()) return it->second;
        std::string nombre = "D" + std::to_string(nombres.size());
        nombres.emplace(conjunto, nombre);
        pendientes.push_back(conjunto);
        dfa.agregarEstado(nombre, detalle::final(origen, conjunto));
        return nombre;
    };
    dfa.setEstadoInicial(registrar(detalle::cierre(origen, { origen.getEstadoInicial()->getId() })));
    for (std::size_t i = 0; i < pendientes.size(); ++i) {
        // Copia: registrar puede realocar pendientes.
        const auto conjunto = pendientes[i];
        const auto nombre = nombres.at(conjunto);
        for (const auto& s : alfabeto) {
            const auto destino = registrar(detalle::mover(origen, conjunto, s));
            dfa.agregarTransicion(nombre, s, { destino });
        }
    }
    return dfa;
}

Automata ConversorAFND::convertirA_AFD(const Automata& a) const { return convertir(a); }

std::set<Estado*> ConversorAFND::clausuraEpsilon(const std::set<Estado*>& estados) const {
    auto cierre = estados;
    std::vector<Estado*> pendientes(estados.begin(), estados.end());
    for (std::size_t i = 0; i < pendientes.size(); ++i) {
        if (!pendientes[i]) throw std::invalid_argument("Estado nulo en clausura");
        for (const auto& t : pendientes[i]->getTransiciones()) if (t.esEpsilon())
            for (auto* d : t.getDestinos()) if (cierre.insert(d).second) pendientes.push_back(d);
    }
    return cierre;
}

std::set<Estado*> ConversorAFND::mover(const std::set<Estado*>& estados, const std::string& simbolo) const {
    if (simbolo.empty()) throw std::invalid_argument("Usa clausuraEpsilon para epsilon");
    std::set<Estado*> destinos;
    for (const auto* e : estados) {
        if (!e) throw std::invalid_argument("Estado nulo en mover");
        for (const auto& t : e->getTransiciones()) if (t.getSimbolo() == simbolo)
            destinos.insert(t.getDestinos().begin(), t.getDestinos().end());
    }
    return destinos;
}