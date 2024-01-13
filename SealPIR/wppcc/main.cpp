#include "pir.hpp"
#include "pir_client.hpp"
#include "pir_server.hpp"
#include "imp_data.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <random>
#include <seal/seal.h>

using namespace std::chrono;
using namespace std;
using namespace seal;

int main(int argc, char *argv[]) {
     uint8_t dim_of_items_number = 32;
     uint64_t number_of_items = 1 << dim_of_items_number;
     uint64_t size_per_item = 1024; // in bytes
     uint32_t N = 4096;
     uint32_t logt = 20;
     uint32_t d = 2;
     bool use_symmetric = true; 
     bool use_batching = true;  
     bool use_recursive_mod_switching = true;
     EncryptionParameters enc_params(scheme_type::bfv);
     PirParams pir_params;

     cout << "Main: Generating SEAL parameters" << endl;
     gen_encryption_params(N, logt, enc_params);

     cout << "Main: Verifying SEAL parameters" << endl;
     verify_encryption_params(enc_params);
     cout << "Main: SEAL parameters are good" << endl;

     cout << "Main: Generating PIR parameters" << endl;
     gen_pir_params(number_of_items, size_per_item, d, enc_params, pir_params,
                    use_symmetric, use_batching, use_recursive_mod_switching);

     print_seal_params(enc_params);
     print_pir_params(pir_params);

     PIRClient client(enc_params, pir_params);
     cout << "Main: Generating galois keys for client" << endl;

     GaloisKeys galois_keys = client.generate_galois_keys();

     cout << "Main: Initializing server" << endl;
     PIRServer server(enc_params, pir_params);

     server.set_galois_key(0, galois_keys);

     cout << "Main: Creating the database with random data (this may take some "
             "time) ..."
          << endl;


     auto db_a(make_unique<uint8_t[]>(number_of_items * size_per_item));
     auto db_a_copy(make_unique<uint8_t[]>(number_of_items * size_per_item));
     auto db_b(make_unique<uint8_t[]>(number_of_items * size_per_item));
     auto db_b_copy(make_unique<uint8_t[]>(number_of_items * size_per_item));
     auto db_c(make_unique<uint8_t[]>(number_of_items * size_per_item));
     auto db_c_copy(make_unique<uint8_t[]>(number_of_items * size_per_item));

     random_device rd;
     // for (uint64_t i = 0; i < number_of_items; i++) {
     //   for (uint64_t j = 0; j < size_per_item; j++) {
     //     uint8_t val = rd() % 256;
     //     db_a.get()[(i * size_per_item) + j] = val;
     //     db_a_copy.get()[(i * size_per_item) + j] = val;
     //   }
     // }

     import_and_parse_data(db_a, "../../data/dataset_A.csv", dim_of_items_number);
     import_and_parse_data(db_a_copy, "../../data/dataset_A.csv", dim_of_items_number);
     import_and_parse_data(db_b, "../../data/dataset_B.csv", dim_of_items_number);
     import_and_parse_data(db_b_copy, "../../data/dataset_B.csv", dim_of_items_number);
     import_and_parse_data(db_c, "../../data/dataset_C.csv", dim_of_items_number);
     import_and_parse_data(db_c_copy, "../../data/dataset_C.csv", dim_of_items_number);

     auto time_pre_s = high_resolution_clock::now();
     server.set_database(move(db_a), number_of_items, size_per_item);
     server.preprocess_database();
     auto time_pre_e = high_resolution_clock::now();
     auto time_pre_us =
         duration_cast<microseconds>(time_pre_e - time_pre_s).count();
     cout << "Main: database pre processed " << endl;


     uint64_t ele_index =
         rd() % number_of_items; // element in db_a at random position
     uint64_t index = client.get_fv_index(ele_index);   // index of FV plaintext
     uint64_t offset = client.get_fv_offset(ele_index); // offset in FV plaintext
     cout << "Main: element index = " << ele_index << " from [0, "
          << number_of_items - 1 << "]" << endl;
     cout << "Main: FV index = " << index << ", FV offset = " << offset << endl;


     auto time_query_s = high_resolution_clock::now();
     PirQuery query = client.generate_query(index);
     auto time_query_e = high_resolution_clock::now();
     auto time_query_us =
         duration_cast<microseconds>(time_query_e - time_query_s).count();
     cout << "Main: query generated" << endl;


     stringstream client_stream;
     stringstream server_stream;
     auto time_s_query_s = high_resolution_clock::now();
     int query_size = client.generate_serialized_query(index, client_stream);
     auto time_s_query_e = high_resolution_clock::now();
     auto time_s_query_us =
         duration_cast<microseconds>(time_s_query_e - time_s_query_s).count();
     cout << "Main: serialized query generated" << endl;


     auto time_deserial_s = high_resolution_clock::now();
     PirQuery query2 = server.deserialize_query(client_stream);
     auto time_deserial_e = high_resolution_clock::now();
     auto time_deserial_us =
         duration_cast<microseconds>(time_deserial_e - time_deserial_s).count();
     cout << "Main: query deserialized" << endl;

     
     auto time_server_s = high_resolution_clock::now();
     // Answer PIR query from client 0. If there are multiple clients,
     // enter the id of the client (to use the associated galois key).
     PirReply reply = server.generate_reply(query, 0);
     auto time_server_e = high_resolution_clock::now();
     auto time_server_us =
         duration_cast<microseconds>(time_server_e - time_server_s).count();
     cout << "Main: reply generated" << endl;
     
     int reply_size = server.serialize_reply(reply, server_stream);
     
     auto time_decode_s = chrono::high_resolution_clock::now();
     vector<uint8_t> elems = client.decode_reply(reply, offset);
     auto time_decode_e = chrono::high_resolution_clock::now();
     auto time_decode_us =
         duration_cast<microseconds>(time_decode_e - time_decode_s).count();
     cout << "Main: reply decoded" << endl;

     assert(elems.size() == size_per_item);
     bool failed = false;
     
     for (uint32_t i = 0; i < size_per_item; i++) {
       if (elems[i] != db_a_copy.get()[(ele_index * size_per_item) + i]) {
         cout << "Main: elems " << (int)elems[i] << ", db_a "
              << (int)db_a_copy.get()[(ele_index * size_per_item) + i] << endl;
         cout << "Main: PIR result wrong at " << i << endl;
         failed = true;
       }
     }
     if (failed) {
       return -1;
     }

     cout << "Main: PIR result correct!" << endl;
     cout << "Main: PIRServer pre-processing time: " << time_pre_us / 1000 << " ms"
          << endl;
     cout << "Main: PIRClient query generation time: " << time_query_us / 1000
          << " ms" << endl;
     cout << "Main: PIRClient serialized query generation time: "
          << time_s_query_us / 1000 << " ms" << endl;
     cout << "Main: PIRServer query deserialization time: " << time_deserial_us
          << " us" << endl;
     cout << "Main: PIRServer reply generation time: " << time_server_us / 1000
          << " ms" << endl;
     cout << "Main: PIRClient answer decode time: " << time_decode_us / 1000
          << " ms" << endl;
     cout << "Main: Query size: " << query_size << " bytes" << endl;
     cout << "Main: Reply num ciphertexts: " << reply.size() << endl;
     cout << "Main: Reply size: " << reply_size << " bytes" << endl;

     return 0;
}
