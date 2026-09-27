#include "TesterEquivalencia.h"
#include "ConversorAFND.h"
#include "AlgoritmosInternos.h"
#include <algorithm>
#include <iostream>
#include <map>

namespace {
Automata renombrarAlfabeto(const Automata& origen, const std::set<std::string>& alfabetoNuevo) {
    const auto alfabetoViejo = origen.getAlfabeto();
    if (alfabetoViejo.size() != alfabetoNuevo.size()) {
        throw std::invalid_argument("Los alfabetos deben tener el mismo tamano para renombrarlos");
    }

    std::map<std::string, std::string> cambio;
    auto viejo = alfabetoViejo.begin();
    auto nuevo = alfabetoNuevo.begin();

    while (viejo != alfabetoViejo.end()) {
        cambio[*viejo] = *nuevo;
        ++viejo;
        ++nuevo;
    }

    Automata resultado;

    for (const Estado* estado : origen.getEstados()) {
        if (origen.esEstadoError(estado)) {continue;}
        resultado.agregarEstado(estado->getId(), estado->esEstadoFinal());
    }

    resultado.setEstadoInicial(origen.getEstadoInicial()->getId());
    resultado.setAlfabeto(alfabetoNuevo);

    for (const Estado* estado : origen.getEstados()) {
        if (origen.esEstadoError(estado)) {continue;}

        for (const auto& transicion : estado->getTransiciones()) {
            Estado* destino = transicion.getDestino();
            if (destino == nullptr) {throw std::logic_error("Transicion sin destino al renombrar alfabeto");}

            std::string simbolo = transicion.getSimbolo();
            if (!transicion.esEpsilon()) {simbolo = cambio.at(simbolo);}

            resultado.agregarTransicion(estado->getId(), simbolo, {destino->getId()});
        }
    }

    return resultado;
}
}

// CODIGO VIEJO: la comparacion siempre escribia avisos por consola.
/*
bool TesterEquivalencia::compararAFDPorIteraciones(const Automata& primero, const Automata& segundo) const;
*/
bool TesterEquivalencia::compararAFDPorIteraciones(const Automata& primero, const Automata& segundo,
                                                    bool mostrarMensajes) const {
    primero.validar();
    segundo.validar();

    Automata a = primero.clonar();
    Automata b = segundo.clonar();

    const auto alfabetoPrimero = a.getAlfabeto();
    const auto alfabetoSegundo = b.getAlfabeto();

    // CODIGO VIEJO: exigia que ambos alfabetos fueran exactamente iguales.
    /*
    const auto alfabeto = primero.getAlfabeto();
    if (alfabeto != segundo.getAlfabeto()) {
        throw std::invalid_argument("Los AFD deben tener el mismo alfabeto");
    }
    */

    if (alfabetoPrimero.size() != alfabetoSegundo.size()) {
        throw std::invalid_argument("Los automatas tienen alfabetos de distinto tamano");
    }

    const bool alfabetosDistintos = alfabetoPrimero != alfabetoSegundo;

    if (alfabetosDistintos) {
        if (mostrarMensajes) {
            std::cout << "Los alfabetos tienen el mismo tamano pero son distintos. "
                      << "Se cambiara el alfabeto del segundo para usar el del primero.\n";

            auto viejo = alfabetoSegundo.begin();
            auto nuevo = alfabetoPrimero.begin();
            while (viejo != alfabetoSegundo.end()) {
                std::cout << "  " << *viejo << " -> " << *nuevo << '\n';
                ++viejo;
                ++nuevo;
            }
        }

        b = renombrarAlfabeto(b, alfabetoPrimero);
    }

    // CODIGO VIEJO: si alguno era AFND, el tester terminaba con una excepcion.
    /*
    if (!primero.esDeterminista() || !segundo.esDeterminista()) {
        throw std::invalid_argument("compararAFDPorIteraciones requiere dos AFD");
    }
    */

    const bool primeroNoDeterminista = !a.esDeterminista();
    const bool segundoNoDeterminista = !b.esDeterminista();

    if (primeroNoDeterminista || segundoNoDeterminista) {
        if (mostrarMensajes) {
            std::cout << "Como el/los automatas no son deterministas de base, "
                      << "los convertiremos a deterministas nosotros.\n";
        }

        ConversorAFND conversor;
        if (primeroNoDeterminista) {a = conversor.convertirA_AFD(a, alfabetoPrimero);}
        if (segundoNoDeterminista) {b = conversor.convertirA_AFD(b, alfabetoPrimero);}
    }

    const auto alfabeto = a.getAlfabeto();
    using ParEstados = std::pair<std::string, std::string>;

    std::set<ParEstados> paresActuales;
    std::set<ParEstados> paresVisitados;

    ParEstados inicial{a.getEstadoInicial()->getId(), b.getEstadoInicial()->getId()};
    paresActuales.insert(inicial);
    paresVisitados.insert(inicial);

    while (!paresActuales.empty()) {
        std::set<ParEstados> paresSiguientes;

        for (const auto& par : paresActuales) {
            const Estado* estadoPrimero = a.buscarEstado(par.first);
            const Estado* estadoSegundo = b.buscarEstado(par.second);

            if (estadoPrimero == nullptr || estadoSegundo == nullptr) {
                throw std::logic_error("Estado inexistente durante comparacion por iteraciones");
            }

            if (estadoPrimero->esEstadoFinal() != estadoSegundo->esEstadoFinal()) {return false;}

            for (const auto& simbolo : alfabeto) {
                const Estado* destinoPrimero = detalle::siguiente(estadoPrimero, simbolo);
                const Estado* destinoSegundo = detalle::siguiente(estadoSegundo, simbolo);

                if (destinoPrimero == nullptr || destinoSegundo == nullptr) {
                    throw std::logic_error("Falta una transicion durante comparacion por iteraciones");
                }

                ParEstados siguiente{destinoPrimero->getId(), destinoSegundo->getId()};

                // Si el par ya estaba visitado, este camino llego a una situacion conocida.
                // No repetimos ese par, pero seguimos revisando los otros pares nuevos.
                if (paresVisitados.insert(siguiente).second) {paresSiguientes.insert(siguiente);}
            }
        }

        paresActuales = std::move(paresSiguientes);
    }

    if (alfabetosDistintos && mostrarMensajes) {
        std::cout << "Son equivalentes en transiciones, pero para alfabetos distintos.\n";
    }

    return true;
}

