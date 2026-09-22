#include "Transicion.h"
#include "Automata.h"
#include <algorithm>
#include <stdexcept>
#include <utility>
Transicion::Transicion(std::string s) : simbolo_(std::move(s)) {}
Transicion::Transicion(std::string s, Estado* d) : Transicion(std::move(s)) { setDestino(d); }
Transicion::Transicion(std::string s, const std::vector<Estado*>& d) : Transicion(std::move(s)) { setDestinos(d); }
Transicion::Transicion(const Transicion& t) : simbolo_(t.simbolo_), destinos_(t.destinos_) {}
Transicion& Transicion::operator=(const Transicion& t) {
    if (this != &t) {
        comprobar(t.destinos_);
        auto s = t.simbolo_; auto d = t.destinos_;
        simbolo_.swap(s); destinos_.swap(d);
    }
    return *this;
}
void Transicion::comprobar(const std::vector<Estado*>& d) const {
    for (auto* e : d) {
        if (!e) throw std::invalid_argument("Un destino no puede ser nullptr");
        if (propietario_ && !propietario_->contieneEstado(e))
            throw std::invalid_argument("Destino ajeno al automata");
    }
}
const std::string& Transicion::getSimbolo() const noexcept { return simbolo_; }
void Transicion::setSimbolo(const std::string& s) { simbolo_ = s; }
Estado* Transicion::getDestino() const {
    if (destinos_.size() > 1) throw std::logic_error("Hay varios destinos: usa getDestinos()");
    return destinos_.empty() ? nullptr : destinos_.front();
}
void Transicion::setDestino(Estado* d) { setDestinos(d ? std::vector<Estado*>{d} : std::vector<Estado*>{}); }
const std::vector<Estado*>& Transicion::getDestinos() const noexcept { return destinos_; }
void Transicion::setDestinos(const std::vector<Estado*>& d) {
    comprobar(d);
    std::vector<Estado*> unicos;
    for (auto* e : d) if (std::find(unicos.begin(), unicos.end(), e) == unicos.end()) unicos.push_back(e);
    destinos_.swap(unicos);
}
void Transicion::agregarDestino(Estado* d) {
    comprobar({d});
    if (!contieneDestino(d)) destinos_.push_back(d);
}
bool Transicion::eliminarDestino(Estado* d) {
    auto it = std::find(destinos_.begin(), destinos_.end(), d);
    if (it == destinos_.end()) return false;
    destinos_.erase(it); return true;
}
bool Transicion::contieneDestino(const Estado* d) const noexcept {
    return std::find(destinos_.begin(), destinos_.end(), d) != destinos_.end();
}
void Transicion::limpiarDestinos() noexcept { destinos_.clear(); }
std::size_t Transicion::getCantidadDestinos() const noexcept { return destinos_.size(); }
bool Transicion::esEpsilon() const noexcept { return simbolo_.empty(); }
