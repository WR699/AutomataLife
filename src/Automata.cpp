#include "Automata.h"
#include "AlgoritmosInternos.h"
#include <algorithm>
#include <stdexcept>
#include <utility>
void Automata::vincular() noexcept {
    for (auto& e : estados_) {
        e->propietario_ = this;
        for (auto& t : e->transiciones_) t.propietario_ = this;
    }
}
Automata::Automata(Automata&& o) noexcept
    : estados_(std::move(o.estados_)), inicial_(o.inicial_), alfabetoDeclarado_(std::move(o.alfabetoDeclarado_)) {
    o.inicial_ = nullptr; vincular();
}
Automata& Automata::operator=(Automata&& o) noexcept {
    if (this != &o) {
        estados_ = std::move(o.estados_); inicial_ = o.inicial_;
        alfabetoDeclarado_ = std::move(o.alfabetoDeclarado_);
        o.inicial_ = nullptr; vincular();
    }
    return *this;
}
Estado& Automata::agregarEstado(const std::string& n, bool f) {
    return agregarEstado(std::make_unique<Estado>(n, f));
}
Estado& Automata::agregarEstado(std::unique_ptr<Estado> e) {
    if (!e || e->propietario_) throw std::invalid_argument("Estado nulo o ya perteneciente a un automata");
    if (buscarEstado(e->getId())) throw std::invalid_argument("Id duplicado: " + e->getId());
    for (const auto& t : e->transiciones_) for (auto* d : t.destinos_)
        if (!d || (d != e.get() && !contieneEstado(d))) throw std::invalid_argument("Destino ajeno al automata");
    auto* ptr = e.get();
    estados_.push_back(std::move(e));
    ptr->propietario_ = this;
    for (auto& t : ptr->transiciones_) t.propietario_ = this;
    if (ptr->inicial_) setEstadoInicial(ptr);
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
    std::set<Estado*> s; for (const auto& e : estados_) s.insert(e.get()); return s;
}
std::set<const Estado*> Automata::getEstados() const {
    std::set<const Estado*> s; for (const auto& e : estados_) s.insert(e.get()); return s;
}
void Automata::setEstados(std::vector<std::unique_ptr<Estado>> es) {
    Automata nuevo;
    std::set<Estado*> direcciones; std::set<std::string> ids;
    for (const auto& e : es) {
        if (!e || e->propietario_ || !ids.insert(e->getId()).second)
            throw std::invalid_argument("Estados nulos, ajenos o duplicados");
        direcciones.insert(e.get());
        if (e->inicial_) {
            if (nuevo.inicial_) throw std::invalid_argument("Mas de un estado inicial");
            nuevo.inicial_ = e.get();
        }
    }
    for (const auto& e : es) for (const auto& t : e->transiciones_)
        for (auto* d : t.destinos_) if (!direcciones.count(d)) throw std::invalid_argument("Destino fuera del nuevo conjunto");
    nuevo.alfabetoDeclarado_ = alfabetoDeclarado_;
    nuevo.estados_ = std::move(es); nuevo.vincular();
    *this = std::move(nuevo);
}
bool Automata::contieneEstado(const Estado* e) const noexcept {
    for (const auto& p : estados_) if (p.get() == e) return true;
    return false;
}
bool Automata::eliminarEstado(const std::string& id) {
    auto* ptr = buscarEstado(id); if (!ptr) return false;
    for (auto& e : estados_) for (auto& t : e->transiciones_) t.eliminarDestino(ptr);
    if (inicial_ == ptr) inicial_ = nullptr;
    auto it = std::find_if(estados_.begin(), estados_.end(), [ptr](const auto& e) { return e.get() == ptr; });
    estados_.erase(it); return true;
}
std::size_t Automata::getCantidadEstados() const noexcept { return estados_.size(); }
Estado* Automata::getEstadoInicial() noexcept { return inicial_; }
const Estado* Automata::getEstadoInicial() const noexcept { return inicial_; }
void Automata::setEstadoInicial(Estado* e) {
    if (e && !contieneEstado(e)) throw std::invalid_argument("Inicial ajeno al automata");
    for (auto& p : estados_) p->inicial_ = (p.get() == e);
    inicial_ = e;
}
void Automata::setEstadoInicial(const std::string& id) {
    auto* e = buscarEstado(id); if (!e) throw std::invalid_argument("Inicial inexistente");
    setEstadoInicial(e);
}
void Automata::establecerFinal(const std::string& id, bool f) {
    auto* e = buscarEstado(id); if (!e) throw std::invalid_argument("Estado inexistente");
    e->setEstadoFinal(f);
}
void Automata::agregarTransicion(const std::string& o, const std::string& s, const std::vector<std::string>& ds) {
    auto* e = buscarEstado(o); if (!e) throw std::invalid_argument("Origen inexistente");
    std::vector<Estado*> destinos;
    for (const auto& id : ds) {
        auto* d = buscarEstado(id); if (!d) throw std::invalid_argument("Destino inexistente: " + id);
        destinos.push_back(d);
    }
    e->agregarTransicion(Transicion(s, destinos));
}
std::set<std::string> Automata::getAlfabeto() const {
    auto s = alfabetoDeclarado_;
    for (const auto& e : estados_) for (const auto& t : e->transiciones_)
        if (!t.esEpsilon()) s.insert(t.getSimbolo());
    return s;
}
void Automata::setAlfabeto(const std::set<std::string>& s) {
    if (s.count("")) throw std::invalid_argument("Epsilon no pertenece al alfabeto");
    for (const auto& e : estados_) for (const auto& t : e->transiciones_)
        if (!t.esEpsilon() && !s.count(t.getSimbolo())) throw std::invalid_argument("Falta un simbolo usado en transiciones");
    alfabetoDeclarado_ = s;
}
void Automata::agregarSimbolo(const std::string& s) {
    if (s.empty()) throw std::invalid_argument("Epsilon no pertenece al alfabeto");
    alfabetoDeclarado_.insert(s);
}
bool Automata::eliminarSimbolo(const std::string& s) {
    for (const auto& e : estados_) for (const auto& t : e->transiciones_)
        if (!t.esEpsilon() && t.getSimbolo() == s) throw std::invalid_argument("Simbolo en uso");
    return alfabetoDeclarado_.erase(s) != 0;
}
bool Automata::esDeterminista() const {
    for (const auto& e : estados_) {
        std::map<std::string, std::set<const Estado*>> destinos;
        for (const auto& t : e->transiciones_) {
            if (t.esEpsilon()) return false;
            for (const auto* d : t.destinos()) destinos[t.simbolo()].insert(d);
        }
        for (const auto& p : destinos) if (p.second.size() > 1) return false;
    }
    return true;
}
TipoAutomata Automata::getTipo() const { return esDeterminista() ? TipoAutomata::AFD : TipoAutomata::AFND; }
void Automata::setTipo(TipoAutomata t) {
    if (t != getTipo()) throw std::invalid_argument("El tipo no coincide con las transiciones; usa ConversorAFND para convertir");
}
bool Automata::validarCadena(const std::string& s) const {
    for (const auto& token : getAlfabeto()) if (token.size() != 1)
        throw std::invalid_argument("Para simbolos de varios bytes usa validarCadena(vector<string>)");
    std::vector<std::string> tokens;
    for (char c : s) tokens.emplace_back(1, c);
    return acepta(tokens);
}
bool Automata::validarCadena(const std::vector<std::string>& s) const { return acepta(s); }
void Automata::limpiar() noexcept { estados_.clear(); inicial_ = nullptr; alfabetoDeclarado_.clear(); }
void Automata::validar() const {
    if (!inicial_ || !contieneEstado(inicial_)) throw std::invalid_argument("Falta un estado inicial valido");
    for (const auto& e : estados_) {
        if (e->esEstadoInicial() != (e.get() == inicial_) || e->propietario_ != this)
            throw std::logic_error("Estado inicial o propietario inconsistente");
        for (const auto& t : e->transiciones_) for (auto* d : t.destinos_)
            if (!contieneEstado(d)) throw std::invalid_argument("Destino ajeno al automata");
    }
}
Automata Automata::clonar() const {
    Automata c; c.alfabetoDeclarado_ = alfabetoDeclarado_;
    for (const auto& e : estados_) c.agregarEstado(e->getId(), e->esEstadoFinal());
    if (inicial_) c.setEstadoInicial(inicial_->getId());
    for (const auto& e : estados_) for (const auto& t : e->transiciones_) {
        std::vector<std::string> ds;
        for (const auto* d : t.destinos_) ds.push_back(d->getId());
        c.agregarTransicion(e->getId(), t.getSimbolo(), ds);
    }
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
