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
    auto& s = a.agregarEstado("s");
    auto& f = a.agregarEstado("f", true);
    comprobar(a.buscarEstado("SR") != nullptr);
    comprobar(!a.buscarEstado("SR")->esEstadoFinal());

    s.setEstadoInicial(true);
    comprobar(s.isEstadoInicial() && a.getEstadoInicial() == &s);
    f.setEstadoInicial(true);
    comprobar(!s.esEstadoInicial() && f.esEstadoInicial() && a.getEstadoInicial() == &f);
    f.setEstadoInicial(false);
    comprobar(!a.getEstadoInicial());
    s.setEsInicial(true);

    s.setId("inicio");
    comprobar(a.getEstadoInicial()->getId() == "inicio");
    falla([&]{ s.setId("f"); });
    falla([&]{ s.setId(""); });
    falla([&]{ s.setId("SR"); });

    // La lista vieja de varios destinos se separa en aristas individuales.
    a.agregarTransicion("inicio", "a", {"f", "inicio"});
    std::size_t salidasA = 0;
    for (const auto& t : s.getTransiciones()) {
        if (t.getSimbolo() == "a") {
            ++salidasA;
            comprobar(t.getCantidadDestinos() == 1);
            comprobar(t.getDestino() != nullptr);
        }
    }
    comprobar(salidasA == 2);
    comprobar(a.getTipo() == TipoAutomata::AFND); // 2 transiciones, 1 simbolo: palomar.

    // Duplicar exactamente la misma arista no duplica la relacion.
    const auto antes = s.getCantidadTransiciones();
    a.agregarTransicion("inicio", "a", {"f"});
    comprobar(s.getCantidadTransiciones() == antes);

    // Agregar b completa automaticamente estados sin b con -> SR.
    a.agregarSimbolo("b");
    auto* sr = a.getEstadoError();
    comprobar(sr && sr->getCantidadTransiciones() == a.getAlfabeto().size());
    bool tieneBError = false;
    for (const auto& t : s.getTransiciones())
        if (t.getSimbolo() == "b" && t.getDestino() == sr) tieneBError = true;
    comprobar(tieneBError);

    // Reemplazar las salidas por una relacion determinista. El completado agrega lo faltante.
    s.setTransiciones({Transicion("a", &f), Transicion("b", &f)});
    comprobar(a.esDeterminista());
    a.setTipo(TipoAutomata::AFD);
    falla([&]{ a.setTipo(TipoAutomata::AFND); });

    Estado externo("externo");
    falla([&]{ s.agregarTransicion(Transicion("x", &externo)); });
    falla([&]{ s.setTransiciones({Transicion("x", &externo)}); });
    falla([&]{ a.setEstadoInicial(&externo); });
    falla([&]{ a.agregarEstado("SR"); });
    falla([&]{ sr->setEstadoInicial(true); });
    falla([&]{ sr->setEstadoFinal(true); });

    // Una Transicion suelta sigue siendo editable, pero una insertada se edita eliminando/agregando.
    Transicion suelta("x", &s);
    suelta.setSimbolo("y");
    suelta.setDestino(&f);
    comprobar(suelta.getDestino() == &f);
    falla([&]{ s.getTransicion(0).setDestino(&s); });

    auto copia = a.clonar();
    comprobar(copia.getEstadoError() != a.getEstadoError());
    comprobar(copia.getEstadoInicial()->getId() == a.getEstadoInicial()->getId());

    auto movido = std::move(a);
    movido.validar();
    auto* inicio = movido.getEstadoInicial();
    inicio->setId("renombrado");
    comprobar(movido.buscarEstado("renombrado") == inicio);

    Automata asignado;
    asignado = std::move(movido);
    comprobar(asignado.eliminarEstado("f"));
    // Al desaparecer f, sus salidas se reemplazan por fallbacks a SR.
    comprobar(asignado.esDeterminista());
    comprobar(!asignado.eliminarEstado("SR"));

    asignado.limpiar();
    comprobar(asignado.getCantidadEstados() == 0 && !asignado.getEstadoInicial());

    // setEstados tambien normaliza y agrega SR.
    std::vector<std::unique_ptr<Estado>> es;
    es.push_back(std::make_unique<Estado>("p", true, false));
    es.push_back(std::make_unique<Estado>("q", false, true));
    es[0]->agregarTransicion(Transicion("", es[1].get()));
    es[1]->agregarTransicion(Transicion("", es[0].get()));
    asignado.setEstados(std::move(es));
    asignado.validar();
    comprobar(asignado.getEstadoError() != nullptr);

    auto* p = asignado.buscarEstado("p");
    auto* q = asignado.buscarEstado("q");
    ConversorAFND conversor;
    comprobar(conversor.clausuraEpsilon({p}) == std::set<Estado*>({p, q}));
    q->agregarTransicion(Transicion("a", q));
    comprobar(conversor.mover({p, q}, "a").count(q) != 0);
    falla([&]{ conversor.clausuraEpsilon({nullptr}); });
    comprobar(!asignado.esDeterminista()); // epsilon agrega transiciones sobre |Sigma|.

    asignado.agregarEstado("inaccesible", true);
    asignado.agregarSimbolo("z");
    MinimizadorAFD minimizador;
    auto accesible = minimizador.eliminarEstadosInaccesibles(asignado);
    comprobar(accesible.getAlfabeto().count("z"));

    auto afd = conversor.convertirA_AFD(asignado);
    comprobar(afd.esDeterminista());
    auto grupos = minimizador.particionarEstados(afd);
    for (const auto& grupo : grupos)
        for (auto* e : grupo) comprobar(afd.contieneEstado(e));
    falla([&]{ minimizador.particionarEstados(asignado); });

    TesterEquivalencia tester;
    auto minimo = minimizador.minimizar(asignado);
    comprobar(tester.comparar(asignado, minimo).equivalentes);

    auto cadenas = tester.generarCadenasDePrueba({"a", "z"}, 3);
    comprobar(cadenas.size() == 15 && cadenas.front().empty());
    comprobar(tester.probarEquivalencia(asignado, minimo, cadenas));
    comprobar(tester.generarCadenasDePrueba({}, 30).size() == 1);
    falla([&]{ tester.generarCadenasDePrueba({"a"}, -1); });
    falla([&]{ tester.generarCadenasDePrueba({"token"}, 2); });

    AdministradorDeAutomatas archivos;
    archivos.guardarAutomata(asignado, "api_v3.automata");
    auto leido = archivos.cargarAutomata("api_v3.automata");
    comprobar(leido.getAlfabeto() == asignado.getAlfabeto());
    comprobar(leido.getEstadoError() != nullptr);
    comprobar(tester.comparar(leido, asignado).equivalentes);
    std::remove("api_v3.automata");

    archivos.guardarAutomata(asignado, "prueba_xml.xml");
    auto leidoXml = archivos.cargarAutomata("prueba_xml.xml");
    comprobar(leidoXml.getAlfabeto() == asignado.getAlfabeto());
    comprobar(leidoXml.getEstadoError() != nullptr);
    comprobar(tester.comparar(leidoXml, asignado).equivalentes);
    std::remove("prueba_xml.xml");

    InterfazUsuario ui;
    ui.setAutomata(asignado.clonar());
    std::ostringstream salida;
    ui.mostrarMenu(salida);
    std::istringstream entrada("5\n11\n6\n8\na\n0\n");
    ui.ejecutar(entrada, salida);
    comprobar(salida.str().find("Error:") == std::string::npos);
    comprobar(ui.getAutomata().getTipo() == TipoAutomata::AFD);
}
