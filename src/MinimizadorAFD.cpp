#include "MinimizadorAFD.h"
#include "ConversorAFND.h"
#include "AlgoritmosInternos.h"
Automata MinimizadorAFD::minimizar(const Automata& origen) const {
    auto accesible = eliminarEstadosInaccesibles(origen);
    auto dfa = ConversorAFND{}.convertirA_AFD(accesible);
    const auto grupos = particionarEstados(dfa);
    std::map<const Estado*, std::string> nombres;
    Automata minimo;
    minimo.setAlfabeto(dfa.getAlfabeto());
    for (std::size_t i = 0; i < grupos.size(); ++i) {
        const auto nombre = "M" + std::to_string(i);
        minimo.agregarEstado(nombre, (*grupos[i].begin())->esEstadoFinal());
        for (const auto* e : grupos[i]) nombres[e] = nombre;
    }
    minimo.setEstadoInicial(nombres.at(dfa.getEstadoInicial()));
    for (const auto& grupo : grupos) {
        const auto* representante = *grupo.begin();
        for (const auto& simbolo : dfa.getAlfabeto())
            minimo.agregarTransicion(nombres.at(representante), simbolo,
                {nombres.at(detalle::siguiente(representante, simbolo))});
    }
    return minimo;
}

std::vector<std::set<const Estado*>> MinimizadorAFD::particionarEstados(const Automata& a) const {
    a.validar();
    if (!a.esDeterminista()) throw std::invalid_argument("particionarEstados requiere un AFD");
    std::vector<const Estado*> es;
    std::map<const Estado*, std::size_t> indice;
    for (const auto& e : a.estados()) { indice[e.get()] = es.size(); es.push_back(e.get()); }
    const auto n = es.size(); // n representa el sumidero virtual de un AFD parcial.
    std::vector<std::size_t> grupo(n + 1, 0);
    for (std::size_t i = 0; i < n; ++i) grupo[i] = es[i]->esEstadoFinal() ? 1 : 0;
    auto siguiente = [&](std::size_t i, const std::string& simbolo) {
        if (i == n) return n;
        for (const auto& t : es[i]->getTransiciones())
            if (t.getSimbolo() == simbolo && !t.getDestinos().empty()) return indice.at(t.getDestinos().front());
        return n;
    };
    for (;;) {
        std::map<std::vector<std::size_t>, std::size_t> ids;
        std::vector<std::size_t> nuevo;
        for (std::size_t i = 0; i <= n; ++i) {
            std::vector<std::size_t> firma{grupo[i]};
            for (const auto& simbolo : a.getAlfabeto()) firma.push_back(grupo[siguiente(i, simbolo)]);
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
    for (std::size_t i = 0; i < cola.size(); ++i)
        for (const auto& t : cola[i]->getTransiciones()) for (const auto* d : t.getDestinos())
            if (visitados.insert(d).second) cola.push_back(d);
    Automata resultado; resultado.setAlfabeto(a.getAlfabeto());
    for (const auto& e : a.estados()) if (visitados.count(e.get())) resultado.agregarEstado(e->getId(), e->esEstadoFinal());
    resultado.setEstadoInicial(a.getEstadoInicial()->getId());
    for (const auto& e : a.estados()) if (visitados.count(e.get())) for (const auto& t : e->getTransiciones()) {
        std::vector<std::string> ds;
        for (const auto* d : t.getDestinos()) ds.push_back(d->getId());
        resultado.agregarTransicion(e->getId(), t.getSimbolo(), ds);
    }
    return resultado;
}
