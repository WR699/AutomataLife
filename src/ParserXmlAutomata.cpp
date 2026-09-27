#include "ParserXmlAutomata.h"
#include "tinyxml2.h"
#include <stdexcept>
#include <map>
#include <string>
#include <vector>

using namespace tinyxml2;

Automata ParserXmlAutomata::cargarDesdeXML(const std::string& rutaArchivo) const {
    XMLDocument doc;
    if (doc.LoadFile(rutaArchivo.c_str()) != XML_SUCCESS)
        throw std::runtime_error("Error al abrir o parsear el archivo XML: " + rutaArchivo);

    XMLElement* root = doc.FirstChildElement("automata");
    if (!root)
        throw std::runtime_error("Formato XML invalido: No se encontro la etiqueta raiz <automata>");

    Automata a;

    if (XMLElement* elemAlfabeto = root->FirstChildElement("alfabeto")) {
        std::set<std::string> alfabeto;
        for (XMLElement* eSimbolo = elemAlfabeto->FirstChildElement("simbolo");
             eSimbolo; eSimbolo = eSimbolo->NextSiblingElement("simbolo")) {
            const char* texto = eSimbolo->GetText();
            if (texto && *texto) alfabeto.insert(texto);
        }
        a.setAlfabeto(alfabeto);
    }

    XMLElement* elemEstados = root->FirstChildElement("estados");
    if (!elemEstados) throw std::runtime_error("El XML no contiene la seccion <estados>");

    std::string idInicialVisible;
    std::map<std::string, std::string> nombreAId;
    std::size_t numeroEstado = 1;

    for (XMLElement* eEstado = elemEstados->FirstChildElement("estado");
         eEstado; eEstado = eEstado->NextSiblingElement("estado")) {
        const char* idAttr = eEstado->Attribute("id");
        if (!idAttr) throw std::runtime_error("Estado encontrado sin atributo 'id'");
        std::string nombreVisible(idAttr);
        if (nombreVisible == Automata::ID_ESTADO_ERROR)
            throw std::runtime_error("El XML no debe declarar SR: se genera automaticamente");

        const bool esFinal = eEstado->BoolAttribute("final", false);
        const bool esInicial = eEstado->BoolAttribute("inicial", false);
        const std::string idInterno = "S" + std::to_string(numeroEstado++);

        if (!nombreAId.emplace(nombreVisible, idInterno).second)
            throw std::runtime_error("Estado XML duplicado: " + nombreVisible);

        // CODIGO VIEJO: el atributo id era tambien el ID interno.
        // a.agregarEstado(nombreVisible, esFinal);
        auto& estado = a.agregarEstado(idInterno, esFinal);
        estado.setNombreVisible(nombreVisible);

        if (esInicial) {
            if (!idInicialVisible.empty()) throw std::runtime_error("Se definio mas de un estado inicial en el XML");
            idInicialVisible = nombreVisible;
        }
    }

    if (!idInicialVisible.empty()) a.establecerInicial(nombreAId.at(idInicialVisible));

    if (XMLElement* elemTransiciones = root->FirstChildElement("transiciones")) {
        for (XMLElement* eTrans = elemTransiciones->FirstChildElement("transicion");
             eTrans; eTrans = eTrans->NextSiblingElement("transicion")) {
            const char* origenAttr = eTrans->Attribute("origen");
            if (!origenAttr) throw std::runtime_error("Transicion sin atributo 'origen'");
            const std::string origenVisible(origenAttr);
            auto origenIt = nombreAId.find(origenVisible);
            if (origenIt == nombreAId.end()) throw std::runtime_error("Origen XML inexistente: " + origenVisible);

            const char* simboloAttr = eTrans->Attribute("simbolo");
            const std::string simbolo = simboloAttr ? std::string(simboloAttr) : "";

            std::vector<std::string> destinos;
            for (XMLElement* eDestino = eTrans->FirstChildElement("destino");
                 eDestino; eDestino = eDestino->NextSiblingElement("destino")) {
                const char* texto = eDestino->GetText();
                if (!texto) {continue;}
                auto destinoIt = nombreAId.find(texto);
                if (destinoIt == nombreAId.end()) throw std::runtime_error("Destino XML inexistente: " + std::string(texto));
                destinos.push_back(destinoIt->second);
            }

            // XML viejo: varios <destino> se separan en varias Transicion individuales.
            a.agregarTransicion(origenIt->second, simbolo, destinos);
        }
    }

    a.completarEstadoError();
    a.validar();
    return a;
}

void ParserXmlAutomata::guardarEnXML(const Automata& a, const std::string& rutaArchivo) const {
    a.validar();

    XMLDocument doc;
    XMLElement* root = doc.NewElement("automata");
    doc.InsertFirstChild(root);

    XMLElement* elemAlfabeto = doc.NewElement("alfabeto");
    for (const auto& simbolo : a.getAlfabeto()) {
        XMLElement* eSimbolo = doc.NewElement("simbolo");
        eSimbolo->SetText(simbolo.c_str());
        elemAlfabeto->InsertEndChild(eSimbolo);
    }
    root->InsertEndChild(elemAlfabeto);

    const auto* sr = a.getEstadoError();

    XMLElement* elemEstados = doc.NewElement("estados");
    for (const auto* e : a.getEstados()) {
        if (e == sr) continue; // SR es interno y se regenera al cargar.
        XMLElement* eEstado = doc.NewElement("estado");
        // CODIGO VIEJO: se imprimia el ID interno.
        // eEstado->SetAttribute("id", e->getId().c_str());
        eEstado->SetAttribute("id", e->getNombreVisible().c_str());
        eEstado->SetAttribute("inicial", e->esEstadoInicial());
        eEstado->SetAttribute("final", e->esEstadoFinal());
        elemEstados->InsertEndChild(eEstado);
    }
    root->InsertEndChild(elemEstados);

    XMLElement* elemTransiciones = doc.NewElement("transiciones");
    for (const auto* e : a.getEstados()) {
        if (e == sr) continue;
        for (const auto& t : e->getTransiciones()) {
            if (t.getDestino() == sr) continue; // fallback interno

            XMLElement* eTrans = doc.NewElement("transicion");
            eTrans->SetAttribute("origen", e->getNombreVisible().c_str());
            eTrans->SetAttribute("simbolo", t.getSimbolo().c_str());

            XMLElement* eDestino = doc.NewElement("destino");
            eDestino->SetText(t.getDestino()->getNombreVisible().c_str());
            eTrans->InsertEndChild(eDestino);
            elemTransiciones->InsertEndChild(eTrans);
        }
    }
    root->InsertEndChild(elemTransiciones);

    if (doc.SaveFile(rutaArchivo.c_str()) != XML_SUCCESS)
        throw std::runtime_error("Error al guardar el archivo XML: " + rutaArchivo);
}
