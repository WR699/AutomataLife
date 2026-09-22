#include "Estado.h"
#include "Automata.h"
#include <stdexcept>
#include <utility>
Estado::Estado(std::string id, bool f) : Estado(std::move(id), false, f) {}
Estado::Estado(std::string id, bool i, bool f) : nombre_(std::move(id)), inicial_(i), final_(f) {
    if (nombre_.empty()) throw std::invalid_argument("Id vacio");
}
const std::string& Estado::getId() const noexcept { return nombre_; }
void Estado::setId(const std::string& id) {
    if (id.empty()) throw std::invalid_argument("Id vacio");
    if (propietario_) {
        auto* otro = propietario_->buscarEstado(id);
        if (otro && otro != this) throw std::invalid_argument("Id duplicado: " + id);
    }
    nombre_ = id;
}
bool Estado::esEstadoInicial() const noexcept { return inicial_; }
void Estado::setEstadoInicial(bool i) {
    if (!propietario_) { inicial_ = i; return; }
    if (i) propietario_->setEstadoInicial(this);
    else if (inicial_) propietario_->setEstadoInicial(nullptr);
}
bool Estado::esEstadoFinal() const noexcept { return final_; }
void Estado::setEstadoFinal(bool f) noexcept { final_ = f; }
const std::vector<Transicion>& Estado::getTransiciones() const noexcept { return transiciones_; }
void Estado::setTransiciones(const std::vector<Transicion>& ts) {
    auto copia = ts;
    for (auto& t : copia) { t.propietario_ = propietario_; t.comprobar(t.destinos_); }
    transiciones_.swap(copia);
}
Transicion& Estado::agregarTransicion(const Transicion& t) {
    Transicion copia(t);
    copia.propietario_ = propietario_; copia.comprobar(copia.destinos_);
    transiciones_.push_back(copia);
    // La copia de Transicion se desacopla del propietario; revincular tras realocar.
    for (auto& v : transiciones_) v.propietario_ = propietario_;
    return transiciones_.back();
}
Transicion& Estado::getTransicion(std::size_t i) { return transiciones_.at(i); }
const Transicion& Estado::getTransicion(std::size_t i) const { return transiciones_.at(i); }
void Estado::eliminarTransicion(std::size_t i) {
    if (i >= transiciones_.size()) throw std::out_of_range("Indice de transicion invalido");
    transiciones_.erase(transiciones_.begin() + static_cast<std::ptrdiff_t>(i));
}
void Estado::limpiarTransiciones() noexcept { transiciones_.clear(); }
std::size_t Estado::getCantidadTransiciones() const noexcept { return transiciones_.size(); }