ResultadoEquivalencia TesterEquivalencia::comparar(const Automata& primero,
                                                      const Automata& segundo) const {
    auto alfabeto = primero.alfabeto();
    auto otro = segundo.alfabeto();
    alfabeto.insert(otro.begin(), otro.end());
    auto a = ConversorAFND{}.convertirA_AFD(primero, alfabeto);
    auto b = ConversorAFND{}.convertirA_AFD(segundo, alfabeto);
    using Par = std::pair<std::string, std::string>;
    struct Paso { Par par; std::size_t padre; std::string simbolo; };
    std::vector<Paso> cola{{{a.inicial()->nombre(), b.inicial()->nombre()}, 0, ""}};
    std::set<Par> visitados{cola.front().par};
    ResultadoEquivalencia resultado;
    for (std::size_t i = 0; i < cola.size(); ++i) {
        const auto par = cola[i].par;
        const auto* x = a.buscarEstado(par.first);
        const auto* y = b.buscarEstado(par.second);
        ++resultado.paresExplorados;
        if (x->esFinal() != y->esFinal()) {
            resultado.equivalentes = false;
            for (auto j = i; j != 0; j = cola[j].padre)
                resultado.contraejemplo.push_back(cola[j].simbolo);
            std::reverse(resultado.contraejemplo.begin(), resultado.contraejemplo.end());
            return resultado;
        }
        for (const auto& s : alfabeto) {
            Par siguiente{detalle::siguiente(x, s)->nombre(), detalle::siguiente(y, s)->nombre()};
            if (visitados.insert(siguiente).second) cola.push_back({siguiente, i, s});
        }
    }
    return resultado;
}

bool TesterEquivalencia::probarEquivalencia(const Automata& a, const Automata& b,
                                           const std::vector<std::string>& cadenas) const {
    a.validar(); b.validar();
    for (const auto& cadena : cadenas) if (a.validarCadena(cadena) != b.validarCadena(cadena)) return false;
    return true;
}
std::vector<std::vector<std::string>> TesterEquivalencia::generarPalabrasDePrueba(
    const std::set<std::string>& alfabeto, int maxima) const {
    if (maxima < 0) throw std::invalid_argument("Longitud negativa");
    if (alfabeto.count("")) throw std::invalid_argument("Epsilon no pertenece al alfabeto");
    std::vector<std::vector<std::string>> todas(1), nivel(1);
    for (int longitud = 0; longitud < maxima && !alfabeto.empty(); ++longitud) {
        std::vector<std::vector<std::string>> siguiente;
        for (const auto& palabra : nivel) for (const auto& simbolo : alfabeto) {
            auto nueva = palabra; nueva.push_back(simbolo); siguiente.push_back(std::move(nueva));
        }
        todas.insert(todas.end(), siguiente.begin(), siguiente.end());
        nivel = std::move(siguiente);
    }
    return todas;
}
std::vector<std::string> TesterEquivalencia::generarCadenasDePrueba(
    const std::set<std::string>& alfabeto, int maxima) const {
    for (const auto& s : alfabeto) if (s.size() != 1)
        throw std::invalid_argument("Para simbolos multibyte usa generarPalabrasDePrueba");
    std::vector<std::string> cadenas;
    for (const auto& palabra : generarPalabrasDePrueba(alfabeto, maxima)) {
        std::string cadena; for (const auto& simbolo : palabra) cadena += simbolo;
        cadenas.push_back(std::move(cadena));
    }
    return cadenas;
}
bool TesterEquivalencia::probarEquivalenciaPorSimbolos(const Automata& a, const Automata& b,
    const std::vector<std::vector<std::string>>& palabras) const {
    a.validar(); b.validar();
    for (const auto& p : palabras) if (a.validarCadena(p) != b.validarCadena(p)) return false;
    return true;
}
