#include "Main.h"
#include "InterfazUsuario.h"
#include <iostream>
int Main::ejecutar() {
    try { InterfazUsuario interfaz; interfaz.iniciar(); return 0; }
    catch (const std::exception& e) { std::cerr << "Error fatal: " << e.what() << '\n'; return 1; }
}

int Main::main(const std::vector<std::string>& args) { (void)args; return Main{}.ejecutar(); }
