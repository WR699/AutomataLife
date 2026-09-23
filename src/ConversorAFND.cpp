#include "ConversorAFND.h"
#include "AlgoritmosInternos.h"
#include <iostream>
#include <cstdio>


Automata ConversorAFND::convertir(const Automata& origen,
    const std::set<std::string>& adicional) const {
    origen.validar();

    // Verificacion de si es AFND o AFD
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
        // El conjunto vacio es exactamente el State Error del AFD.
        if (conjunto.empty()) {
            if (!dfa.getEstadoError()) {
                // El SR se crea automaticamente al agregar el primer estado normal.
                // Este caso solo puede ocurrir despues de registrar el inicial.
                throw std::logic_error("SR aun no disponible al registrar el conjunto vacio");
            }
            return Automata::ID_ESTADO_ERROR;
        }
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
            if (destino != Automata::ID_ESTADO_ERROR)
                dfa.agregarTransicion(nombre, s, { destino });
            // Si el destino es SR no hace falta agregar nada: el completado ya dejo s -> SR.
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
        for (const auto& t : pendientes[i]->getTransiciones()) {
            if (t.esEpsilon()) {
                auto* d = t.getDestino();
                if (!d) throw std::logic_error("Transicion epsilon sin destino");
                if (cierre.insert(d).second) pendientes.push_back(d);
            }
        }
    }
    return cierre;
}

std::set<Estado*> ConversorAFND::mover(const std::set<Estado*>& estados, const std::string& simbolo) const {
    if (simbolo.empty()) throw std::invalid_argument("Usa clausuraEpsilon para epsilon");
    std::set<Estado*> destinos;
    for (const auto* e : estados) {
        if (!e) throw std::invalid_argument("Estado nulo en mover");
        for (const auto& t : e->getTransiciones()) {
            if (t.getSimbolo() == simbolo && t.getDestino() &&
                t.getDestino()->getId() != Automata::ID_ESTADO_ERROR)
                destinos.insert(t.getDestino());
        }
    }
    return destinos;
}