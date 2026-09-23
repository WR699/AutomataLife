#include "Transicion.h"
#include "Automata.h"
#include "Estado.h"
#include <stdexcept>
#include <utility>

Transicion::Transicion(std::string s) : simbolo_(std::move(s)) {}

Transicion::Transicion(std::string s, Estado* d)
    : simbolo_(std::move(s)), destino_(d) {}

Transicion::Transicion(std::string s, const std::vector<Estado*>& d)
    : simbolo_(std::move(s)) {
    if (d.size() > 1)
        throw std::invalid_argument("Una Transicion individual solo puede tener un destino");
    destino_ = d.empty() ? nullptr : d.front();
}

Transicion::Transicion(const Transicion& t)
    : simbolo_(t.simbolo_), destino_(t.destino_) {}

Transicion::Transicion(Transicion&& t) noexcept
    : simbolo_(std::move(t.simbolo_)), destino_(t.destino_), propietario_(t.propietario_) {
    t.destino_ = nullptr;
    t.propietario_ = nullptr;
}

Transicion& Transicion::operator=(const Transicion& t) {
    if (this != &t) {
        if (t.destino_) comprobar(t.destino_);
        // Conserva el propietario del receptor. Esto permite que std::vector
        // compacte sus elementos internamente al borrar/reordenar transiciones.
        simbolo_ = t.simbolo_;
        destino_ = t.destino_;
    }
    return *this;
}

Transicion& Transicion::operator=(Transicion&& t) noexcept {
    if (this != &t) {
        simbolo_ = std::move(t.simbolo_);
        destino_ = t.destino_;
        propietario_ = t.propietario_;
        t.destino_ = nullptr;
        t.propietario_ = nullptr;
    }
    return *this;
}

void Transicion::comprobar(Estado* d) const {
    if (!d) throw std::invalid_argument("Una transicion debe tener exactamente un destino");
    if (propietario_ && !propietario_->contieneEstado(d))
        throw std::invalid_argument("Destino ajeno al automata");
}

const std::string& Transicion::getSimbolo() const noexcept { return simbolo_; }

void Transicion::setSimbolo(const std::string& s) {
    if (propietario_)
        throw std::logic_error("No cambies una transicion insertada; elimina y agrega la nueva transicion");
    simbolo_ = s;
}

Estado* Transicion::getDestino() const noexcept { return destino_; }

void Transicion::setDestino(Estado* d) {
    if (propietario_)
        throw std::logic_error("No cambies una transicion insertada; elimina y agrega la nueva transicion");
    if (d) comprobar(d);
    destino_ = d;
}

std::vector<Estado*> Transicion::getDestinos() const {
    return destino_ ? std::vector<Estado*>{destino_} : std::vector<Estado*>{};
}

void Transicion::setDestinos(const std::vector<Estado*>& d) {
    if (d.size() > 1)
        throw std::invalid_argument("Una Transicion individual solo puede tener un destino");
    setDestino(d.empty() ? nullptr : d.front());
}

void Transicion::agregarDestino(Estado* d) {
    if (!d) throw std::invalid_argument("Un destino no puede ser nullptr");
    if (!destino_) {
        setDestino(d);
        return;
    }
    if (destino_ == d) return;
    throw std::logic_error(
        "Una Transicion individual solo admite un destino; agrega otra Transicion para el segundo destino");
}

bool Transicion::eliminarDestino(Estado* d) {
    if (destino_ != d) return false;
    if (propietario_)
        throw std::logic_error("Elimina la transicion desde Estado/Automata para mantener el estado SR");
    destino_ = nullptr;
    return true;
}

bool Transicion::contieneDestino(const Estado* d) const noexcept { return destino_ == d; }

void Transicion::limpiarDestinos() {
    if (propietario_)
        throw std::logic_error("Elimina la transicion desde Estado/Automata para mantener el estado SR");
    destino_ = nullptr;
}

std::size_t Transicion::getCantidadDestinos() const noexcept { return destino_ ? 1u : 0u; }

bool Transicion::esEpsilon() const noexcept { return simbolo_.empty(); }
