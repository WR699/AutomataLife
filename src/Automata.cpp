#include "Automata.h"
#include "AlgoritmosInternos.h"
#include <algorithm>
#include <map>
#include <set>
#include <stdexcept>
#include <utility>

namespace {
bool mismaArista(const Transicion& t, const std::string& simbolo, const Estado* destino) {
    return t.getSimbolo() == simbolo && t.getDestino() == destino;
}
}

void Automata::vincular() noexcept {
    for (auto& e : estados_) {
        e->propietario_ = this;
        for (auto& t : e->transiciones_) t.propietario_ = this;
    }
}

Automata::Automata(Automata&& o) noexcept
    : estados_(std::move(o.estados_)),
      inicial_(o.inicial_),
      alfabetoDeclarado_(std::move(o.alfabetoDeclarado_)) {
    o.inicial_ = nullptr;
    vincular();
}

Automata& Automata::operator=(Automata&& o) noexcept {
    if (this != &o) {
        estados_ = std::move(o.estados_);
        inicial_ = o.inicial_;
        alfabetoDeclarado_ = std::move(o.alfabetoDeclarado_);
        o.inicial_ = nullptr;
        vincular();
    }
    return *this;
}

Estado& Automata::asegurarEstadoError() {
    if (auto* sr = buscarEstado(ID_ESTADO_ERROR)) return *sr;

    auto sr = std::make_unique<Estado>(ID_ESTADO_ERROR, false);
    auto* ptr = sr.get();
    ptr->propietario_ = this;
    estados_.push_back(std::move(sr));
    return *ptr;
}

bool Automata::esEstadoError(const Estado* estado) const noexcept {
    return estado && estado->getId() == ID_ESTADO_ERROR;
}

Estado& Automata::agregarEstado(const std::string& n, bool f) {
    return agregarEstado(std::make_unique<Estado>(n, f));
}

Estado& Automata::agregarEstado(std::unique_ptr<Estado> e) {
    if (!e || e->propietario_)
        throw std::invalid_argument("Estado nulo o ya perteneciente a un automata");
    if (e->getId() == ID_ESTADO_ERROR)
        throw std::invalid_argument("El id SR esta reservado para el State Error interno");
    if (buscarEstado(e->getId()))
        throw std::invalid_argument("Id duplicado: " + e->getId());

    for (const auto& t : e->transiciones_) {
        auto* d = t.destino_;
        if (!d || (d != e.get() && !contieneEstado(d)))
            throw std::invalid_argument("Destino ajeno al automata");
        if (!t.esEpsilon()) alfabetoDeclarado_.insert(t.getSimbolo());
    }

    auto* ptr = e.get();
    estados_.push_back(std::move(e));
    ptr->propietario_ = this;
    for (auto& t : ptr->transiciones_) t.propietario_ = this;

    if (ptr->inicial_) setEstadoInicial(ptr);
    completarEstadoError();
    return *ptr;
}

Estado* Automata::buscarEstado(const std::string& n) {
    for (const auto& e : estados_) if (e->getId() == n) return e.get();
    return nullptr;
}

const Estado* Automata::buscarEstado(const std::string& n) const {
    for (const auto& e : estados_) if (e->getId() == n) return e.get();
    return nullptr;
}

std::set<Estado*> Automata::getEstados() {
    std::set<Estado*> s;
    for (const auto& e : estados_) s.insert(e.get());
    return s;
}

std::set<const Estado*> Automata::getEstados() const {
    std::set<const Estado*> s;
    for (const auto& e : estados_) s.insert(e.get());
    return s;
}

void Automata::setEstados(std::vector<std::unique_ptr<Estado>> es) {
    Automata nuevo;
    nuevo.alfabetoDeclarado_ = alfabetoDeclarado_;

    std::set<Estado*> direcciones;
    std::set<std::string> ids;
    for (const auto& e : es) {
        if (!e || e->propietario_ || !ids.insert(e->getId()).second)
            throw std::invalid_argument("Estados nulos, ajenos o duplicados");
        if (e->getId() == ID_ESTADO_ERROR)
            throw std::invalid_argument("El id SR esta reservado para el State Error interno");
        direcciones.insert(e.get());
        if (e->inicial_) {
            if (nuevo.inicial_) throw std::invalid_argument("Mas de un estado inicial");
            nuevo.inicial_ = e.get();
        }
    }

    for (const auto& e : es) {
        for (const auto& t : e->transiciones_) {
            if (!t.destino_ || !direcciones.count(t.destino_))
                throw std::invalid_argument("Destino fuera del nuevo conjunto");
            if (!t.esEpsilon()) nuevo.alfabetoDeclarado_.insert(t.getSimbolo());
        }
    }

    nuevo.estados_ = std::move(es);
    nuevo.vincular();
    nuevo.completarEstadoError();
    *this = std::move(nuevo);
}

