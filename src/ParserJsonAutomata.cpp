#include "ParserJsonAutomata.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <regex>
#include <map>

namespace {
std::string leerArchivoCompleto(const std::string& ruta) {
    std::ifstream in(ruta);
    if (!in) throw std::runtime_error("No se pudo abrir el archivo JSON: " + ruta);
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}
}

Automata ParserJsonAutomata::cargarDesdeJSON(const std::string& ruta) const {
    std::string contenido = leerArchivoCompleto(ruta);
    Automata a;

    // 1. Extraer símbolos del alfabeto
    std::regex regAlfabeto("\"alfabeto\"\\s*:\\s*\\[([^\\]]*)\\]");
    std::smatch matchAlfabeto;
    if (std::smatch m; std::regex_search(contenido, m, regAlfabeto)) {
        std::regex regString("\"([^\"]+)\"");
        std::string bloque = m[1].str();
        auto it = std::sregex_iterator(bloque.begin(), bloque.end(), regString);
        auto end = std::sregex_iterator();
        std::set<std::string> alfabeto;
        for (; it != end; ++it) alfabeto.insert((*it)[1].str());
        a.setAlfabeto(alfabeto);
    }

    // 2. Extraer estados
    std::regex regEstado("\\{\\s*\"id\"\\s*:\\s*\"([^\"]+)\"\\s*,\\s*\"inicial\"\\s*:\\s*(true|false)\\s*,\\s*\"final\"\\s*:\\s*(true|false)\\s*\\}");
    auto itE = std::sregex_iterator(contenido.begin(), contenido.end(), regEstado);
    auto endE = std::sregex_iterator();
    std::map<std::string, std::string> nombreAId;
    std::string inicialVisible;
    std::size_t numeroEstado = 1;

    for (; itE != endE; ++itE) {
        std::string nombreVisible = (*itE)[1].str();
        bool inicial = ((*itE)[2].str() == "true");
        bool esFinal = ((*itE)[3].str() == "true");

        if (nombreVisible != Automata::ID_ESTADO_ERROR) {
            const std::string idInterno = "S" + std::to_string(numeroEstado++);
            if (!nombreAId.emplace(nombreVisible, idInterno).second)
                throw std::runtime_error("Estado JSON duplicado: " + nombreVisible);

            // CODIGO VIEJO: el id JSON tambien era el ID interno.
            // a.agregarEstado(nombreVisible, esFinal);
            auto& estado = a.agregarEstado(idInterno, esFinal);
            estado.setNombreVisible(nombreVisible);

            if (inicial) {
                if (!inicialVisible.empty()) throw std::runtime_error("Se definio mas de un estado inicial en JSON");
                inicialVisible = nombreVisible;
            }
        }
    }

    if (!inicialVisible.empty()) a.setEstadoInicial(nombreAId.at(inicialVisible));

    // 3. Extraer transiciones
    std::regex regTransicion("\\{\\s*\"origen\"\\s*:\\s*\"([^\"]+)\"\\s*,\\s*\"simbolo\"\\s*:\\s*\"([^\"]*)\"\\s*,\\s*\"destino\"\\s*:\\s*\"([^\"]+)\"\\s*\\}");
    auto itT = std::sregex_iterator(contenido.begin(), contenido.end(), regTransicion);
    auto endT = std::sregex_iterator();
    for (; itT != endT; ++itT) {
        std::string origen = (*itT)[1].str();
        std::string simbolo = (*itT)[2].str();
        std::string destino = (*itT)[3].str();

        if (origen != Automata::ID_ESTADO_ERROR && destino != Automata::ID_ESTADO_ERROR) {
            auto o = nombreAId.find(origen);
            auto d = nombreAId.find(destino);
            if (o == nombreAId.end()) throw std::runtime_error("Origen JSON inexistente: " + origen);
            if (d == nombreAId.end()) throw std::runtime_error("Destino JSON inexistente: " + destino);
            a.agregarTransicion(o->second, simbolo, {d->second});
        }
    }

    a.completarEstadoError();
    a.validar();
    return a;
}

void ParserJsonAutomata::guardarEnJSON(const Automata& a, const std::string& ruta) const {
    a.validar();
    std::ofstream out(ruta);
    if (!out) throw std::runtime_error("No se pudo crear el archivo JSON: " + ruta);

    out << "{\n";
    
    // Alfabeto
    out << "  \"alfabeto\": [";
    std::size_t count = 0;
    auto alf = a.getAlfabeto();
    for (const auto& s : alf) {
        out << "\"" << s << "\"" << (++count < alf.size() ? ", " : "");
    }
    out << "],\n";

    // Estados
    out << "  \"estados\": [\n";
    const auto* sr = a.getEstadoError();
    std::vector<const Estado*> estadosValidos;
    for (const auto* e : a.getEstados()) if (e != sr) estadosValidos.push_back(e);

    for (std::size_t i = 0; i < estadosValidos.size(); ++i) {
        const auto* e = estadosValidos[i];
        // CODIGO VIEJO: se guardaba el ID interno.
        // out << "    { \"id\": \"" << e->getId() << "\", \"inicial\": "
        out << "    { \"id\": \"" << e->getNombreVisible() << "\", \"inicial\": "
            << (e == a.getEstadoInicial() ? "true" : "false")
            << ", \"final\": " << (e->esEstadoFinal() ? "true" : "false") << " }"
            << (i + 1 < estadosValidos.size() ? ",\n" : "\n");
    }
    out << "  ],\n";

    // Transiciones
    out << "  \"transiciones\": [\n";
    struct TTuple { std::string o, s, d; };
    std::vector<TTuple> listaT;
    for (const auto* e : a.getEstados()) {
        if (e == sr) continue;
        for (const auto& t : e->getTransiciones()) {
            if (t.getDestino() != sr) {
                listaT.push_back({e->getNombreVisible(), t.getSimbolo(), t.getDestino()->getNombreVisible()});
            }
        }
    }

    for (std::size_t i = 0; i < listaT.size(); ++i) {
        out << "    { \"origen\": \"" << listaT[i].o << "\", \"simbolo\": \""
            << listaT[i].s << "\", \"destino\": \"" << listaT[i].d << "\" }"
            << (i + 1 < listaT.size() ? ",\n" : "\n");
    }
    out << "  ]\n}\n";
}