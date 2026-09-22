#include "AdministradorDeAutomatas.h"
#include <fstream>
#include <iomanip>
#include <limits>
#include <stdexcept>

namespace {

    void exigir(bool ok) {
        if (!ok) throw std::runtime_error("Archivo de automata invalido o incompleto");
    }

    std::size_t leerCantidad(std::istream& in) {
        std::string s;
        exigir(bool(in >> s));
        exigir(!s.empty() && s.find_first_not_of("0123456789") == std::string::npos);
        const auto n = std::stoull(s);
        exigir(n <= std::numeric_limits<std::size_t>::max());
        return static_cast<std::size_t>(n);
    }

    bool esExtensionXml(const std::string& ruta) {
        return ruta.size() >= 4 && ruta.substr(ruta.size() - 4) == ".xml";
    }

} // namespace

void AdministradorDeAutomatas::guardarAutomata(const Automata& a, const std::string& ruta) const {
    if (esExtensionXml(ruta)) {
        parserXml_.guardarEnXML(a, ruta);
        return;
    }

    a.validar();
    std::ofstream out(ruta);
    if (!out) throw std::runtime_error("No se pudo abrir para guardar: " + ruta);

    out << "AUTOMATA_V2\n" << a.getAlfabeto().size() << '\n';
    for (const auto& simbolo : a.getAlfabeto()) {
        out << std::quoted(simbolo) << '\n';
    }

    out << a.getEstados().size() << '\n';
    for (const auto* e : a.getEstados()) {
        out << std::quoted(e->getId()) << ' ' << e->esEstadoFinal() << '\n';
    }

    out << std::quoted(a.getEstadoInicial()->getId()) << '\n';

    std::size_t cantidad = 0;
    for (const auto* e : a.getEstados()) {
        cantidad += e->getCantidadTransiciones();
    }

    out << cantidad << '\n';
    for (const auto* e : a.getEstados()) {
        for (const auto& t : e->getTransiciones()) {
            out << std::quoted(e->getId()) << ' ' << std::quoted(t.getSimbolo()) << ' ' << t.getDestinos().size();
            for (const auto* d : t.getDestinos()) {
                out << ' ' << std::quoted(d->getId());
            }
            out << '\n';
        }
    }

    out.close();
    if (!out) throw std::runtime_error("Error escribiendo: " + ruta);
}

Automata AdministradorDeAutomatas::cargarAutomata(const std::string& ruta) const {
    if (esExtensionXml(ruta)) {
        return parserXml_.cargarDesdeXML(ruta);
    }

    std::ifstream in(ruta);
    if (!in) throw std::runtime_error("No se pudo abrir: " + ruta);

    std::string cabecera;
    exigir(bool(in >> cabecera) && (cabecera == "AUTOMATA_V1" || cabecera == "AUTOMATA_V2"));

    Automata a;
    if (cabecera == "AUTOMATA_V2") {
        const auto cantidad = leerCantidad(in);
        std::set<std::string> simbolos;
        for (std::size_t i = 0; i < cantidad; ++i) {
            std::string simbolo;
            exigir(bool(in >> std::quoted(simbolo)));
            exigir(!simbolo.empty() && simbolos.insert(simbolo).second);
        }
        a.setAlfabeto(simbolos);
    }

    const auto declarado = a.getAlfabeto();
    const auto n = leerCantidad(in);
    for (std::size_t i = 0; i < n; ++i) {
        std::string nombre;
        int final;
        exigir(bool(in >> std::quoted(nombre) >> final));
        exigir(final == 0 || final == 1);
        a.agregarEstado(nombre, final == 1);
    }

    std::string inicial;
    exigir(bool(in >> std::quoted(inicial)));
    a.setEstadoInicial(inicial);

    const auto t = leerCantidad(in);
    for (std::size_t i = 0; i < t; ++i) {
        std::string origen, simbolo;
        exigir(bool(in >> std::quoted(origen) >> std::quoted(simbolo)));
        const auto cantidad = leerCantidad(in);
        std::vector<std::string> destinos;
        for (std::size_t j = 0; j < cantidad; ++j) {
            std::string d;
            exigir(bool(in >> std::quoted(d)));
            destinos.push_back(d);
        }
        a.agregarTransicion(origen, simbolo, destinos);
    }

    in >> std::ws;
    exigir(in.eof());

    if (cabecera == "AUTOMATA_V2") {
        a.setAlfabeto(declarado);
    }

    a.validar();
    return a;
}