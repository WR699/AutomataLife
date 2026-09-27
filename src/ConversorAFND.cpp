#include "ConversorAFND.h"
#include "AlgoritmosInternos.h"
#include <iostream>
#include <cstdio>


Automata ConversorAFND::convertirA_AFD(const Automata& origen,
    const std::set<std::string>& adicional) const {
    origen.validar();

    /// CODIGO VIEJO: devolvia directamente el automata si ya era determinista.
    /*
    if (origen.esDeterminista() && adicional.empty()) {
        return origen.clonar();
    }
    */

    /// CODIGO VIEJO: construccion por subconjuntos anterior.
    /*
    auto alfabeto = origen.getAlfabeto();
    alfabeto.insert(adicional.begin(), adicional.end());
    alfabeto.erase("");

    Automata dfa;
    dfa.setAlfabeto(alfabeto);
    std::map<detalle::Conjunto, std::string> nombres;
    std::vector<detalle::Conjunto> pendientes;

    auto registrar = [&](const detalle::Conjunto& conjunto) -> std::string {
        if (conjunto.empty()) {
            if (!dfa.getEstadoError()) {
                throw std::logic_error("SR aun no disponible al registrar el conjunto vacio");
            }
            return Automata::ID_ESTADO_ERROR;
        }

        auto it = nombres.find(conjunto);
        if (it != nombres.end()) return it->second;

        std::string nombre = "D" + std::to_string(nombres.size());
        nombres.emplace(conjunto, nombre);
        pendientes.push_back(conjunto);
        dfa.agregarEstado(nombre, detalle::final(origen, conjunto));
        return nombre;
    };

    dfa.setEstadoInicial(registrar(detalle::cierre(origen, {origen.getEstadoInicial()->getId()})));

    for (std::size_t i = 0; i < pendientes.size(); ++i) {
        const auto conjunto = pendientes[i];
        const auto nombre = nombres.at(conjunto);

        for (const auto& simbolo : alfabeto) {
            const auto destino = registrar(detalle::mover(origen, conjunto, simbolo));
            if (destino != Automata::ID_ESTADO_ERROR) {dfa.agregarTransicion(nombre, simbolo, {destino});}
        }
    }

    return dfa;
    */

    // CODIGO NUEVO: trabajar sobre un clon y ejecutar agregarEstadosCompuestos.
    Automata dfa = origen.clonar();
    auto alfabeto = dfa.getAlfabeto();
    alfabeto.insert(adicional.begin(), adicional.end());
    alfabeto.erase("");
    dfa.setAlfabeto(alfabeto);

    agregarEstadosCompuestos(dfa);
    return dfa;
}


