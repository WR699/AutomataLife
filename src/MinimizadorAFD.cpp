#include "MinimizadorAFD.h"
#include "ConversorAFND.h"
#include "AlgoritmosInternos.h"
#include <map>
#include <vector>

Automata MinimizadorAFD::minimizar(const Automata& origen) const {
    auto accesible = eliminarEstadosInaccesibles(origen);
    auto dfa = ConversorAFND{}.convertirA_AFD(accesible);

    // CODIGO VIEJO: se particionaba inmediatamente despues de convertir.
    // const auto grupos = particionarEstados(dfa);

    // La determinizacion puede volver inaccesibles estados que antes eran alcanzables
    // por transiciones multiples. Quitarlos ahora evita procesarlos en las particiones.
    dfa = eliminarEstadosInaccesibles(dfa);
    const auto grupos = particionarEstados(dfa);

    std::map<const Estado*, std::string> nombres;
    Automata minimo;
    minimo.setAlfabeto(dfa.getAlfabeto());

    const auto* inicial = dfa.getEstadoInicial();
    std::size_t indiceNombre = 0;
    for (const auto& grupo : grupos) {
        // CODIGO VIEJO: SR solo prevalecia si era el unico estado del grupo.
        /*
        if (grupo.size() == 1 && dfa.esEstadoError(*grupo.begin())) {
            nombres[*grupo.begin()] = Automata::ID_ESTADO_ERROR;
            continue;
        }
        */

        bool contieneSR = false;
        bool contieneInicial = false;
        for (const auto* e : grupo) {
            if (dfa.esEstadoError(e)) {contieneSR = true;}
            if (e == inicial) {contieneInicial = true;}
        }

        // Si SR es equivalente a otros estados, SR prevalece y representa a todo el grupo.
        // Excepcion: SR nunca puede ser inicial. Si el inicial es equivalente a SR,
        // necesitamos conservar un estado de usuario como representante del grupo.
        if (contieneSR && !contieneInicial) {
            for (const auto* e : grupo) nombres[e] = Automata::ID_ESTADO_ERROR;
            continue;
        }

        const auto nombre = "M" + std::to_string(indiceNombre++);
        bool esFinal = false;
        for (const auto* e : grupo) if (e->esEstadoFinal()) {esFinal = true; break;}
        minimo.agregarEstado(nombre, esFinal);
        for (const auto* e : grupo) nombres[e] = nombre;
    }

    if (dfa.esEstadoError(inicial))
        throw std::logic_error("SR no puede ser inicial");
    minimo.setEstadoInicial(nombres.at(inicial));

    for (const auto& grupo : grupos) {
        const auto* representante = *grupo.begin();
        const auto& origenNombre = nombres.at(representante);

        // Si todo el grupo fue absorbido por SR, sus bucles los administra SR internamente.
        if (origenNombre == Automata::ID_ESTADO_ERROR) {continue;}

        for (const auto& simbolo : dfa.getAlfabeto()) {
            const auto* dest = detalle::siguiente(representante, simbolo);
            const auto& destinoNombre = nombres.at(dest);

            // Si el grupo destino fue absorbido por SR, completarEstadoError() agrega el fallback.
            if (destinoNombre == Automata::ID_ESTADO_ERROR) {continue;}

            minimo.agregarTransicion(origenNombre, simbolo, {destinoNombre});
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
        if (visitados.count(e.get())) {
            // CODIGO VIEJO: se copiaba solo el ID interno.
            // resultado.agregarEstado(e->getId(), e->esEstadoFinal());
            auto& copiaEstado = resultado.agregarEstado(e->getId(), e->esEstadoFinal());
            copiaEstado.setNombreVisible(e->getNombreVisible());
        }
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
