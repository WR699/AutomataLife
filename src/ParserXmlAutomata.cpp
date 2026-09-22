#include "ParserXmlAutomata.h"
#include "tinyxml2.h"
#include <stdexcept>
#include <vector>

using namespace tinyxml2;

Automata ParserXmlAutomata::cargarDesdeXML(const std::string& rutaArchivo) const {
    XMLDocument doc;
    if (doc.LoadFile(rutaArchivo.c_str()) != XML_SUCCESS) { // Corregido: XML_SUCCESS
        throw std::runtime_error("Error al abrir o parsear el archivo XML: " + rutaArchivo);
    }

    XMLElement* root = doc.FirstChildElement("automata");
    if (!root) {
        throw std::runtime_error("Formato XML invalido: No se encontro la etiqueta raiz <automata>");
    }

    Automata a;

    // 1. Cargar Alfabeto
    XMLElement* elemAlfabeto = root->FirstChildElement("alfabeto");
    if (elemAlfabeto) {
        for (XMLElement* eSimbolo = elemAlfabeto->FirstChildElement("simbolo");
            eSimbolo != nullptr;
            eSimbolo = eSimbolo->NextSiblingElement("simbolo")) {

            const char* texto = eSimbolo->GetText();
            if (texto) {
                a.agregarSimbolo(std::string(texto));
            }
        }
    }

    // 2. Cargar Estados
    XMLElement* elemEstados = root->FirstChildElement("estados");
    if (!elemEstados) {
        throw std::runtime_error("El XML no contiene la seccion <estados>");
    }

    std::string idInicial = "";

    for (XMLElement* eEstado = elemEstados->FirstChildElement("estado");
        eEstado != nullptr;
        eEstado = eEstado->NextSiblingElement("estado")) {

        const char* idAttr = eEstado->Attribute("id");
        if (!idAttr) {
            throw std::runtime_error("Estado encontrado sin atributo 'id'");
        }
        std::string id(idAttr);

        bool esFinal = eEstado->BoolAttribute("final", false);
        bool esInicial = eEstado->BoolAttribute("inicial", false);

        a.agregarEstado(id, esFinal);

        if (esInicial) {
            if (!idInicial.empty()) {
                throw std::runtime_error("Se definio mas de un estado inicial en el XML");
            }
            idInicial = id;
        }
    }

    if (!idInicial.empty()) {
        a.establecerInicial(idInicial);
    }

    // 3. Cargar Transiciones
    XMLElement* elemTransiciones = root->FirstChildElement("transiciones");
    if (elemTransiciones) {
        for (XMLElement* eTrans = elemTransiciones->FirstChildElement("transicion");
            eTrans != nullptr;
            eTrans = eTrans->NextSiblingElement("transicion")) {

            const char* origenAttr = eTrans->Attribute("origen");
            if (!origenAttr) throw std::runtime_error("Transicion sin atributo 'origen'");
            std::string origen(origenAttr);

            const char* simboloAttr = eTrans->Attribute("simbolo");
            std::string simbolo = simboloAttr ? std::string(simboloAttr) : ""; // Vacio representa Epsilon

            std::vector<std::string> destinos;
            for (XMLElement* eDestino = eTrans->FirstChildElement("destino");
                eDestino != nullptr;
                eDestino = eDestino->NextSiblingElement("destino")) {

                const char* destText = eDestino->GetText();
                if (destText) {
                    destinos.push_back(std::string(destText));
                }
            }

            if (!destinos.empty()) {
                a.agregarTransicion(origen, simbolo, destinos);
            }
        }
    }

    // Validar el automata cargado
    a.validar();
    return a;
}

void ParserXmlAutomata::guardarEnXML(const Automata& a, const std::string& rutaArchivo) const {
    a.validar();

    XMLDocument doc;
    XMLElement* root = doc.NewElement("automata");
    doc.InsertFirstChild(root);

    // 1. Guardar Alfabeto
    XMLElement* elemAlfabeto = doc.NewElement("alfabeto");
    for (const auto& simbolo : a.getAlfabeto()) {
        XMLElement* eSimbolo = doc.NewElement("simbolo");
        eSimbolo->SetText(simbolo.c_str());
        elemAlfabeto->InsertEndChild(eSimbolo);
    }
    root->InsertEndChild(elemAlfabeto);

    // 2. Guardar Estados
    XMLElement* elemEstados = doc.NewElement("estados");
    for (const auto& e : a.getEstados()) { // Corregido: getEstados() o iteración sobre estados_
        XMLElement* eEstado = doc.NewElement("estado");
        eEstado->SetAttribute("id", e->getId().c_str()); // Corregido: getId()
        eEstado->SetAttribute("inicial", e->esEstadoInicial()); // Corregido: esEstadoInicial()
        eEstado->SetAttribute("final", e->esEstadoFinal()); // Corregido: esEstadoFinal()
        elemEstados->InsertEndChild(eEstado);
    }
    root->InsertEndChild(elemEstados);

    // 3. Guardar Transiciones
    XMLElement* elemTransiciones = doc.NewElement("transiciones");
    for (const auto& e : a.getEstados()) {
        for (const auto& t : e->getTransiciones()) { // Corregido: getTransiciones()
            XMLElement* eTrans = doc.NewElement("transicion");
            eTrans->SetAttribute("origen", e->getId().c_str()); // Corregido: getId()
            eTrans->SetAttribute("simbolo", t.getSimbolo().c_str()); // Corregido: getSimbolo()

            for (const auto* d : t.getDestinos()) { // Corregido: getDestinos()
                XMLElement* eDestino = doc.NewElement("destino");
                eDestino->SetText(d->getId().c_str()); // Corregido: getId()
                eTrans->InsertEndChild(eDestino);
            }
            elemTransiciones->InsertEndChild(eTrans);
        }
    }
    root->InsertEndChild(elemTransiciones);

    if (doc.SaveFile(rutaArchivo.c_str()) != XML_SUCCESS) { // Corregido: XML_SUCCESS
        throw std::runtime_error("Error al guardar el archivo XML: " + rutaArchivo);
    }
}