// CODIGO VIEJO: los compuestos se identificaban con strings como "q1+q2"
// y se iban creando mientras se recorria una lista de pendientes.
// Se conserva desactivado para poder comparar con la version nueva.
#if 0
std::map<std::string, std::string>
ConversorAFND::agregarEstadosCompuestos(Automata& automata) const {

    // CODIGO VIEJO: primero completaba SR y recien despues se trataba el resto.
    /*
    automata.completarEstadoError();
    */

    // CODIGO NUEVO: primero se quitan las epsilon y recien despues se completa SR.
    quitarTransicionesEpsilon(automata);
    automata.completarEstadoError();

    const auto alfabeto = automata.getAlfabeto();
    const std::size_t cantidadEstadosInicial = automata.getCantidadEstados();

    std::map<std::string, std::string> nuevosEstados;
    std::vector<std::string> pendientes;

    for (Estado* estado : automata.getEstados()) {
        pendientes.push_back(estado->getId());
    }

    std::size_t siguienteNumero = 0;

    for (std::size_t i = 0; i < pendientes.size(); i++) {

        Estado* estado = automata.buscarEstado(pendientes[i]);
        if (estado == nullptr) {continue;}

        // Con SR, un estado determinista completo tiene
        // exactamente una transición por símbolo.
        if (estado->getTransiciones().size() <= alfabeto.size()) {continue;}

        // simbolo -> destinos.
        // Epsilon simplemente se almacena como "".
        std::map<std::string, std::set<std::string>> transiciones;

        for (const auto& transicion : estado->getTransiciones()) {
            Estado* destino = transicion.getDestino();
            if (destino == nullptr) {continue;}

            transiciones[transicion.getSimbolo()].insert(destino->getId());
        }

        for (const auto& grupo : transiciones) {

            const std::set<std::string>& destinos = grupo.second;

            // Si un símbolo tiene un solo destino,
            // no hace falta crear estado compuesto.
            if (destinos.size() < 2) {continue;}

            // Nombre lógico del conjunto: q1+q2+SR
            std::string composicion;

            for (const std::string& id : destinos) {
                if (!composicion.empty()) {composicion += "+";}
                composicion += id;
            }

            // Si este mismo conjunto ya tiene estado compuesto, no repetirlo.
            if (nuevosEstados.count(composicion)) {continue;}

            // Nombre real del nuevo estado:
            // S + cantidadEstadosInicial + contador
            std::string nombreNuevo;

            do {
                nombreNuevo = "S" + std::to_string(cantidadEstadosInicial + siguienteNumero);
                siguienteNumero++;
            } while (automata.buscarEstado(nombreNuevo) != nullptr);

            // El compuesto es final si alguno de sus componentes es final.
            bool esFinal = false;

            for (const std::string& id : destinos) {
                Estado* componente = automata.buscarEstado(id);

                if (componente != nullptr && componente->esEstadoFinal()) {
                    esFinal = true;
                    break;
                }
            }

            automata.agregarEstado(nombreNuevo, esFinal);
            nuevosEstados[composicion] = nombreNuevo;

            // El nuevo estado recibe todas las transiciones
            // de todos los estados que lo componen.
            for (const std::string& id : destinos) {

                Estado* componente = automata.buscarEstado(id);
                if (componente == nullptr) {
                    throw std::logic_error("Estado componente inexistente: " + id);
                }

                for (const auto& salida : componente->getTransiciones()) {
                    Estado* destino = salida.getDestino();
                    if (destino == nullptr) {continue;}

                    automata.agregarTransicion(nombreNuevo, salida.getSimbolo(), {destino->getId()});
                }
            }

            // El nuevo estado puede tener a su vez símbolos repetidos.
            pendientes.push_back(nombreNuevo);
        }
    }

    // Primero se termino de armar la tabla completa de equivalencias.
    // Recién ahora se cambian las transiciones multiples por el nombre S correspondiente.
    reemplazarTransicionesCompuestas(automata, nuevosEstados);

    return nuevosEstados;
}

