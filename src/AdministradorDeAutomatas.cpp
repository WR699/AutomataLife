#include "AdministradorDeAutomatas.h"
#include <fstream>
#include <iomanip>
#include <limits>
#include <set>
#include <stdexcept>
#include <filesystem>

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

bool esExtensionXmlLocal(const std::string& ruta) {
    return ruta.size() >= 4 && ruta.substr(ruta.size() - 4) == ".xml";
}
}

void AdministradorDeAutomatas::guardarAutomata(const Automata& a, const std::string& ruta) const {
    if (esExtensionXmlLocal(ruta)) {
        parserXml_.guardarEnXML(a, ruta);
        return;
    }

    a.validar();
    std::ofstream out(ruta);
    if (!out) throw std::runtime_error("No se pudo abrir para guardar: " + ruta);

    // V3: una linea de archivo = una transicion = un solo destino.
    // SR y sus fallbacks no se serializan: se regeneran al cargar.
    out << "AUTOMATA_V3\n" << a.getAlfabeto().size() << '\n';
    for (const auto& simbolo : a.getAlfabeto()) out << std::quoted(simbolo) << '\n';

    const auto* sr = a.getEstadoError();
    std::size_t cantidadEstados = 0;
    for (const auto* e : a.getEstados()) if (e != sr) ++cantidadEstados;
    out << cantidadEstados << '\n';

    for (const auto* e : a.getEstados()) {
        if (e == sr) continue;
        out << std::quoted(e->getId()) << ' ' << e->esEstadoFinal() << '\n';
    }

    out << std::quoted(a.getEstadoInicial()->getId()) << '\n';

    std::size_t cantidadTransiciones = 0;
    for (const auto* e : a.getEstados()) {
        if (e == sr) continue;
        for (const auto& t : e->getTransiciones())
            if (t.getDestino() != sr) ++cantidadTransiciones;
    }
    out << cantidadTransiciones << '\n';

    for (const auto* e : a.getEstados()) {
        if (e == sr) continue;
        for (const auto& t : e->getTransiciones()) {
            if (t.getDestino() == sr) continue;
            out << std::quoted(e->getId()) << ' '
                << std::quoted(t.getSimbolo()) << ' '
                << std::quoted(t.getDestino()->getId()) << '\n';
        }
    }

    out.close();
    if (!out) throw std::runtime_error("Error escribiendo: " + ruta);
}

Automata AdministradorDeAutomatas::cargarAutomata(
    const std::string& ruta
) const {

    // PRIMER INTENTO:
    // usar exactamente lo que escribió el usuario.
    // Puede ser ruta absoluta o relativa.
    std::filesystem::path rutaArchivo = ruta;


    if (!std::filesystem::exists(rutaArchivo)) {

        // SEGUNDO INTENTO:
        // buscar una carpeta "automatas" desde la carpeta actual, sino va subiendo carpetas

        std::filesystem::path carpetaActual = std::filesystem::current_path();

        bool encontrado = false;

        //busca en la carpeta actual, sino sube hasta encontrar la cabecera del disco. Tengo que cambiarlo a ya saber cuanto hay que subir
        while (true) {rutaArchivo = carpetaActual/ "automatas"/ ruta;
            if (std::filesystem::exists(rutaArchivo)) {encontrado = true; break;}

            // Llegamos a la raíz del disco/sistema.
            if (carpetaActual == carpetaActual.parent_path()) {break;}
            // Subir una carpeta.
            carpetaActual = carpetaActual.parent_path();
        }


        if (!encontrado) {
    throw std::runtime_error(
        "No se pudo abrir el automata: " + ruta +
        "\n\nOpciones validas:"
        "\n- Ruta absoluta"
        "\n- Ruta relativa"
        "\n- Nombre de un archivo dentro de la carpeta automatas"
    );
}
    }


    // Ahora rutaArchivo contiene la ruta que realmente existe.

    if (esExtensionXmlLocal(rutaArchivo.string())) {
        return parserXml_.cargarDesdeXML(rutaArchivo.string());
    }
    std::ifstream in(rutaArchivo);


    if (!in) {
        throw std::runtime_error("Se encontro el archivo pero no se pudo abrir: "+ rutaArchivo.string());
    }

  

    std::string cabecera;
    exigir(bool(in >> cabecera));
    exigir(cabecera == "AUTOMATA_V1" || cabecera == "AUTOMATA_V2" || cabecera == "AUTOMATA_V3");

    Automata a;
    std::set<std::string> declarado;

    if (cabecera == "AUTOMATA_V2" || cabecera == "AUTOMATA_V3") {
        const auto cantidad = leerCantidad(in);
        for (std::size_t i = 0; i < cantidad; ++i) {
            std::string simbolo;
            exigir(bool(in >> std::quoted(simbolo)));
            exigir(!simbolo.empty() && declarado.insert(simbolo).second);
        }
        a.setAlfabeto(declarado);
    }

    const auto n = leerCantidad(in);
    for (std::size_t i = 0; i < n; ++i) {
        std::string nombre;
        int final;
        exigir(bool(in >> std::quoted(nombre) >> final));
        exigir(final == 0 || final == 1);
        exigir(nombre != Automata::ID_ESTADO_ERROR);
        a.agregarEstado(nombre, final == 1);
    }

    std::string inicial;
    exigir(bool(in >> std::quoted(inicial)));
    a.setEstadoInicial(inicial);

    const auto t = leerCantidad(in);
    for (std::size_t i = 0; i < t; ++i) {
        std::string origen, simbolo;
        exigir(bool(in >> std::quoted(origen) >> std::quoted(simbolo)));

        if (cabecera == "AUTOMATA_V3") {
            std::string destino;
            exigir(bool(in >> std::quoted(destino)));
            a.agregarTransicion(origen, simbolo, {destino});
        } else {
            // Compatibilidad V1/V2: una transicion vieja podia contener varios destinos.
            const auto cantidad = leerCantidad(in);
            std::vector<std::string> destinos;
            for (std::size_t j = 0; j < cantidad; ++j) {
                std::string d;
                exigir(bool(in >> std::quoted(d)));
                destinos.push_back(d);
            }
            // Automata::agregarTransicion separa la lista en aristas individuales.
            a.agregarTransicion(origen, simbolo, destinos);
        }
    }

    in >> std::ws;
    exigir(in.eof());

    if (cabecera == "AUTOMATA_V2" || cabecera == "AUTOMATA_V3")
        a.setAlfabeto(declarado);

    a.completarEstadoError();
    a.validar();
    return a;
}
