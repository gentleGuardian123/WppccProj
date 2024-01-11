find_package(SEAL 4.1 REQUIRED)

add_library(wppcc pir.hpp pir.cpp pir_client.hpp pir_client.cpp pir_server.hpp pir_server.cpp)
add_library(imp_data imp_data.cpp imp_data.hpp)

add_executable(wppcc_main main.cpp)
add_executable(imp_data_test imp_data_test.cpp)
add_executable(invert_mod_test invert_mod_test.cpp)

target_link_libraries(imp_data_test imp_data)
target_link_libraries(wppcc SEAL::seal)
target_link_libraries(wppcc_main imp_data wppcc)
target_link_libraries(invert_mod_test imp_data wppcc)
