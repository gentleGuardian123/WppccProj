include_directories (${WppccProj_SOURCE_DIR}/src)

add_executable(batch_query_test batch_query_test.cpp)
target_link_libraries(batch_query_test pir)
add_test(NAME batch_query_test COMMAND batch_query_test)

add_executable(imp_data_test imp_data_test.cpp)
target_link_libraries(imp_data_test imp_data)
add_test(NAME imp_data_test COMMAND imp_data_test)

add_executable(invert_mod_test invert_mod_test.cpp)
target_link_libraries(invert_mod_test pir)
add_test(NAME invert_mod_test COMMAND invert_mod_test)

add_executable(gen_rand_pt_test gen_rand_pt_test.cpp)
target_link_libraries(gen_rand_pt_test pir)
add_test(NAME gen_rand_pt_test COMMAND gen_rand_pt_test)

add_executable(multi_party_comput_test multi_party_comput_test.cpp)
target_link_libraries(multi_party_comput_test pir)
add_test(NAME multi_party_comput_test COMMAND multi_party_comput_test)

add_executable(batch_mpc_query_test batch_mpc_query_test.cpp)
target_link_libraries(batch_mpc_query_test pir)
add_test(NAME batch_mpc_query_test COMMAND batch_mpc_query_test)