void ConversorAFND::reemplazarTransicionesCompuestas(
    Automata& automata,
    const std::map<std::string, std::string>& nuevosEstados) const {

    // CODIGO VIEJO: primero calculaba un mapa de reemplazos, despues recorria
    // otra vez las transiciones y finalmente aplicaba todas las tablas al final.
    /*

        // Los cambios se preparan primero y se aplican despues, como si se hiciera
        // una segunda pasada manual sobre la tabla ya terminada.
        std::map<std::string, std::vector<Transicion>> tablasNuevas;

        for (Estado* estado : automata.getEstados()) {
            if (estado == nullptr) {continue;}

            // simbolo -> conjunto de destinos actuales.
            std::map<std::string, std::set<std::string>> transiciones;
            for (const auto& transicion : estado->getTransiciones()) {
                Estado* destino = transicion.getDestino();
                if (destino == nullptr) {continue;}
                transiciones[transicion.getSimbolo()].insert(destino->getId());
            }

            // simbolo -> nombre S que reemplaza al conjunto de destinos.
            std::map<std::string, std::string> reemplazos;
            for (const auto& grupo : transiciones) {
                if (grupo.second.size() < 2) {continue;}

                std::string composicion;
                for (const std::string& id : grupo.second) {
                    if (!composicion.empty()) {composicion += "+";}
                    composicion += id;
                }

                auto compuesto = nuevosEstados.find(composicion);
                if (compuesto == nuevosEstados.end()) {
                    throw std::logic_error("No existe estado compuesto para: " + composicion);
                }

                reemplazos[grupo.first] = compuesto->second;
            }

            if (reemplazos.empty()) {continue;}

            std::vector<Transicion> nuevasTransiciones;
            std::set<std::string> simbolosReemplazados;

            for (const auto& transicion : estado->getTransiciones()) {
                auto reemplazo = reemplazos.find(transicion.getSimbolo());

                if (reemplazo == reemplazos.end()) {
                    nuevasTransiciones.emplace_back(transicion.getSimbolo(), transicion.getDestino());
                    continue;
                }

                // Varias transiciones del mismo simbolo pasan a ser UNA sola hacia Sx.
                if (!simbolosReemplazados.insert(transicion.getSimbolo()).second) {continue;}

                Estado* destinoNuevo = automata.buscarEstado(reemplazo->second);
                if (destinoNuevo == nullptr) {
                    throw std::logic_error("Estado compuesto inexistente: " + reemplazo->second);
                }

                nuevasTransiciones.emplace_back(transicion.getSimbolo(), destinoNuevo);
            }

            tablasNuevas[estado->getId()] = std::move(nuevasTransiciones);
        }

        // Aplicar las tablas sin llamar completarEstadoError(): esta etapa solo cambia
        // nombres/destinos y no simplifica ni reinterpreta ninguna transicion.
        for (auto& cambio : tablasNuevas) {
            Estado* estado = automata.buscarEstado(cambio.first);
            if (estado == nullptr) {throw std::logic_error("Estado inexistente al reemplazar transiciones: " + cambio.first);}
            estado->transiciones_ = std::move(cambio.second);
        }

        automata.vincular();
    */

    const std::size_t cantidadSimbolos = automata.getAlfabeto().size();

    for (Estado* estado : automata.getEstados()) {
        if (estado == nullptr) {continue;}

        // En este punto ya se quitaron epsilon y se completo SR.
        // Si no hay mas transiciones que simbolos, no hay simbolos repetidos.
        if (estado->getTransiciones().size() <= cantidadSimbolos) {continue;}

        // Primero agrupamos las transiciones actuales por simbolo.
        std::map<std::string, std::set<std::string>> transiciones;
        for (const auto& transicion : estado->getTransiciones()) {
            Estado* destino = transicion.getDestino();
            if (destino == nullptr) {continue;}
            transiciones[transicion.getSimbolo()].insert(destino->getId());
        }

        std::vector<Transicion> nuevasTransiciones;

        // En esta misma pasada decidimos si conservar el destino o cambiarlo por Sx.
        for (const auto& grupo : transiciones) {
            const std::string& simbolo = grupo.first;
            const std::set<std::string>& destinos = grupo.second;

            if (destinos.size() == 1) {
                Estado* destino = automata.buscarEstado(*destinos.begin());
                if (destino == nullptr) {throw std::logic_error("Destino inexistente al reemplazar transiciones");}
                nuevasTransiciones.emplace_back(simbolo, destino);
                continue;
            }

            std::string composicion;
            for (const std::string& id : destinos) {
                if (!composicion.empty()) {composicion += "+";}
                composicion += id;
            }

            auto compuesto = nuevosEstados.find(composicion);
            if (compuesto == nuevosEstados.end()) {
                throw std::logic_error("No existe estado compuesto para: " + composicion);
            }

            Estado* destinoNuevo = automata.buscarEstado(compuesto->second);
            if (destinoNuevo == nullptr) {
                throw std::logic_error("Estado compuesto inexistente: " + compuesto->second);
            }

            nuevasTransiciones.emplace_back(simbolo, destinoNuevo);
        }

        // Ya terminamos de leer este estado, asi que ahora es seguro reemplazar su tabla.
        estado->transiciones_ = std::move(nuevasTransiciones);
    }

    automata.vincular();
}

// CODIGO VIEJO: convertirA_AFD() solo reenviaba la llamada a convertir().
// Ahora convertirA_AFD() contiene directamente toda la conversion.
/*
Automata ConversorAFND::convertirA_AFD(const Automata& a) const { return convertir(a); }
*/

