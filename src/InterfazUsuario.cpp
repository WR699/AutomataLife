#include "InterfazUsuario.h"
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace {
std::vector<std::string> obtenerArgumentos(std::istringstream& in) {
    std::vector<std::string> args;
    in >> std::ws;
    while (!in.eof()) {
        std::string s;
        if (!(in >> std::quoted(s))) throw std::invalid_argument("Argumento o comillas invalidas");
        args.push_back(s);
        in >> std::ws;
    }
    return args;
}

void validarCantidadArgs(const std::vector<std::string>& args, std::size_t esperados) {
    if (args.size() != esperados) throw std::invalid_argument("Cantidad de argumentos incorrecta; usa ayuda");
}

bool parsearBooleano(const std::string& s) {
    if (s != "0" && s != "1") throw std::invalid_argument("Se esperaba 0 o 1");
    return s == "1";
}
}

InterfazUsuario::InterfazUsuario() 
    : actual_(constructor_.ejemplo()), 
      etiquetaActual_("Ejemplo inicial (AFND)"),
      etiquetaComparacion_("Ninguno") {}

void InterfazUsuario::mostrar(std::ostream& out) const {
    out << "========================================================\n";
    out << " AUTOMATA EN USO: " << etiquetaActual_ << "\n";
    out << " TIPO: " << (actual_.esDeterminista() ? "AFD (Determinista)" : "AFND (No Determinista)") << "\n";
    if (!etiquetaComparacion_.empty() && etiquetaComparacion_ != "Ninguno") {
        out << " EN REFERENCIA PARA COMPARAR: " << etiquetaComparacion_ << "\n";
    }
    out << "========================================================\n";
    out << "Estados (" << actual_.estados().size() << "):\n";
    for (const auto& e : actual_.estados()) {
        out << (e.get() == actual_.inicial() ? "-> " : "   ") << std::quoted(e->nombre());
        if (e->getNombreVisible() != e->getId()) {out << " = " << std::quoted(e->getNombreVisible());}
        out << (e->esFinal() ? " [FINAL]" : "") << '\n';

        for (const auto& t : e->transiciones()) {
            const Estado* destino = t.getDestino();
            out << "     " << (t.esEpsilon() ? "epsilon" : t.simbolo())
                << " -> " << std::quoted(destino->nombre());
            if (destino->getNombreVisible() != destino->getId()) {out << " = " << std::quoted(destino->getNombreVisible());}
            out << '\n';
        }
    }
    out << "--------------------------------------------------------\n";
}