bool Automata::contieneEstado(const Estado* e) const noexcept {
    for (const auto& p : estados_) if (p.get() == e) return true;
    return false;
}

bool Automata::eliminarEstado(const std::string& id) {
    if (id == ID_ESTADO_ERROR) return false;
    auto* ptr = buscarEstado(id);
    if (!ptr) return false;

    for (auto& e : estados_) {
        auto& ts = e->transiciones_;
        ts.erase(std::remove_if(ts.begin(), ts.end(),
            [ptr](const Transicion& t) { return t.getDestino() == ptr; }), ts.end());
    }

    if (inicial_ == ptr) inicial_ = nullptr;
    auto it = std::find_if(estados_.begin(), estados_.end(),
        [ptr](const auto& e) { return e.get() == ptr; });
    estados_.erase(it);
    completarEstadoError();
    return true;
}

std::size_t Automata::getCantidadEstados() const noexcept { return estados_.size(); }

Estado* Automata::getEstadoInicial() noexcept { return inicial_; }
const Estado* Automata::getEstadoInicial() const noexcept { return inicial_; }

void Automata::setEstadoInicial(Estado* e) {
    if (e && !contieneEstado(e)) throw std::invalid_argument("Inicial ajeno al automata");
    if (esEstadoError(e)) throw std::invalid_argument("SR no puede ser el estado inicial");
    for (auto& p : estados_) p->inicial_ = (p.get() == e);
    inicial_ = e;
}

void Automata::setEstadoInicial(const std::string& id) {
    auto* e = buscarEstado(id);
    if (!e) throw std::invalid_argument("Inicial inexistente");
    setEstadoInicial(e);
}

void Automata::establecerFinal(const std::string& id, bool f) {
    auto* e = buscarEstado(id);
    if (!e) throw std::invalid_argument("Estado inexistente");
    if (esEstadoError(e) && f) throw std::invalid_argument("SR no puede ser final");
    e->setEstadoFinal(f);
}

std::set<std::string> Automata::getAlfabeto() const {
    auto s = alfabetoDeclarado_;
    const auto* sr = buscarEstado(ID_ESTADO_ERROR);

    // Los enlaces hacia SR son completado interno y no crean simbolos nuevos.
    for (const auto& e : estados_) {
        if (e.get() == sr) continue;
        for (const auto& t : e->transiciones_) {
            if (!t.esEpsilon() && t.getDestino() != sr)
                s.insert(t.getSimbolo());
        }
    }
    return s;
}

void Automata::setAlfabeto(const std::set<std::string>& s) {
    if (s.count("")) throw std::invalid_argument("Epsilon no pertenece al alfabeto");
    const auto* sr = buscarEstado(ID_ESTADO_ERROR);

    for (const auto& e : estados_) {
        if (e.get() == sr) continue;
        for (const auto& t : e->transiciones_) {
            if (!t.esEpsilon() && t.getDestino() != sr && !s.count(t.getSimbolo()))
                throw std::invalid_argument("Falta un simbolo usado en transiciones: " + t.getSimbolo());
        }
    }

    alfabetoDeclarado_ = s;
    completarEstadoError();
}

void Automata::agregarSimbolo(const std::string& s) {
    if (s.empty()) throw std::invalid_argument("Epsilon no pertenece al alfabeto");
    alfabetoDeclarado_.insert(s);
    completarEstadoError();
}

bool Automata::eliminarSimbolo(const std::string& s) {
    auto* sr = buscarEstado(ID_ESTADO_ERROR);
    for (const auto& e : estados_) {
        if (e.get() == sr) continue;
        for (const auto& t : e->transiciones_) {
            if (!t.esEpsilon() && t.getSimbolo() == s && t.getDestino() != sr)
                throw std::invalid_argument("Simbolo en uso");
        }
    }

    const bool eliminado = alfabetoDeclarado_.erase(s) != 0;
    completarEstadoError();
    return eliminado;
}