#endif

std::map<std::set<std::string>, std::string>
ConversorAFND::agregarEstadosCompuestos(Automata& automata) const {
    quitarTransicionesEpsilon(automata);
    automata.completarEstadoError();

    const auto alfabeto = automata.getAlfabeto();
    const std::size_t cantidadEstadosInicial = automata.getCantidadEstados();
    std::map<std::set<std::string>, std::string> nuevosEstados;
    std::size_t siguienteNumero = 0;

    // La tabla se cierra por completo ANTES de reemplazar ninguna transicion.
    // Si una vuelta crea un compuesto nuevo, se vuelve a recorrer todo.
    bool seAgregoEstado = true;
    while (seAgregoEstado) {
        seAgregoEstado = false;
        const auto estadosActuales = automata.getEstados();

        for (Estado* estado : estadosActuales) {
            if (estado == nullptr) {continue;}
            if (estado->getTransiciones().size() <= alfabeto.size()) {continue;}

            std::map<std::string, std::set<std::string>> transiciones;
            for (const auto& transicion : estado->getTransiciones()) {
                Estado* destino = transicion.getDestino();
                if (destino == nullptr) {continue;}
                transiciones[transicion.getSimbolo()].insert(destino->getId());
            }

            for (const auto& grupo : transiciones) {
                const std::set<std::string>& destinos = grupo.second;
                if (destinos.size() < 2) {continue;}
                if (nuevosEstados.count(destinos)) {continue;}

                std::string nombreNuevo;
                do {
                    nombreNuevo = "S" + std::to_string(cantidadEstadosInicial + siguienteNumero);
                    siguienteNumero++;
                } while (automata.buscarEstado(nombreNuevo) != nullptr);

                bool esFinal = false;
                std::string nombreVisible;
                for (const std::string& id : destinos) {
                    Estado* componente = automata.buscarEstado(id);
                    if (componente == nullptr) {throw std::logic_error("Estado componente inexistente: " + id);}
                    if (componente->esEstadoFinal()) {esFinal = true;}
                    if (!nombreVisible.empty()) {nombreVisible += "+";}
                    nombreVisible += componente->getNombreVisible();
                }

                auto& compuesto = automata.agregarEstado(nombreNuevo, esFinal);
                compuesto.setNombreVisible(nombreVisible);
                nuevosEstados[destinos] = nombreNuevo;

                for (const std::string& id : destinos) {
                    Estado* componente = automata.buscarEstado(id);
                    if (componente == nullptr) {throw std::logic_error("Estado componente inexistente: " + id);}

                    for (const auto& salida : componente->getTransiciones()) {
                        Estado* destino = salida.getDestino();
                        if (destino == nullptr) {continue;}
                        automata.agregarTransicion(nombreNuevo, salida.getSimbolo(), {destino->getId()});
                    }
                }

                seAgregoEstado = true;
            }
        }
    }

    reemplazarTransicionesCompuestas(automata, nuevosEstados);
    return nuevosEstados;
}

void ConversorAFND::reemplazarTransicionesCompuestas(
    Automata& automata,
    const std::map<std::set<std::string>, std::string>& nuevosEstados) const {

    const std::size_t cantidadSimbolos = automata.getAlfabeto().size();

    for (Estado* estado : automata.getEstados()) {
        if (estado == nullptr) {continue;}
        if (estado->getTransiciones().size() <= cantidadSimbolos) {continue;}

        std::map<std::string, std::set<std::string>> transiciones;
        for (const auto& transicion : estado->getTransiciones()) {
            Estado* destino = transicion.getDestino();
            if (destino == nullptr) {continue;}
            transiciones[transicion.getSimbolo()].insert(destino->getId());
        }

        std::vector<Transicion> nuevasTransiciones;

        for (const auto& grupo : transiciones) {
            const std::string& simbolo = grupo.first;
            const std::set<std::string>& destinos = grupo.second;

            if (destinos.size() == 1) {
                Estado* destino = automata.buscarEstado(*destinos.begin());
                if (destino == nullptr) {throw std::logic_error("Destino inexistente al reemplazar transiciones");}
                nuevasTransiciones.emplace_back(simbolo, destino);
                continue;
            }

            auto compuesto = nuevosEstados.find(destinos);
            if (compuesto == nuevosEstados.end()) {
                throw std::logic_error("No existe estado compuesto para el conjunto solicitado");
            }

            Estado* destinoNuevo = automata.buscarEstado(compuesto->second);
            if (destinoNuevo == nullptr) {throw std::logic_error("Estado compuesto inexistente: " + compuesto->second);}
            nuevasTransiciones.emplace_back(simbolo, destinoNuevo);
        }

        estado->transiciones_ = std::move(nuevasTransiciones);
    }

    automata.vincular();
}

