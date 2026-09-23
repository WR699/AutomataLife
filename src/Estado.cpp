#include "Estado.h"
#include "Automata.h"
#include <stdexcept>
#include <utility>

Estado::Estado(std::string id, bool f) : Estado(std::move(id), false, f) {}

Estado::Estado(std::string id, bool i, bool f)
    : nombre_(std::move(id)), inicial_(i), final_(f) {
    if (nombre_.empty()) throw std::invalid_argument("Id vacio");
}

const std::string& Estado::getId() const noexcept { return nombre_; }

void Estado::setId(const std::string& id) {
    if (id.empty()) throw std::invalid_argument("Id vacio");
    if (propietario_) {
        if (nombre_ == Automata::ID_ESTADO_ERROR)
            throw std::logic_error("El estado SR es interno y no se puede renombrar");
        if (id == Automata::ID_ESTADO_ERROR)
            throw std::invalid_argument("El id SR esta reservado para State Error");
        auto* otro = propietario_->buscarEstado(id);
        if (otro && otro != this) throw std::invalid_argument("Id duplicado: " + id);
    }
    nombre_ = id;
}

bool Estado::esEstadoInicial() const noexcept { return inicial_; }

void Estado::setEstadoInicial(bool i) {
    if (!propietario_) {
        inicial_ = i;
        return;
    }
    if (i) propietario_->setEstadoInicial(this);
    else if (inicial_) propietario_->setEstadoInicial(nullptr);
}

bool Estado::esEstadoFinal() const noexcept { return final_; }

void Estado::setEstadoFinal(bool f) {
    if (propietario_ && nombre_ == Automata::ID_ESTADO_ERROR && f)
        throw std::logic_error("SR no puede ser estado final");
    final_ = f;
}

const std::vector<Transicion>& Estado::getTransiciones() const noexcept { return transiciones_; }

void Estado::setTransiciones(const std::vector<Transicion>& ts) {
    std::vector<Transicion> copia;
    copia.reserve(ts.size());

    for (const auto& t : ts) {
        if (!t.getDestino())
            throw std::invalid_argument("Cada transicion debe tener exactamente un destino");
        if (propietario_ && !propietario_->contieneEstado(t.getDestino()))
            throw std::invalid_argument("Destino ajeno al automata");

        bool duplicada = false;
        for (const auto& existente : copia) {
            if (existente.getSimbolo() == t.getSimbolo() && existente.getDestino() == t.getDestino()) {
                duplicada = true;
                break;
            }
        }
        if (!duplicada) copia.emplace_back(t.getSimbolo(), t.getDestino());
    }

    if (propietario_ && nombre_ == Automata::ID_ESTADO_ERROR)
        throw std::logic_error("Las transiciones de SR son internas");

    transiciones_ = std::move(copia);
    for (auto& t : transiciones_) t.propietario_ = propietario_;

    if (propietario_) {
        for (const auto& t : transiciones_)
            if (!t.esEpsilon()) propietario_->alfabetoDeclarado_.insert(t.getSimbolo());
        propietario_->completarEstadoError();
    }
}

Transicion& Estado::agregarTransicion(const Transicion& t) {
    if (!t.getDestino())
        throw std::invalid_argument("Cada transicion debe tener exactamente un destino");

    if (propietario_)
        return propietario_->agregarAristaInterna(*this, t.getSimbolo(), t.getDestino());

    for (auto& existente : transiciones_) {
        if (existente.getSimbolo() == t.getSimbolo() && existente.getDestino() == t.getDestino())
            return existente;
    }

    transiciones_.emplace_back(t.getSimbolo(), t.getDestino());
    return transiciones_.back();
}

Transicion& Estado::getTransicion(std::size_t i) { return transiciones_.at(i); }
const Transicion& Estado::getTransicion(std::size_t i) const { return transiciones_.at(i); }

void Estado::eliminarTransicion(std::size_t i) {
    if (propietario_ && nombre_ == Automata::ID_ESTADO_ERROR)
        throw std::logic_error("Las transiciones de SR son internas");
    if (i >= transiciones_.size()) throw std::out_of_range("Indice de transicion invalido");
    transiciones_.erase(transiciones_.begin() + static_cast<std::ptrdiff_t>(i));
    if (propietario_) propietario_->completarEstadoError();
}

void Estado::limpiarTransiciones() {
    if (propietario_ && nombre_ == Automata::ID_ESTADO_ERROR)
        throw std::logic_error("Las transiciones de SR son internas");
    transiciones_.clear();
    if (propietario_) propietario_->completarEstadoError();
}

std::size_t Estado::getCantidadTransiciones() const noexcept { return transiciones_.size(); }
