#include "Main.h"
int main(int argc, char* argv[]) {
    std::vector<std::string> argumentos;
    for (int i = 1; i < argc; ++i) argumentos.emplace_back(argv[i]);
    return Main::main(argumentos);
}
