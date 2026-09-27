cmake_minimum_required(VERSION 3.16)
project(AutomatasCpp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Se agregan los archivos del parser XML, JSON y TinyXML-2 a AutomatasCore
add_library(AutomatasCore
    src/Automata.cpp 
    src/Estado.cpp 
    src/Transicion.cpp 
    src/ConstruirAutomata.cpp 
    src/ConversorAFND.cpp
    src/AdministradorDeAutomatas.cpp 
    src/ParserXmlAutomata.cpp
    src/ParserJsonAutomata.cpp
    src/tinyxml2.cpp
    src/MinimizadorAFD.cpp 
    src/TesterEquivalencia.cpp
    src/InterfazUsuario.cpp 
    src/Main.cpp
)

target_include_directories(AutomatasCore PUBLIC include)

if(MSVC)
    target_compile_options(AutomatasCore PRIVATE /W4 /utf-8)
else()
    target_compile_options(AutomatasCore PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(Automatas src/PuntoEntrada.cpp)
target_link_libraries(Automatas PRIVATE AutomatasCore)

include(CTest)
if(BUILD_TESTING)
    add_executable(Pruebas tests/pruebas.cpp tests/pruebas_api.cpp)
    target_link_libraries(Pruebas PRIVATE AutomatasCore)
    add_test(NAME pruebas_automatas COMMAND Pruebas)

    # Stress grande: se compila siempre, pero el CTest normal ejecuta solo un smoke.
    # El barrido completo hace 9.999.900.000 comparaciones y se lanza aparte.
    find_package(Threads REQUIRED)
    add_executable(PruebasStress200k tests/pruebas_stress_200k.cpp)
    target_link_libraries(PruebasStress200k PRIVATE AutomatasCore Threads::Threads)
    add_test(NAME pruebas_stress_smoke COMMAND PruebasStress200k --smoke)
endif()
