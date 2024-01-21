find_package(SEAL 4.1 REQUIRED)

add_library(pir pir.hpp pir.cpp pir_client.hpp pir_client.cpp pir_server.hpp pir_server.cpp)
add_library(imp_data imp_data.cpp imp_data.hpp)

add_executable(main main.cpp)

target_link_libraries(pir SEAL::seal)
target_link_libraries(main imp_data pir)