void InterfazUsuario::ejecutar(std::istream& in, std::ostream& out) {
    out << "Sistema de automatas C++17. Ejemplo cargado. Escribe ayuda.\n";
    salir_ = false;
    mostrarMenu(out);
    std::string linea;
    while (!salir_ && (out << "> ") && std::getline(in, linea)) {
        try {
            std::istringstream parser(linea);
            std::string cmd;
            if (!(parser >> cmd)) continue;

            const auto args = obtenerArgumentos(parser);

            if (cmd.find_first_not_of("0123456789") == std::string::npos) {
                validarCantidadArgs(args, 0);
                procesarOpcion(std::stoi(cmd), in, out);
                continue;
            }

            if (cmd == "salir") {
                validarCantidadArgs(args, 0);
                salir_ = true;
                break;
            } else if (cmd == "ayuda") {
                validarCantidadArgs(args, 0);
                mostrarMenu(out);
            } else if (cmd == "nuevo") {
                validarCantidadArgs(args, 0);
                actual_ = constructor_.crear();
                etiquetaActual_ = "Nuevo automata vacio";
            } else if (cmd == "ejemplo") {
                validarCantidadArgs(args, 0);
                actual_ = constructor_.ejemplo();
                etiquetaActual_ = "Ejemplo inicial (AFND)";
            } else if (cmd == "estado") {
                validarCantidadArgs(args, 2);
                constructor_.agregarEstado(actual_, args[0], parsearBooleano(args[1]));
            } else if (cmd == "inicial") {
                validarCantidadArgs(args, 1);
                constructor_.establecerInicial(actual_, args[0]);
            } else if (cmd == "final") {
                validarCantidadArgs(args, 2);
                constructor_.establecerFinal(actual_, args[0], parsearBooleano(args[1]));
            } else if (cmd == "transicion") {
                if (args.size() < 2) throw std::invalid_argument("Faltan origen y simbolo");
                constructor_.agregarTransicion(actual_, args[0], args[1], {args.begin() + 2, args.end()});
            } else if (cmd == "renombrar") {
                validarCantidadArgs(args, 2);
                auto* e = actual_.buscarEstado(args[0]);
                if (!e) throw std::invalid_argument("Estado inexistente");
                e->setId(args[1]);
            } else if (cmd == "eliminar") {
                validarCantidadArgs(args, 1);
                out << (actual_.eliminarEstado(args[0]) ? "Eliminado.\n" : "No existe.\n");
            } else if (cmd == "alfabeto") {
                actual_.setAlfabeto({args.begin(), args.end()});
            } else if (cmd == "cadena") {
                validarCantidadArgs(args, 1);
                out << (actual_.validarCadena(args[0]) ? "ACEPTADA\n" : "RECHAZADA\n");
            } else if (cmd == "cadenas") {
                validarCantidadArgs(args, 1);
                auto cadenas = archivos_.cargarCadenas(args[0]);
                out << "\nProcesando lote de " << cadenas.size() << " cadenas desde \"" << args[0] << "\":\n";
                out << "--------------------------------------------------------\n";
                std::size_t aceptadas = 0;
                for (std::size_t i = 0; i < cadenas.size(); ++i) {
                    bool ok = actual_.validarCadena(cadenas[i]);
                    if (ok) ++aceptadas;
                    out << "[" << (i + 1) << "] \"" << cadenas[i] << "\": " << (ok ? "ACEPTADA" : "RECHAZADA") << '\n';
                }
                out << "--------------------------------------------------------\n";
                out << "Resumen: " << aceptadas << " aceptadas, " << (cadenas.size() - aceptadas) << " rechazadas.\n";
            } else if (cmd == "mostrar") {
                validarCantidadArgs(args, 0);
                mostrar(out);
            } else if (cmd == "validar") {
                validarCantidadArgs(args, 0);
                actual_.validar();
                out << (actual_.esDeterminista() ? "AFD valido\n" : "AFN/AFN-epsilon valido\n");
            } else if (cmd == "probar") {
                out << (actual_.acepta(args) ? "ACEPTADA\n" : "RECHAZADA\n");
            } else if (cmd == "determinizar") {
                validarCantidadArgs(args, 0);
                comparacion_ = std::move(actual_.clonar());
                etiquetaComparacion_ = "Previo a determinizacion (" + etiquetaActual_ + ")";
                actual_ = conversor_.convertirA_AFD(actual_);
                etiquetaActual_ = "Convertido a AFD (basado en " + etiquetaComparacion_ + ")";
                mostrar(out);
            } else if (cmd == "minimizar") {
                validarCantidadArgs(args, 0);
                comparacion_ = std::move(actual_.clonar());
                etiquetaComparacion_ = "Previo a minimizacion (" + etiquetaActual_ + ")";
                actual_ = minimizador_.minimizar(actual_);
                etiquetaActual_ = "Minimizado (basado en " + etiquetaComparacion_ + ")";
                mostrar(out);
            } else if (cmd == "guardar") {
                validarCantidadArgs(args, 1);
                archivos_.guardarAutomata(actual_, args[0]);
                out << "Guardado.\n";
            } else if (cmd == "cargar") {
                validarCantidadArgs(args, 1);
                actual_ = archivos_.cargarAutomata(args[0]);
                etiquetaActual_ = "Cargado desde " + args[0];
                out << "Cargado.\n";
            } else if (cmd == "equivalencia") {
                validarCantidadArgs(args, 1);
                auto otro = archivos_.cargarAutomata(args[0]);
                const auto r = equivalencia_.comparar(actual_, otro);
                out << (r.equivalentes ? "EQUIVALENTES" : "NO EQUIVALENTES")
                    << " (pares explorados: " << r.paresExplorados << ")\n";
                if (!r.equivalentes) {
                    out << "Contraejemplo: ";
                    if (r.contraejemplo.empty()) out << "epsilon";
                    for (const auto& s : r.contraejemplo) out << std::quoted(s) << ' ';
                    out << '\n';
                }
            } else {
                throw std::invalid_argument("Comando desconocido; usa ayuda");
            }
        } catch (const std::exception& e) {
            out << "Error: " << e.what() << '\n';
        }
    }
}

void InterfazUsuario::iniciar() { ejecutar(std::cin, std::cout); }
void InterfazUsuario::mostrarMenu() const { mostrarMenu(std::cout); }

