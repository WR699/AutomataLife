#pragma once
#include "Automata.h"
struct ResultadoEquivalencia {
    bool equivalentes = true;
    std::vector<std::string> contraejemplo;
    std::size_t paresExplorados = 0;
};
class TesterEquivalencia {
public:
    // Prueba solamente las cadenas dadas; no demuestra equivalencia universal.
    bool probarEquivalencia(const Automata& a1, const Automata& a2,
                           const std::vector<std::string>& cadenasPrueba) const;
    std::vector<std::string> generarCadenasDePrueba(const std::set<std::string>& alfabeto,
                                                  int longitudMaxima) const;
    // Versiones por tokens para simbolos multicaracter/UTF-8.
    bool probarEquivalenciaPorSimbolos(const Automata& a1, const Automata& a2,
                          const std::vector<std::vector<std::string>>& palabras) const;
    std::vector<std::vector<std::string>> generarPalabrasDePrueba(
                          const std::set<std::string>& alfabeto, int longitudMaxima) const;
    // Comparacion por iteraciones. Si recibe AFND los convierte primero a AFD.
    // Si los alfabetos tienen igual tamano pero simbolos distintos, renombra el segundo.
    // Cada par de estados se revisa una sola vez.
    // CODIGO VIEJO: siempre imprimia mensajes durante la comparacion.
    // bool compararAFDPorIteraciones(const Automata& primero, const Automata& segundo) const;
    bool compararAFDPorIteraciones(const Automata& primero, const Automata& segundo,
                                   bool mostrarMensajes = true) const;

    // Demostracion exacta por producto de AFD, no limitada a un conjunto de prueba.
    ResultadoEquivalencia comparar(const Automata& primero, const Automata& segundo) const;
};
