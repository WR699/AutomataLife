#include "AdministradorDeAutomatas.h"
#include <fstream>
#include <iomanip>
#include <limits>
#include <stdexcept>
namespace {
void exigir(bool ok) { if (!ok) throw std::runtime_error("Archivo de automata invalido o incompleto"); }
std::size_t leerCantidad(std::istream& in) {
    std::string s; exigir(bool(in >> s));
    exigir(!s.empty() && s.find_first_not_of("0123456789") == std::string::npos);
    const auto n = std::stoull(s);
    exigir(n <= std::numeric_limits<std::size_t>::max());
    return static_cast<std::size_t>(n);
}
}
void AdministradorDeAutomatas::guardarAutomata(const Automata& a, const std::string& ruta) const {
    a.validar();
    std::ofstream out(ruta);
    if (!out) throw std::runtime_error("No se pudo abrir para guardar: " + ruta);
    out << "AUTOMATA_V2\n" << a.getAlfabeto().size() << '\n';
    for (const auto& simbolo : a.getAlfabeto()) out << std::quoted(simbolo) << '\n';
    out << a.estados().size() << '\n';
    for (const auto& e : a.estados()) out << std::quoted(e->nombre()) << ' ' << e->esFinal() << '\n';
    out << std::quoted(a.inicial()->nombre()) << '\n';
    std::size_t cantidad = 0;
    for (const auto& e : a.estados()) cantidad += e->transiciones().size();
    out << cantidad << '\n';
    for (const auto& e : a.estados()) for (const auto& t : e->transiciones()) {
        out << std::quoted(e->nombre()) << ' ' << std::quoted(t.simbolo()) << ' ' << t.destinos().size();
        for (const auto* d : t.destinos()) out << ' ' << std::quoted(d->nombre());
        out << '\n';
    }
    out.close();
    if (!out) throw std::runtime_error("Error escribiendo: " + ruta);
}
Automata AdministradorDeAutomatas::cargarAutomata(const std::string& ruta) const {
    std::ifstream in(ruta);
    if (!in) throw std::runtime_error("No se pudo abrir: " + ruta);
    std::string cabecera; exigir(bool(in >> cabecera) && (cabecera == "AUTOMATA_V1" || cabecera == "AUTOMATA_V2"));
    Automata a;
    if (cabecera == "AUTOMATA_V2") {
        const auto cantidad = leerCantidad(in);
        std::set<std::string> simbolos;
        for (std::size_t i = 0; i < cantidad; ++i) {
            std::string simbolo; exigir(bool(in >> std::quoted(simbolo)));
            exigir(!simbolo.empty() && simbolos.insert(simbolo).second);
        }
        a.setAlfabeto(simbolos);
    }
    const auto declarado = a.getAlfabeto();
    const auto n = leerCantidad(in);
    for (std::size_t i = 0; i < n; ++i) {
        std::string nombre; int final;
        exigir(bool(in >> std::quoted(nombre) >> final));
        exigir(final == 0 || final == 1);
        a.agregarEstado(nombre, final == 1);
    }
    std::string inicial; exigir(bool(in >> std::quoted(inicial))); a.establecerInicial(inicial);
    const auto t = leerCantidad(in);
    for (std::size_t i = 0; i < t; ++i) {
        std::string origen, simbolo; exigir(bool(in >> std::quoted(origen) >> std::quoted(simbolo)));
        const auto cantidad = leerCantidad(in);
        std::vector<std::string> destinos;
        for (std::size_t j = 0; j < cantidad; ++j) {
            std::string d; exigir(bool(in >> std::quoted(d))); destinos.push_back(d);
        }
        a.agregarTransicion(origen, simbolo, destinos);
    }
    in >> std::ws; exigir(in.eof());
    if (cabecera == "AUTOMATA_V2") a.setAlfabeto(declarado);
    a.validar();
    return a;
}
