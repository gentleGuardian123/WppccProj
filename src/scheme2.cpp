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

int main() {
    uint8_t dim_of_items_number = 12;
    uint64_t number_of_items = 1UL << dim_of_items_number;
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
    PIRServer server_a(enc_params, pir_params);
    PIRServer server_b(enc_params, pir_params);
    PIRServer server_c(enc_params, pir_params);

    server_a.set_galois_key(0, galois_keys);
    server_b.set_galois_key(0, galois_keys);
    server_c.set_galois_key(0, galois_keys);

    cout << "Main: Creating the database with random data (this may take some time) ..."
         << endl;


    auto db_a(make_unique<uint8_t[]>(number_of_items * size_per_item));
    auto db_a_copy(make_unique<uint8_t[]>(number_of_items * size_per_item));
    auto db_b(make_unique<uint8_t[]>(number_of_items * size_per_item));
    auto db_b_copy(make_unique<uint8_t[]>(number_of_items * size_per_item));
    auto db_c(make_unique<uint8_t[]>(number_of_items * size_per_item));
    auto db_c_copy(make_unique<uint8_t[]>(number_of_items * size_per_item));

    random_device rd;
    for (uint64_t i = 0; i < number_of_items; i++) {
        for (uint64_t j = 0; j < size_per_item; j++) {
            uint8_t val = rd() % 256;
            db_a.get()[(i * size_per_item) + j] = val;
            db_a_copy.get()[(i * size_per_item) + j] = val;
            val = rd() % 256;
            db_b.get()[(i * size_per_item) + j] = val;
            db_b_copy.get()[(i * size_per_item) + j] = val;
            val = rd() % 256;
            db_c.get()[(i * size_per_item) + j] = val;
            db_c_copy.get()[(i * size_per_item) + j] = val;
        }
    }
    
    server_a.set_database(move(db_a), number_of_items, size_per_item);
    server_a.preprocess_database();
    server_b.set_database(move(db_b), number_of_items, size_per_item);
    server_b.preprocess_database();
    server_c.set_database(move(db_c), number_of_items, size_per_item);
    server_c.preprocess_database();
    cout << "Main: database pre processed " << endl;

    uint64_t ele_index = rd() % number_of_items; // element in db_a at random position
    uint64_t index = client.get_fv_index(ele_index);   // index of FV plaintext
    uint64_t offset = client.get_fv_offset(ele_index); // offset in FV plaintext
    cout << "Main: element index = " << ele_index << " from [0, "
         << number_of_items - 1 << "]" << endl;
    cout << "Main: FV index = " << index << ", FV offset = " << offset << endl;

    PirQuery query = client.generate_query(index);
    cout << "Main: query generated" << endl;

    PirReply reply_a = server_a.generate_reply(query, 0);
    PirReply reply_b = server_b.generate_reply(query, 0);
    PirReply reply_c = server_c.generate_reply(query, 0);

    return 0;
}