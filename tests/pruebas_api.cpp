#include "Automata.h"
#include "ConversorAFND.h"
#include "MinimizadorAFD.h"
#include "AdministradorDeAutomatas.h"
#include "TesterEquivalencia.h"
#include "InterfazUsuario.h"
#include "UserInterface.h"
#include "ConvertirDeterministico.h"
#include <cstdio>
#include <sstream>
#include <stdexcept>
#include <type_traits>
namespace {
void comprobar(bool ok) { if (!ok) throw std::runtime_error("Fallo API UML"); }
template<class F> void falla(F f) {
    bool fallo = false;
    try { f(); } catch (const std::exception&) { fallo = true; }
    comprobar(fallo);
}
}
void pruebasApi() {
    static_assert(std::is_same<UserInterface, InterfazUsuario>::value, "Compatibilidad UI");
    static_assert(std::is_same<ConvertirDeterministico, ConversorAFND>::value, "Compatibilidad conversor");
    Automata a;
    auto& s = a.agregarEstado("s"); auto& f = a.agregarEstado("f", true);
    s.setEstadoInicial(true);
    comprobar(s.isEstadoInicial() && a.getEstadoInicial() == &s);
    f.setEstadoInicial(true);
    comprobar(!s.esEstadoInicial() && f.esEstadoInicial() && a.getEstadoInicial() == &f);
    f.setEstadoInicial(false); comprobar(!a.getEstadoInicial());
    s.setEsInicial(true); f.setEsFinal(false); comprobar(!f.isEstadoFinal()); f.setEstadoFinal(true);
    s.setId("inicio"); comprobar(a.getEstadoInicial()->getId() == "inicio");
    falla([&]{ s.setId("f"); }); falla([&]{ s.setId(""); });
    comprobar(s.getId() == "inicio");
    s.agregarTransicion(Transicion("a", &f));
    comprobar(s.getTransicion(0).getDestino() == &f);
    s.getTransicion(0).agregarDestino(&s);
    comprobar(s.getTransicion(0).getCantidadDestinos() == 2);
    falla([&]{ s.getTransicion(0).getDestino(); });
    comprobar(a.getTipo() == TipoAutomata::AFND);
    a.setTipo(TipoAutomata::AFND); falla([&]{ a.setTipo(TipoAutomata::AFD); });
    s.getTransicion(0).setDestinos({&f, &f}); comprobar(s.getTransicion(0).getCantidadDestinos() == 1);
    s.getTransicion(0).setSimbolo("b"); comprobar(a.getAlfabeto() == std::set<std::string>{"b"});
    comprobar(a.validarCadena(std::string("b")) && a.validarCadena(std::vector<std::string>{"b"}));
    comprobar(!a.validarCadena(std::string("a")));
    a.setAlfabeto({"a", "b"}); a.agregarSimbolo("c"); comprobar(a.eliminarSimbolo("c"));
    falla([&]{ a.setAlfabeto({"a"}); }); falla([&]{ a.eliminarSimbolo("b"); });
    falla([&]{ a.agregarSimbolo(""); });
    a.setTipo(TipoAutomata::AFD);
    Estado externo("externo");
    falla([&]{ s.getTransicion(0).agregarDestino(&externo); });
    falla([&]{ s.getTransicion(0).setDestinos({&s, nullptr}); });
    falla([&]{ s.getTransicion(0) = Transicion("x", &externo); });
    falla([&]{ s.setTransiciones({Transicion("x", &externo)}); });
    falla([&]{ a.setEstadoInicial(&externo); });
    comprobar(s.getTransicion(0).getDestino() == &f);
    // La realocacion y la asignacion de transiciones conservan la validacion.
    for (int i = 0; i < 50; ++i) s.agregarTransicion(Transicion("b", &f));
    falla([&]{ s.getTransicion(0).setDestino(&externo); });
    s.setTransiciones({Transicion("a", &f), Transicion("b", &s)});
    comprobar(s.getCantidadTransiciones() == 2);
    s.eliminarTransicion(1); falla([&]{ s.eliminarTransicion(2); });
    auto copia = a.clonar();
    auto movido = std::move(a);
    auto* inicio = movido.getEstadoInicial();
    inicio->setId("renombrado"); comprobar(movido.buscarEstado("renombrado") == inicio);
    inicio->getTransicion(0).setDestino(inicio); // propietario revinculado tras mover.
    falla([&]{ inicio->getTransicion(0).setDestino(copia.getEstadoInicial()); });
    Automata asignado; asignado = std::move(movido);
    asignado.getEstadoInicial()->getTransicion(0).setDestino(asignado.buscarEstado("f"));
    comprobar(asignado.eliminarEstado("f"));
    comprobar(asignado.getEstadoInicial()->getTransicion(0).getDestino() == nullptr);
    comprobar(!asignado.eliminarEstado("f"));
    comprobar(asignado.getEstados().size() == 1);
    auto& tt = asignado.getEstadoInicial()->getTransicion(0);
    tt.setDestino(asignado.getEstadoInicial()); comprobar(tt.contieneDestino(asignado.getEstadoInicial()));
    comprobar(tt.eliminarDestino(asignado.getEstadoInicial()));
    tt.setDestino(nullptr); tt.limpiarDestinos(); tt.setSimbolo(""); comprobar(tt.isEpsilon());
    asignado.getEstadoInicial()->limpiarTransiciones();
    comprobar(asignado.getEstadoInicial()->getCantidadTransiciones() == 0);
    asignado.limpiar(); comprobar(asignado.getCantidadEstados() == 0 && !asignado.getEstadoInicial());
    // Set de estados con punteros cruzados, seguido de setters directos.
    std::vector<std::unique_ptr<Estado>> es;
    es.push_back(std::make_unique<Estado>("p", true, false));
    es.push_back(std::make_unique<Estado>("q", false, true));
    es[0]->agregarTransicion(Transicion("", es[1].get()));
    es[1]->agregarTransicion(Transicion("", es[0].get()));
    asignado.setEstados(std::move(es)); asignado.validar();
    auto* p = asignado.buscarEstado("p"); auto* q = asignado.buscarEstado("q");
    ConversorAFND conversor;
    comprobar(conversor.clausuraEpsilon({p}) == std::set<Estado*>({p,q}));
    q->agregarTransicion(Transicion("a", q));
    comprobar(conversor.mover({p,q}, "a") == std::set<Estado*>{q});
    falla([&]{ conversor.clausuraEpsilon({nullptr}); });
    asignado.agregarEstado("inaccesible", true);
    asignado.agregarSimbolo("z");
    MinimizadorAFD minimizador;
    auto accesible = minimizador.eliminarEstadosInaccesibles(asignado);
    comprobar(accesible.getCantidadEstados() == 2 && accesible.getAlfabeto().count("z"));
    auto afd = conversor.convertirA_AFD(asignado);
    comprobar(afd.esDeterminista());
    auto grupos = minimizador.particionarEstados(afd);
    for (const auto& grupo : grupos) for (auto* e : grupo) comprobar(afd.contieneEstado(e));
    falla([&]{ minimizador.particionarEstados(asignado); });
    Automata parcial;
    parcial.agregarEstado("dead1"); parcial.agregarEstado("dead2"); parcial.setEstadoInicial("dead1");
    parcial.agregarTransicion("dead2", "x", {"dead2"});
    comprobar(minimizador.particionarEstados(parcial).size() == 1);
    TesterEquivalencia tester;
    auto minimo = minimizador.minimizar(asignado);
    comprobar(tester.comparar(asignado, minimo).equivalentes);
    auto cadenas = tester.generarCadenasDePrueba({"a","z"}, 3);
    comprobar(cadenas.size() == 15 && cadenas.front().empty());
    comprobar(tester.probarEquivalencia(asignado, minimo, cadenas));
    comprobar(tester.generarCadenasDePrueba({}, 30).size() == 1);
    falla([&]{ tester.generarCadenasDePrueba({"a"}, -1); });
    falla([&]{ tester.generarCadenasDePrueba({"token"}, 2); });
    auto palabras = tester.generarPalabrasDePrueba({"token", "otro"}, 2);
    comprobar(palabras.size() == 7);
    comprobar(tester.probarEquivalenciaPorSimbolos(asignado, minimo, palabras));
    AdministradorDeAutomatas archivos;
    archivos.guardarAutomata(asignado, "api_v2.automata");
    auto leido = archivos.cargarAutomata("api_v2.automata");
    comprobar(leido.getAlfabeto() == asignado.getAlfabeto());
    comprobar(leido.getEstadoInicial()->isEstadoInicial());
    comprobar(tester.comparar(leido, asignado).equivalentes);
    std::remove("api_v2.automata");

    // === PRUEBA DE SERIALIZACIÓN Y DESERIALIZACIÓN XML ===
    archivos.guardarAutomata(asignado, "prueba_xml.xml");
    auto leidoXml = archivos.cargarAutomata("prueba_xml.xml");

    // Verificaciones de integridad del objeto parseado desde XML
    comprobar(leidoXml.getAlfabeto() == asignado.getAlfabeto());
    comprobar(leidoXml.getCantidadEstados() == asignado.getCantidadEstados());
    comprobar(leidoXml.getEstadoInicial() != nullptr);
    comprobar(leidoXml.getEstadoInicial()->getId() == asignado.getEstadoInicial()->getId());

    // Verificación de equivalencia semántica entre el autómata original y el recuperado del XML
    comprobar(tester.comparar(leidoXml, asignado).equivalentes);

    // Limpieza del archivo temporal
    std::remove("prueba_xml.xml");

    InterfazUsuario ui; ui.setAutomata(asignado.clonar());
    std::ostringstream salida; ui.mostrarMenu(salida);
    std::istringstream entrada("5\n11\n6\n8\na\n0\n"); ui.ejecutar(entrada, salida);
    comprobar(salida.str().find("Error:") == std::string::npos);
    comprobar(salida.str().find("ACEPTADA") != std::string::npos);
    comprobar(ui.getAutomata().getTipo() == TipoAutomata::AFD);
}
