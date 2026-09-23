#include "InterfazUsuario.h"
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
namespace {
std::vector<std::string> argumentos(std::istringstream& in) {
    std::vector<std::string> args;
    in >> std::ws;
    while (!in.eof()) {
        std::string s;
        if (!(in >> std::quoted(s))) throw std::invalid_argument("Argumento o comillas invalidas");
        args.push_back(s); in >> std::ws;
    }
    return args;
}
void cantidad(const std::vector<std::string>& a, std::size_t n) {
    if (a.size() != n) throw std::invalid_argument("Cantidad de argumentos incorrecta; usa ayuda");
}
bool booleano(const std::string& s) {
    if (s != "0" && s != "1") throw std::invalid_argument("Se esperaba 0 o 1");
    return s == "1";
}
}
InterfazUsuario::InterfazUsuario() : actual_(constructor_.ejemplo()) {}
void InterfazUsuario::mostrar(std::ostream& out) const {
    out << "Estados: " << actual_.estados().size() << '\n';
    for (const auto& e : actual_.estados()) {
        out << (e.get() == actual_.inicial() ? "-> " : "   ") << std::quoted(e->nombre())
            << (e->esFinal() ? " [FINAL]" : "") << '\n';
        for (const auto& t : e->transiciones()) {
            out << "     " << (t.esEpsilon() ? "epsilon" : t.simbolo())
                << " -> " << std::quoted(t.getDestino()->nombre()) << '\n';
        }
    }
}
void InterfazUsuario::ejecutar(std::istream& in, std::ostream& out) {
    out << "Sistema de automatas C++17. Ejemplo cargado. Escribe ayuda.\n";
    salir_ = false;
    mostrarMenu(out);
    std::string linea;
    while (!salir_ && (out << "> ") && std::getline(in, linea)) {
        try {
            std::istringstream parser(linea);
            std::string cmd; if (!(parser >> cmd)) continue;
            const auto a = argumentos(parser);
            if (cmd.find_first_not_of("0123456789") == std::string::npos) {
                cantidad(a, 0); procesarOpcion(std::stoi(cmd), in, out); continue;
            }
            if (cmd == "salir") { cantidad(a, 0); break; }
            if (cmd == "ayuda") {
                cantidad(a, 0);
                mostrarMenu(out);
                out << "nuevo | ejemplo | mostrar | validar | determinizar | minimizar | salir\n"
                       "estado nombre final(0/1)\ninicial nombre\nfinal nombre 0/1\n"
                       "transicion origen simbolo destino1 destino2 ...\n"
                       "  Cada destino se guarda como una transicion individual.\n"
                       "  Simbolo vacio \"\" = epsilon. Sin destino = salida a SR.\n"
                       "probar simbolo1 simbolo2 ... (sin argumentos = palabra vacia)\n"
                       "guardar ruta | cargar ruta | equivalencia ruta\n"
                       "renombrar id nuevoId | eliminar id | alfabeto simbolos... | cadena texto\n"
                       "Usa comillas para nombres o rutas con espacios.\n";
            } else if (cmd == "nuevo") { cantidad(a, 0); actual_ = constructor_.crear(); }
            else if (cmd == "ejemplo") { cantidad(a, 0); actual_ = constructor_.ejemplo(); }
            else if (cmd == "estado") { cantidad(a, 2); constructor_.agregarEstado(actual_, a[0], booleano(a[1])); }
            else if (cmd == "inicial") { cantidad(a, 1); constructor_.establecerInicial(actual_, a[0]); }
            else if (cmd == "final") { cantidad(a, 2); constructor_.establecerFinal(actual_, a[0], booleano(a[1])); }
            else if (cmd == "transicion") {
                if (a.size() < 2) throw std::invalid_argument("Faltan origen y simbolo");
                constructor_.agregarTransicion(actual_, a[0], a[1], {a.begin() + 2, a.end()});
            } else if (cmd == "renombrar") {
                cantidad(a, 2); auto* e = actual_.buscarEstado(a[0]);
                if (!e) throw std::invalid_argument("Estado inexistente");
                e->setId(a[1]);
            } else if (cmd == "eliminar") {
                cantidad(a, 1); out << (actual_.eliminarEstado(a[0]) ? "Eliminado.\n" : "No existe.\n");
            } else if (cmd == "alfabeto") {
                actual_.setAlfabeto({a.begin(), a.end()});
            } else if (cmd == "cadena") {
                cantidad(a, 1); out << (actual_.validarCadena(a[0]) ? "ACEPTADA\n" : "RECHAZADA\n");
            } else if (cmd == "mostrar") { cantidad(a, 0); mostrar(out); }
            else if (cmd == "validar") {
                cantidad(a, 0); actual_.validar();
                out << (actual_.esDeterministico() ? "AFD valido\n" : "AFN/AFN-epsilon valido\n");
            } else if (cmd == "probar") { out << (actual_.acepta(a) ? "ACEPTADA\n" : "RECHAZADA\n"); }
            else if (cmd == "determinizar") { cantidad(a, 0); actual_ = conversor_.convertir(actual_); mostrar(out); }
            else if (cmd == "minimizar") { cantidad(a, 0); actual_ = minimizador_.minimizar(actual_); mostrar(out); }
            else if (cmd == "guardar") { cantidad(a, 1); archivos_.guardar(actual_, a[0]); out << "Guardado.\n"; }
            else if (cmd == "cargar") { cantidad(a, 1); actual_ = archivos_.cargar(a[0]); out << "Cargado.\n"; }
            else if (cmd == "equivalencia") {
                cantidad(a, 1);
                auto otro = archivos_.cargar(a[0]);
                const auto r = equivalencia_.comparar(actual_, otro);
                out << (r.equivalentes ? "EQUIVALENTES" : "NO EQUIVALENTES")
                    << " (pares explorados: " << r.paresExplorados << ")\n";
                if (!r.equivalentes) {
                    out << "Contraejemplo: ";
                    if (r.contraejemplo.empty()) out << "epsilon";
                    for (const auto& s : r.contraejemplo) out << std::quoted(s) << ' ';
                    out << '\n';
                }
            } else throw std::invalid_argument("Comando desconocido; usa ayuda");
        } catch (const std::exception& e) { out << "Error: " << e.what() << '\n'; }
    }
}

