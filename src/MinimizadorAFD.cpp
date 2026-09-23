#include "MinimizadorAFD.h"
#include "ConversorAFND.h"
#include "AlgoritmosInternos.h"
#include <map>
#include <vector>

Automata MinimizadorAFD::minimizar(const Automata& origen) const {
    auto accesible = eliminarEstadosInaccesibles(origen);
    auto dfa = ConversorAFND{}.convertirA_AFD(accesible);
    const auto grupos = particionarEstados(dfa);

    std::map<const Estado*, std::string> nombres;
    Automata minimo;
    minimo.setAlfabeto(dfa.getAlfabeto());

    std::size_t indiceNombre = 0;
    for (const auto& grupo : grupos) {
        // SR se regenera automaticamente. No crear un estado M para el bloque formado solo por SR.
        if (grupo.size() == 1 && dfa.esEstadoError(*grupo.begin())) {
            nombres[*grupo.begin()] = Automata::ID_ESTADO_ERROR;
            continue;
        }

        const auto nombre = "M" + std::to_string(indiceNombre++);
        bool esFinal = false;
        for (const auto* e : grupo) if (e->esEstadoFinal()) { esFinal = true; break; }
        minimo.agregarEstado(nombre, esFinal);
        for (const auto* e : grupo) nombres[e] = nombre;
    }

    const auto* inicial = dfa.getEstadoInicial();
    if (dfa.esEstadoError(inicial))
        throw std::logic_error("SR no puede ser inicial");
    minimo.setEstadoInicial(nombres.at(inicial));

    for (const auto& grupo : grupos) {
        const auto* representante = *grupo.begin();
        if (dfa.esEstadoError(representante)) continue;

        const auto& origenNombre = nombres.at(representante);
        for (const auto& simbolo : dfa.getAlfabeto()) {
            const auto* dest = detalle::siguiente(representante, simbolo);
            if (dfa.esEstadoError(dest)) {
                // No se agrega manualmente: el completado de minimo ya crea -> SR si falta.
                continue;
            }
            minimo.agregarTransicion(origenNombre, simbolo, {nombres.at(dest)});
        }
    }

    minimo.completarEstadoError();
    return minimo;
}

std::vector<std::set<const Estado*>> MinimizadorAFD::particionarEstados(const Automata& a) const {
    a.validar();
    if (!a.esDeterminista())
        throw std::invalid_argument("particionarEstados requiere un AFD");

    std::vector<const Estado*> es;
    std::map<const Estado*, std::size_t> indice;
    for (const auto& e : a.estados()) {
        indice[e.get()] = es.size();
        es.push_back(e.get());
    }

    const auto n = es.size();
    std::vector<std::size_t> grupo(n, 0);
    for (std::size_t i = 0; i < n; ++i)
        grupo[i] = es[i]->esEstadoFinal() ? 1u : 0u;

    auto siguienteIndice = [&](std::size_t i, const std::string& simbolo) {
        return indice.at(detalle::siguiente(es[i], simbolo));
    };

    for (;;) {
        std::map<std::vector<std::size_t>, std::size_t> ids;
        std::vector<std::size_t> nuevo;
        nuevo.reserve(n);

        for (std::size_t i = 0; i < n; ++i) {
            std::vector<std::size_t> firma{grupo[i]};
            for (const auto& simbolo : a.getAlfabeto())
                firma.push_back(grupo[siguienteIndice(i, simbolo)]);
            const auto id = ids.size();
            nuevo.push_back(ids.emplace(firma, id).first->second);
        }

        if (nuevo == grupo) break;
        grupo = std::move(nuevo);
    }

    std::map<std::size_t, std::set<const Estado*>> bloques;
    for (std::size_t i = 0; i < n; ++i) bloques[grupo[i]].insert(es[i]);

    std::vector<std::set<const Estado*>> resultado;
    for (auto& b : bloques) resultado.push_back(std::move(b.second));
    return resultado;
}

std::vector<std::set<Estado*>> MinimizadorAFD::particionarEstados(Automata& a) const {
    const auto bloques = particionarEstados(static_cast<const Automata&>(a));
    std::vector<std::set<Estado*>> resultado;
    for (const auto& b : bloques) {
        std::set<Estado*> grupo;
        for (const auto* e : b) grupo.insert(a.buscarEstado(e->getId()));
        resultado.push_back(std::move(grupo));
    }
    return resultado;
}

Automata MinimizadorAFD::eliminarEstadosInaccesibles(const Automata& a) const {
    a.validar();

    std::set<const Estado*> visitados{a.getEstadoInicial()};
    std::vector<const Estado*> cola{a.getEstadoInicial()};

    for (std::size_t i = 0; i < cola.size(); ++i) {
        for (const auto& t : cola[i]->getTransiciones()) {
            const auto* d = t.getDestino();
            if (d && visitados.insert(d).second) cola.push_back(d);
        }
    }

    Automata resultado;
    resultado.setAlfabeto(a.getAlfabeto());

    for (const auto& e : a.estados()) {
        if (a.esEstadoError(e.get())) continue; // SR se regenera
        if (visitados.count(e.get()))
            resultado.agregarEstado(e->getId(), e->esEstadoFinal());
    }

    resultado.setEstadoInicial(a.getEstadoInicial()->getId());
    const auto* sr = a.getEstadoError();

    for (const auto& e : a.estados()) {
        if (a.esEstadoError(e.get()) || !visitados.count(e.get())) continue;
        for (const auto& t : e->getTransiciones()) {
            const auto* d = t.getDestino();
            if (!d || d == sr || !visitados.count(d)) continue;
            resultado.agregarTransicion(e->getId(), t.getSimbolo(), {d->getId()});
        }
    }

    resultado.completarEstadoError();
    return resultado;
}