Transicion& Automata::agregarAristaInterna(Estado& origen, const std::string& simbolo, Estado* destino) {
    if (!contieneEstado(&origen)) throw std::invalid_argument("Origen ajeno al automata");
    if (!destino || !contieneEstado(destino)) throw std::invalid_argument("Destino ajeno al automata");
    if (esEstadoError(&origen)) throw std::invalid_argument("Las transiciones de SR son internas");
    if (esEstadoError(destino)) throw std::invalid_argument("SR es un estado interno y no se usa como destino manual");

    auto* sr = buscarEstado(ID_ESTADO_ERROR);

    if (!simbolo.empty()) {
        const bool nuevoSimbolo = !getAlfabeto().count(simbolo);
        alfabetoDeclarado_.insert(simbolo);

        if (nuevoSimbolo) {
            sr = &asegurarEstadoError();
            // Completar el nuevo simbolo en todos los otros estados.
            for (auto& e : estados_) {
                if (e.get() == &origen || e.get() == sr) continue;
                e->transiciones_.emplace_back(simbolo, sr);
                e->transiciones_.back().propietario_ = this;
            }
            sr->transiciones_.emplace_back(simbolo, sr);
            sr->transiciones_.back().propietario_ = this;
        }

        sr = buscarEstado(ID_ESTADO_ERROR);
        if (sr) {
            auto& ts = origen.transiciones_;
            ts.erase(std::remove_if(ts.begin(), ts.end(), [&](const Transicion& t) {
                return t.getSimbolo() == simbolo && t.getDestino() == sr;
            }), ts.end());
        }
    }

    for (auto& t : origen.transiciones_) {
        if (mismaArista(t, simbolo, destino)) return t; // deduplicar arista exacta
    }

    origen.transiciones_.emplace_back(simbolo, destino);
    origen.transiciones_.back().propietario_ = this;
    return origen.transiciones_.back();
}

void Automata::agregarTransicion(const std::string& o, const std::string& s,
                                 const std::vector<std::string>& ds) {
    auto* origen = buscarEstado(o);
    if (!origen) throw std::invalid_argument("Origen inexistente");
    if (esEstadoError(origen)) throw std::invalid_argument("SR se administra automaticamente");

    std::vector<Estado*> destinos;
    std::set<Estado*> unicos;
    for (const auto& id : ds) {
        auto* d = buscarEstado(id);
        if (!d) throw std::invalid_argument("Destino inexistente: " + id);
        if (esEstadoError(d)) throw std::invalid_argument("SR se administra automaticamente");
        if (unicos.insert(d).second) destinos.push_back(d);
    }

    // Una entrada sin destinos equivale a no definir arista. Si el simbolo es real,
    // se declara igualmente y el completado agregara simbolo -> SR.
    if (destinos.empty()) {
        if (!s.empty()) alfabetoDeclarado_.insert(s);
        completarEstadoError();
        return;
    }

    for (auto* d : destinos) agregarAristaInterna(*origen, s, d);
}

void Automata::completarEstadoError() {
    bool hayEstadoUsuario = false;
    for (const auto& e : estados_) {
        if (!esEstadoError(e.get())) { hayEstadoUsuario = true; break; }
    }
    if (!hayEstadoUsuario) return;

    auto& srRef = asegurarEstadoError();
    auto* sr = &srRef;
    const auto alfabeto = getAlfabeto();

    // SR siempre es no inicial, no final y tiene exactamente un bucle por simbolo.
    sr->inicial_ = false;
    sr->final_ = false;
    sr->transiciones_.clear();
    for (const auto& simbolo : alfabeto) {
        sr->transiciones_.emplace_back(simbolo, sr);
        sr->transiciones_.back().propietario_ = this;
    }

    for (auto& e : estados_) {
        if (e.get() == sr) continue;

        // 1) conservar aristas validas y quitar duplicados exactos.
        std::vector<Transicion> limpias;
        std::set<std::pair<std::string, Estado*>> vistas;
        for (const auto& t : e->transiciones_) {
            auto* d = t.getDestino();
            if (!d || !contieneEstado(d))
                throw std::invalid_argument("Transicion con destino nulo o ajeno");

            // Un fallback viejo de un simbolo eliminado ya no pertenece al automata.
            if (d == sr && !t.esEpsilon() && !alfabeto.count(t.getSimbolo())) continue;

            if (vistas.insert({t.getSimbolo(), d}).second)
                limpias.emplace_back(t.getSimbolo(), d);
        }

        // 2) si hay una arista real para un simbolo, quitar el fallback -> SR.
        for (const auto& simbolo : alfabeto) {
            bool hayReal = false;
            for (const auto& t : limpias) {
                if (t.getSimbolo() == simbolo && t.getDestino() != sr) {
                    hayReal = true;
                    break;
                }
            }
            if (hayReal) {
                limpias.erase(std::remove_if(limpias.begin(), limpias.end(), [&](const Transicion& t) {
                    return t.getSimbolo() == simbolo && t.getDestino() == sr;
                }), limpias.end());
            }
        }

        // 3) todo simbolo ausente recibe exactamente un fallback -> SR.
        for (const auto& simbolo : alfabeto) {
            bool existe = false;
            for (const auto& t : limpias) {
                if (t.getSimbolo() == simbolo) { existe = true; break; }
            }
            if (!existe) limpias.emplace_back(simbolo, sr);
        }

        e->transiciones_ = std::move(limpias);
        for (auto& t : e->transiciones_) t.propietario_ = this;
    }
}

