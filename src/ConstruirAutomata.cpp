#include "ConstruirAutomata.h"
Estado& ConstruirAutomata::agregarEstado(Automata& a, const std::string& n, bool f) const {
    return a.agregarEstado(n, f);
}
void ConstruirAutomata::establecerInicial(Automata& a, const std::string& n) const { a.establecerInicial(n); }
void ConstruirAutomata::establecerFinal(Automata& a, const std::string& n, bool f) const { a.establecerFinal(n, f); }
void ConstruirAutomata::agregarTransicion(Automata& a, const std::string& o,
    const std::string& s, const std::vector<std::string>& d) const { a.agregarTransicion(o, s, d); }
Automata ConstruirAutomata::ejemplo() const {
    // Palabras sobre {a,b} terminadas en a; q0 --a--> {q0,q1}.
    Automata a;
    agregarEstado(a, "q0"); agregarEstado(a, "q1", true);
    establecerInicial(a, "q0");
    agregarTransicion(a, "q0", "a", {"q0", "q1"});
    agregarTransicion(a, "q0", "b", {"q0"});
    return a;
}
