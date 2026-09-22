#include "TesterEquivalencia.h"
#include "ConversorAFND.h"
#include "AlgoritmosInternos.h"
#include <algorithm>
ResultadoEquivalencia TesterEquivalencia::comparar(const Automata& primero,
                                                      const Automata& segundo) const {
    auto alfabeto = primero.alfabeto();
    auto otro = segundo.alfabeto();
    alfabeto.insert(otro.begin(), otro.end());
    auto a = ConversorAFND{}.convertir(primero, alfabeto);
    auto b = ConversorAFND{}.convertir(segundo, alfabeto);
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
