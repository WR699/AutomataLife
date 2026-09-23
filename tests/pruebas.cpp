#include "ConstruirAutomata.h"
#include "ConversorAFND.h"
#include "MinimizadorAFD.h"
#include "TesterEquivalencia.h"
#include "AdministradorDeAutomatas.h"
#include "InterfazUsuario.h"
#include <cstdio>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <stdexcept>

void verificar(bool ok) { if (!ok) throw std::runtime_error("Fallo de prueba"); }
template<class F> void debeFallar(F f) {
    bool fallo = false;
    try { f(); } catch (const std::exception&) { fallo = true; }
    verificar(fallo);
}
void pruebasApi();

int main() {
    try {
        pruebasApi();
        ConstruirAutomata construir;
        ConversorAFND convertir;
        MinimizadorAFD minimizar;
        TesterEquivalencia equivalencia;
        AdministradorDeAutomatas archivos;

        auto a = construir.ejemplo();
        verificar(a.getEstadoError() != nullptr);
        verificar(a.acepta({"b", "a"}) && !a.acepta({"a", "b"}) && !a.acepta({}));
        verificar(!a.esDeterminista());

        auto* q0 = a.buscarEstado("q0");
        for (int i = 0; i < 200; ++i) a.agregarEstado("extra" + std::to_string(i));
        verificar(q0 == a.buscarEstado("q0"));
        verificar(q0->transiciones()[0].getDestino() == q0);

        auto copia = a.clonar();
        verificar(copia.buscarEstado("q0") != q0);
        verificar(copia.buscarEstado("q0")->transiciones()[0].getDestino() == copia.buscarEstado("q0"));
        auto movido = std::move(copia);
        movido.validar();
        verificar(movido.acepta({"a"}));

        auto d = convertir.convertir(a);
        auto m = minimizar.minimizar(a);
        verificar(d.esDeterministico() && m.esDeterministico());
        verificar(equivalencia.comparar(a, m).equivalentes);

        Automata e;
        e.agregarEstado("s");
        e.agregarEstado("f", true);
        e.establecerInicial("s");
        e.agregarTransicion("s", "", {"f"});
        e.agregarTransicion("f", "", {"s"});
        e.agregarTransicion("f", "a", {"f"});
        verificar(e.acepta({}) && e.acepta({"a", "a"}) && !e.acepta({"b"}));
        verificar(equivalencia.comparar(e, minimizar.minimizar(e)).equivalentes);

        auto r = equivalencia.comparar(a, e);
        verificar(!r.equivalentes && r.contraejemplo.empty());

        Automata vacio;
        vacio.agregarEstado("s");
        vacio.establecerInicial("s");
        verificar(minimizar.minimizar(vacio).esDeterminista());
        r = equivalencia.comparar(vacio, a);
        verificar(!r.equivalentes && r.contraejemplo == std::vector<std::string>{"a"});

        Automata especial;
        especial.agregarEstado("nombre con \"comillas\" y \\ barra", true);
        especial.establecerInicial("nombre con \"comillas\" y \\ barra");
        especial.agregarTransicion(especial.inicial()->nombre(), "token con espacios", {});
        archivos.guardar(especial, "test_roundtrip.automata");
        auto leido = archivos.cargar("test_roundtrip.automata");
        verificar(leido.inicial()->nombre() == especial.inicial()->nombre());
        verificar(leido.getEstadoError() != nullptr);
        verificar(equivalencia.comparar(leido, especial).equivalentes);
        std::remove("test_roundtrip.automata");

        debeFallar([&]{ a.agregarEstado("q0"); });
        debeFallar([&]{ a.agregarTransicion("q0", "x", {"no_existe"}); });
        debeFallar([&]{ Automata{}.validar(); });
        debeFallar([&]{ a.acepta({""}); });

        {
            std::ofstream out("test_invalido.automata");
            out << "AUTOMATA_V1\n-1\n";
        }
        debeFallar([&]{ archivos.cargar("test_invalido.automata"); });
        std::remove("test_invalido.automata");

        // Comparar NFA/AFD en automatas aleatorios con epsilon.
        std::mt19937 azar(42);
        for (int caso = 0; caso < 30; ++caso) {
            Automata nfa;
            for (int i = 0; i < 5; ++i)
                nfa.agregarEstado(std::to_string(i), azar() % 2 != 0);
            nfa.establecerInicial("0");

            for (int i = 0; i < 5; ++i) {
                for (const auto& s : {"", "a", "b"}) {
                    std::vector<std::string> destinos;
                    for (int j = 0; j < 5; ++j)
                        if (azar() % 5 == 0) destinos.push_back(std::to_string(j));
                    nfa.agregarTransicion(std::to_string(i), s, destinos);
                }
            }

            auto afd = convertir.convertir(nfa);
            auto minimo = minimizar.minimizar(nfa);
            verificar(afd.esDeterministico() && minimo.esDeterministico());
            verificar(equivalencia.comparar(nfa, minimo).equivalentes);

            for (int longitud = 0; longitud <= 5; ++longitud) {
                for (int bits = 0; bits < (1 << longitud); ++bits) {
                    std::vector<std::string> palabra;
                    for (int k = 0; k < longitud; ++k)
                        palabra.push_back((bits & (1 << k)) ? "a" : "b");
                    verificar(nfa.acepta(palabra) == afd.acepta(palabra));
                    verificar(nfa.acepta(palabra) == minimo.acepta(palabra));
                }
            }
        }

        InterfazUsuario ui;
        std::istringstream entrada(
            "nuevo\n"
            "estado s 0\n"
            "estado f 1\n"
            "inicial s\n"
            "transicion s a s f\n"
            "probar a\n"
            "probar b\n"
            "determinizar\n"
            "minimizar\n"
            "salir\n");
        std::ostringstream salida;
        ui.ejecutar(entrada, salida);
        verificar(salida.str().find("Error:") == std::string::npos);
        verificar(salida.str().find("ACEPTADA") != std::string::npos);
        verificar(salida.str().find("RECHAZADA") != std::string::npos);

        std::cout << "OK: transiciones unitarias, SR, palomar, epsilon, AFD, minimizacion, archivos y UI.\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