void InterfazUsuario::mostrarMenu(std::ostream& out) const {
    out << "0 Salir | 1 Mostrar | 2 Nuevo | 3 Cargar | 4 Guardar | 5 Determinizar\n"
           "6 Minimizar | 7 Equivalencia exacta | 8 Validar cadena | 9 Ejemplo\n"
           "10 Quitar inaccesibles | 11 Particiones | 12 Pruebas por longitud | 13 Lote cadenas\n"
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
    case 2: 
        actual_ = constructor_.crear(); 
        etiquetaActual_ = "Nuevo automata vacio";
        out << "Automata vacio creado. Usa estado e inicial para construirlo.\n"; 
        break;
    case 3: {
        std::string ruta = pedir("Ruta (sin comillas): ");
        actual_ = archivos_.cargarAutomata(ruta);
        etiquetaActual_ = "Cargado desde " + ruta;
        out << "Cargado exitosamente.\n";
        break;
    }
    case 4: 
        archivos_.guardarAutomata(actual_, pedir("Ruta (sin comillas): ")); 
        out << "Guardado.\n"; 
        break;
    case 5: 
        comparacion_ = std::move(actual_.clonar());
        etiquetaComparacion_ = "Previo a determinizacion (" + etiquetaActual_ + ")";
        actual_ = conversor_.convertirA_AFD(actual_); 
        etiquetaActual_ = "Convertido a AFD (basado en " + etiquetaComparacion_ + ")";
        mostrar(out); 
        break;
    case 6: 
        comparacion_ = std::move(actual_.clonar());
        etiquetaComparacion_ = "Previo a minimizacion (" + etiquetaActual_ + ")";
        actual_ = minimizador_.minimizar(actual_); 
        etiquetaActual_ = "Minimizado (basado en " + etiquetaComparacion_ + ")";
        mostrar(out); 
        break;
    case 7: {
        out << "\n--- MODULO DE COMPARACION DE EQUIVALENCIA ---\n";
        out << "1. Comparar automata actual [" << etiquetaActual_ 
            << "] contra la version anterior [" << etiquetaComparacion_ << "]\n";
        out << "2. Comparar automata actual [" << etiquetaActual_ 
            << "] contra un archivo externo\n";
        
        std::string subOpcion = pedir("Seleccione opcion (1/2): ");
        Automata otro;
        std::string etiquetaOtro;

        if (subOpcion == "1") {
            if (etiquetaComparacion_ == "Ninguno") {
                throw std::runtime_error("No hay un automata previo en el historial para comparar.");
            }
            otro = std::move(comparacion_.clonar());
            etiquetaOtro = etiquetaComparacion_;
        } else {
            std::string ruta = pedir("Ruta del segundo automata: ");
            otro = archivos_.cargarAutomata(ruta);
            etiquetaOtro = "Archivo: " + ruta;
        }

        out << "\nComparando:\n";
        out << "  [A] " << etiquetaActual_ << "\n";
        out << "  [B] " << etiquetaOtro << "\n";

        const auto r = equivalencia_.comparar(actual_, otro);
        out << "\nRESULTADO: " << (r.equivalentes ? "EQUIVALENTES" : "NO EQUIVALENTES")
            << " (pares explorados: " << r.paresExplorados << ")\n";

        if (!r.equivalentes) {
            out << "Contraejemplo que los diferencia: ";
            if (r.contraejemplo.empty()) out << "epsilon (palabra vacia)";
            for (const auto& simbolo : r.contraejemplo) out << std::quoted(simbolo) << ' ';
            out << '\n';
        }
        break;
    }
    case 8: 
        out << (actual_.validarCadena(pedir("Cadena (vacia = epsilon): ")) ? "ACEPTADA\n" : "RECHAZADA\n"); 
        break;
    case 9: 
        actual_ = constructor_.ejemplo(); 
        etiquetaActual_ = "Ejemplo inicial (AFND)";
        break;
    case 10: 
        actual_ = minimizador_.eliminarEstadosInaccesibles(actual_); 
        etiquetaActual_ += " (sin estados inaccesibles)";
        mostrar(out); 
        break;
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
        
        auto alfabeto = actual_.getAlfabeto(); 
        auto segundo = otro.getAlfabeto();
        alfabeto.insert(segundo.begin(), segundo.end());
        auto palabras = equivalencia_.generarPalabrasDePrueba(alfabeto, longitud);

        bool sonIguales = equivalencia_.probarEquivalenciaPorSimbolos(actual_, otro, palabras);
        out << (sonIguales
            ? "Coinciden en las palabras probadas; no demuestra equivalencia universal.\n"
            : "Difieren en al menos una palabra probada.\n");

        std::string guardarRuta = pedir("Guardar palabras en archivo (vacio para omitir, ej. 'generadas.txt'): ");
        if (!guardarRuta.empty()) {
            archivos_.guardarCadenas(palabras, guardarRuta);
            out << "Se guardaron " << palabras.size() << " cadenas en 'cadenas/" << guardarRuta << "'.\n";
        }
        break; 
    }
    case 13: {
        std::string ruta = pedir("Nombre del archivo de cadenas (en carpeta 'cadenas'): ");
        auto cadenas = archivos_.cargarCadenas(ruta);
        out << "\nProcesando lote de " << cadenas.size() << " cadenas sobre [" << etiquetaActual_ << "]:\n";
        out << "--------------------------------------------------------\n";
        std::size_t aceptadas = 0;
        for (std::size_t i = 0; i < cadenas.size(); ++i) {
            bool ok = actual_.validarCadena(cadenas[i]);
            if (ok) ++aceptadas;
            out << "[" << (i + 1) << "] \"" << cadenas[i] << "\": " << (ok ? "ACEPTADA" : "RECHAZADA") << '\n';
        }
        out << "--------------------------------------------------------\n";
        out << "Resumen: " << aceptadas << " aceptadas, " << (cadenas.size() - aceptadas) << " rechazadas.\n";
        break;
    }
    default: throw std::invalid_argument("Opcion inexistente");
    }
}