void InterfazUsuario::iniciar() { ejecutar(std::cin, std::cout); }
void InterfazUsuario::mostrarMenu() const { mostrarMenu(std::cout); }
void InterfazUsuario::mostrarMenu(std::ostream& out) const {
    out << "0 Salir | 1 Mostrar | 2 Nuevo | 3 Cargar | 4 Guardar | 5 Determinizar\n"
           "6 Minimizar | 7 Equivalencia exacta | 8 Validar cadena | 9 Ejemplo\n"
           "10 Quitar inaccesibles | 11 Particiones | 12 Pruebas por longitud\n"
           "Tambien podes usar comandos de texto. Escribe ayuda.\n";
}
void InterfazUsuario::procesarOpcion(int opcion) { procesarOpcion(opcion, std::cin, std::cout); }
void InterfazUsuario::procesarOpcion(int opcion, std::istream& in, std::ostream& out) {
    auto pedir = [&](const std::string& etiqueta) {
        out << etiqueta;
        std::string valor;
        if (!std::getline(in, valor)) throw std::runtime_error("Entrada finalizada");
        return valor;
    };
    switch (opcion) {
    case 0: salir_ = true; break;
    case 1: mostrar(out); break;
    case 2: actual_ = constructor_.crear(); out << "Automata vacio. Usa estado e inicial para construirlo.\n"; break;
    case 3: actual_ = archivos_.cargarAutomata(pedir("Ruta (sin comillas): ")); break;
    case 4: archivos_.guardarAutomata(actual_, pedir("Ruta (sin comillas): ")); break;
    case 5: actual_ = conversor_.convertirA_AFD(actual_); mostrar(out); break;
    case 6: actual_ = minimizador_.minimizar(actual_); mostrar(out); break;
    case 7: {
        auto otro = archivos_.cargarAutomata(pedir("Ruta del segundo automata: "));
        const auto r = equivalencia_.comparar(actual_, otro);
        out << (r.equivalentes ? "EQUIVALENTES\n" : "NO EQUIVALENTES\n");
        if (!r.equivalentes) {
            out << "Contraejemplo: ";
            if (r.contraejemplo.empty()) out << "epsilon";
            for (const auto& simbolo : r.contraejemplo) out << std::quoted(simbolo) << ' ';
            out << '\n';
        }
        break;
    }
    case 8: out << (actual_.validarCadena(pedir("Cadena (vacia = epsilon): ")) ? "ACEPTADA\n" : "RECHAZADA\n"); break;
    case 9: actual_ = constructor_.ejemplo(); break;
    case 10: actual_ = minimizador_.eliminarEstadosInaccesibles(actual_); mostrar(out); break;
    case 11:
        for (const auto& grupo : minimizador_.particionarEstados(actual_)) {
            out << "{ "; for (const auto* e : grupo) out << std::quoted(e->getId()) << ' '; out << "}\n";
        }
        break;
    case 12: {
        auto otro = archivos_.cargarAutomata(pedir("Ruta del segundo automata: "));
        auto texto = pedir("Longitud maxima: ");
        std::size_t usados = 0; int longitud = std::stoi(texto, &usados);
        if (usados != texto.size()) throw std::invalid_argument("Longitud invalida");
        auto alfabeto = actual_.getAlfabeto(); auto segundo = otro.getAlfabeto();
        alfabeto.insert(segundo.begin(), segundo.end());
        auto palabras = equivalencia_.generarPalabrasDePrueba(alfabeto, longitud);
        out << (equivalencia_.probarEquivalenciaPorSimbolos(actual_, otro, palabras)
            ? "Coinciden en las palabras probadas; no demuestra equivalencia universal.\n"
            : "Difieren en al menos una palabra probada.\n");
        break;
    }
    default: throw std::invalid_argument("Opcion inexistente");
    }
}