bool Automata::esDeterminista() const {
    const auto cantidadSimbolos = getAlfabeto().size();

    for (const auto& estado : estados_) {
        const auto cantidadTransiciones = estado->getTransiciones().size();

        // Tras completar con SR, cada estado tiene COMO MINIMO una salida por simbolo.
        // Si hay mas transiciones que simbolos, por el principio del palomar hay
        // repeticion de simbolo o una epsilon: el automata es no determinista.
        if (cantidadTransiciones > cantidadSimbolos) return false;

        // Defensa ante una estructura manipulada por fuera del API normalizado.
        if (cantidadTransiciones < cantidadSimbolos) return false;
    }
    return true;
}

TipoAutomata Automata::getTipo() const {
    return esDeterminista() ? TipoAutomata::AFD : TipoAutomata::AFND;
}

void Automata::setTipo(TipoAutomata t) {
    if (t != getTipo())
        throw std::invalid_argument("El tipo no coincide con las transiciones; usa ConversorAFND para convertir");
}

bool Automata::validarCadena(const std::string& s) const {
    for (const auto& token : getAlfabeto()) {
        if (token.size() != 1)
            throw std::invalid_argument("Para simbolos de varios bytes usa validarCadena(vector<string>)");
    }
    std::vector<std::string> tokens;
    for (char c : s) tokens.emplace_back(1, c);
    return acepta(tokens);
}

bool Automata::validarCadena(const std::vector<std::string>& s) const { return acepta(s); }

void Automata::limpiar() noexcept {
    estados_.clear();
    inicial_ = nullptr;
    alfabetoDeclarado_.clear();
}

void Automata::validar() const {
    if (!inicial_ || !contieneEstado(inicial_))
        throw std::invalid_argument("Falta un estado inicial valido");

    const auto* sr = buscarEstado(ID_ESTADO_ERROR);
    if (!sr) throw std::logic_error("Falta el estado interno SR; ejecuta completarEstadoError()");
    if (sr->esEstadoInicial() || sr->esEstadoFinal())
        throw std::logic_error("SR debe ser no inicial y no final");

    const auto alfabeto = getAlfabeto();

    for (const auto& e : estados_) {
        if (e->esEstadoInicial() != (e.get() == inicial_) || e->propietario_ != this)
            throw std::logic_error("Estado inicial o propietario inconsistente");

        std::set<std::pair<std::string, const Estado*>> vistas;
        for (const auto& t : e->transiciones_) {
            if (t.propietario_ != this)
                throw std::logic_error("Propietario de transicion inconsistente");
            if (!t.getDestino() || !contieneEstado(t.getDestino()))
                throw std::invalid_argument("Destino ajeno al automata");
            if (!vistas.insert({t.getSimbolo(), t.getDestino()}).second)
                throw std::logic_error("Transicion duplicada exacta");
        }

        for (const auto& simbolo : alfabeto) {
            bool existe = false;
            for (const auto& t : e->transiciones_) {
                if (t.getSimbolo() == simbolo) { existe = true; break; }
            }
            if (!existe)
                throw std::logic_error("Automata incompleto: falta transicion para simbolo " + simbolo);
        }
    }
}

Automata Automata::clonar() const {
    Automata c;
    c.alfabetoDeclarado_ = alfabetoDeclarado_;

    for (const auto& e : estados_) {
        if (esEstadoError(e.get())) continue;
        c.agregarEstado(e->getId(), e->esEstadoFinal());
    }
    if (inicial_) c.setEstadoInicial(inicial_->getId());

    const auto* sr = getEstadoError();
    for (const auto& e : estados_) {
        if (e.get() == sr) continue;
        for (const auto& t : e->transiciones_) {
            if (t.getDestino() == sr) continue; // fallback interno: se regenera
            c.agregarTransicion(e->getId(), t.getSimbolo(), {t.getDestino()->getId()});
        }
    }
    c.completarEstadoError();
    return c;
}

bool Automata::acepta(const std::vector<std::string>& palabra) const {
    validar();
    auto activos = detalle::cierre(*this, {inicial()->nombre()});
    for (const auto& s : palabra) {
        if (s.empty()) throw std::invalid_argument("Epsilon no es un simbolo de entrada");
        activos = detalle::mover(*this, activos, s);
    }
    return detalle::final(*this, activos);
}
