find_package(SEAL 4.1 REQUIRED)

add_library(wppcc pir.hpp pir.cpp pir_client.hpp pir_client.cpp pir_server.hpp pir_server.cpp)

target_link_libraries(wppcc SEAL::seal)

add_executable(wppcc_main main.cpp)
target_link_libraries(wppcc_main wppcc)
