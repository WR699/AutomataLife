#include "AdministradorDeAutomatas.h"
#include <fstream>
#include <iomanip>
#include <limits>
#include <map>
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

bool esExtensionJsonLocal(const std::string& ruta) {
    return ruta.size() >= 5 && ruta.substr(ruta.size() - 5) == ".json";
}
}

void AdministradorDeAutomatas::guardarAutomata(const Automata& a, const std::string& ruta) const {
    if (esExtensionXmlLocal(ruta)) {
        parserXml_.guardarEnXML(a, ruta);
        return;
    }
    if (esExtensionJsonLocal(ruta)) {
        parserJson_.guardarEnJSON(a, ruta);
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
        // CODIGO VIEJO: se guardaba el ID interno.
        // out << std::quoted(e->getId()) << ' ' << e->esEstadoFinal() << '\n';
        out << std::quoted(e->getNombreVisible()) << ' ' << e->esEstadoFinal() << '\n';
    }

    // CODIGO VIEJO: se guardaba el ID interno del inicial.
    // out << std::quoted(a.getEstadoInicial()->getId()) << '\n';
    out << std::quoted(a.getEstadoInicial()->getNombreVisible()) << '\n';

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
            // CODIGO VIEJO: origen y destino se serializaban con IDs internos.
            // out << std::quoted(e->getId()) << ' ' << std::quoted(t.getSimbolo()) << ' '
            //     << std::quoted(t.getDestino()->getId()) << '\n';
            out << std::quoted(e->getNombreVisible()) << ' '
                << std::quoted(t.getSimbolo()) << ' '
                << std::quoted(t.getDestino()->getNombreVisible()) << '\n';
        }
    }

    out.close();
    if (!out) throw std::runtime_error("Error escribiendo: " + ruta);
}

Automata AdministradorDeAutomatas::cargarAutomata(const std::string& ruta) const {
    // PRIMER INTENTO: Ruta directa
    std::filesystem::path rutaArchivo = ruta;

    if (!std::filesystem::exists(rutaArchivo)) {
        // SEGUNDO INTENTO: Buscar dentro de "automatas" subiendo en la jerarquia
        std::filesystem::path carpetaActual = std::filesystem::current_path();
        bool encontrado = false;

        while (true) {
            rutaArchivo = carpetaActual / "automatas" / ruta;
            if (std::filesystem::exists(rutaArchivo)) { 
                encontrado = true; 
                break; 
            }

            if (carpetaActual == carpetaActual.parent_path()) { break; }
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

    // Despachar a los parsers segun la extension del archivo encontrado
    if (esExtensionXmlLocal(rutaArchivo.string())) {
        return parserXml_.cargarDesdeXML(rutaArchivo.string());
    }
    if (esExtensionJsonLocal(rutaArchivo.string())) {
        return parserJson_.cargarDesdeJSON(rutaArchivo.string());
    }

    std::ifstream in(rutaArchivo);
    if (!in) {
        throw std::runtime_error("Se encontro el archivo pero no se pudo abrir: " + rutaArchivo.string());
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
    std::map<std::string, std::string> nombreAId;

    for (std::size_t i = 0; i < n; ++i) {
        std::string nombre;
        int final;
        exigir(bool(in >> std::quoted(nombre) >> final));
        exigir(final == 0 || final == 1);
        exigir(nombre != Automata::ID_ESTADO_ERROR);

        // CODIGO VIEJO: el nombre del archivo tambien era el ID usado por los algoritmos.
        // a.agregarEstado(nombre, final == 1);
        const std::string idInterno = "S" + std::to_string(i + 1);
        exigir(nombreAId.emplace(nombre, idInterno).second);
        auto& estado = a.agregarEstado(idInterno, final == 1);
        estado.setNombreVisible(nombre);
    }

    std::string inicial;
    exigir(bool(in >> std::quoted(inicial)));
    exigir(nombreAId.count(inicial) == 1);
    a.setEstadoInicial(nombreAId.at(inicial));

    const auto t = leerCantidad(in);
    for (std::size_t i = 0; i < t; ++i) {
        std::string origen, simbolo;
        exigir(bool(in >> std::quoted(origen) >> std::quoted(simbolo)));
        exigir(nombreAId.count(origen) == 1);

        if (cabecera == "AUTOMATA_V3") {
            std::string destino;
            exigir(bool(in >> std::quoted(destino)));
            exigir(nombreAId.count(destino) == 1);
            a.agregarTransicion(nombreAId.at(origen), simbolo, {nombreAId.at(destino)});
        } else {
            const auto cantidad = leerCantidad(in);
            std::vector<std::string> destinos;
            for (std::size_t j = 0; j < cantidad; ++j) {
                std::string d;
                exigir(bool(in >> std::quoted(d)));
                exigir(nombreAId.count(d) == 1);
                destinos.push_back(nombreAId.at(d));
            }
            a.agregarTransicion(nombreAId.at(origen), simbolo, destinos);
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

std::vector<std::string> AdministradorDeAutomatas::cargarCadenas(const std::string& ruta) const {
    std::filesystem::path rutaArchivo = ruta;

    if (!std::filesystem::exists(rutaArchivo)) {
        std::filesystem::path carpetaActual = std::filesystem::current_path();
        bool encontrado = false;

        while (true) {
            rutaArchivo = carpetaActual / "cadenas" / ruta;
            if (std::filesystem::exists(rutaArchivo)) { 
                encontrado = true; 
                break; 
            }

            if (carpetaActual == carpetaActual.parent_path()) { 
                break; 
            }
            carpetaActual = carpetaActual.parent_path();
        }

        if (!encontrado) {
            throw std::runtime_error(
                "No se pudo encontrar el archivo de cadenas: " + ruta +
                "\n\nOpciones validas:"
                "\n- Ruta absoluta"
                "\n- Ruta relativa"
                "\n- Nombre de un archivo dentro de la carpeta 'cadenas'"
            );
        }
    }

    std::ifstream in(rutaArchivo);
    if (!in) {
        throw std::runtime_error("Se encontro el archivo pero no se pudo abrir: " + rutaArchivo.string());
    }

    std::vector<std::string> cadenas;
    std::string linea;
    while (std::getline(in, linea)) {
        if (!linea.empty() && linea.back() == '\r') {
            linea.pop_back();
        }
        cadenas.push_back(linea);
    }

    return cadenas;
}

void AdministradorDeAutomatas::guardarCadenas(
    const std::vector<std::vector<std::string>>& palabras, 
    const std::string& ruta
) const {
    std::filesystem::path carpetaCadenas = std::filesystem::current_path() / "cadenas";
    std::filesystem::create_directories(carpetaCadenas);

    std::filesystem::path rutaSalida = carpetaCadenas / ruta;
    std::ofstream out(rutaSalida);
    if (!out) {
        throw std::runtime_error("No se pudo crear el archivo de cadenas en: " + rutaSalida.string());
    }

    for (const auto& palabra : palabras) {
        std::string cadena;
        for (const auto& simbolo : palabra) {
            cadena += simbolo;
        }
        out << cadena << '\n';
    }
}