void ConversorAFND::quitarTransicionesEpsilon(Automata& automata) const {

    // CODIGO VIEJO: las epsilon se dejaban para despues y agregarEstadosCompuestos()
    // terminaba viendo un automata que seguia siendo AFND por epsilon.
    /*
    automata.completarEstadoError();
    */

    std::map<std::string, std::vector<Transicion>> tablasNuevas;
    std::map<std::string, bool> finalesNuevos;

    for (Estado* estado : automata.getEstados()) {
        if (estado == nullptr) {continue;}

        std::set<Estado*> cierre = clausuraEpsilon({estado});
        std::vector<Transicion> nuevasTransiciones;
        std::set<std::pair<std::string, std::string>> yaAgregadas;
        bool esFinalNuevo = estado->esEstadoFinal();

        for (Estado* componente : cierre) {
            if (componente == nullptr) {continue;}
            if (componente->esEstadoFinal()) {esFinalNuevo = true;}

            for (const auto& transicion : componente->getTransiciones()) {
                if (transicion.esEpsilon()) {continue;}

                Estado* destino = transicion.getDestino();
                if (destino == nullptr) {continue;}

                if (!yaAgregadas.insert({transicion.getSimbolo(), destino->getId()}).second) {continue;}
                nuevasTransiciones.emplace_back(transicion.getSimbolo(), destino);
            }
        }

        tablasNuevas[estado->getId()] = std::move(nuevasTransiciones);
        finalesNuevos[estado->getId()] = esFinalNuevo;
    }

    for (auto& cambio : tablasNuevas) {
        Estado* estado = automata.buscarEstado(cambio.first);
        if (estado == nullptr) {throw std::logic_error("Estado inexistente al quitar epsilon: " + cambio.first);}

        estado->transiciones_ = std::move(cambio.second);
        estado->final_ = finalesNuevos[cambio.first];
    }

    automata.vincular();
}

std::set<Estado*> ConversorAFND::clausuraEpsilon(const std::set<Estado*>& estados) const {
    auto cierre = estados;
    std::vector<Estado*> pendientes(estados.begin(), estados.end());
    for (std::size_t i = 0; i < pendientes.size(); ++i) {
        if (!pendientes[i]) throw std::invalid_argument("Estado nulo en clausura");
        for (const auto& t : pendientes[i]->getTransiciones()) {
            if (t.esEpsilon()) {
                auto* d = t.getDestino();
                if (!d) throw std::logic_error("Transicion epsilon sin destino");
                if (cierre.insert(d).second) pendientes.push_back(d);
            }
        }
    }
    return cierre;
}

// CODIGO VIEJO: mover pertenecia a ConversorAFND. Se movio a Automata sin cambiar su logica.
/*
std::set<Estado*> ConversorAFND::mover(const std::set<Estado*>& estados, const std::string& simbolo) const {
    if (simbolo.empty()) throw std::invalid_argument("Usa clausuraEpsilon para epsilon");
    std::set<Estado*> destinos;
    for (const auto* e : estados) {
        if (!e) throw std::invalid_argument("Estado nulo en mover");
        for (const auto& t : e->getTransiciones()) {
            if (t.getSimbolo() == simbolo && t.getDestino() &&
                t.getDestino()->getId() != Automata::ID_ESTADO_ERROR)
                destinos.insert(t.getDestino());
        }
    }
    return destinos;
}
*/