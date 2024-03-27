#include "pir.hpp"
#include "pir_server.hpp"
#include "pir_client.hpp"
#include "imp_data.hpp"

#include <seal/seal.h>
#include <iomanip>

using namespace std;
using namespace seal;

int main(int argc, char *argv[]) {
    uint8_t dim_of_items_number = 12;
    uint64_t number_of_items = (1UL << dim_of_items_number);
    uint64_t size_per_item = 1024; // in byte
    uint32_t N = 4096;
    uint32_t logt = 16;
    uint32_t d = 1;
    bool use_symmetric = true; 
    bool use_batching = true;
    bool use_recursive_mod_switching = true;
    EncryptionParameters enc_params(scheme_type::bfv);
    PirParams pir_params;

    gen_encryption_params(N, logt, enc_params);
    verify_encryption_params(enc_params);
    gen_pir_params(number_of_items, size_per_item, d, enc_params, pir_params,
                   use_symmetric, use_batching, use_recursive_mod_switching);
    print_seal_params(enc_params);
    print_pir_params(pir_params);

    PIRClient client(enc_params, pir_params);
    GaloisKeys galois_keys = client.generate_galois_keys();
    PIRServer server_A(enc_params, pir_params);
    PIRServer server_B(enc_params, pir_params);
    PIRServer server_C(enc_params, pir_params);
    server_A.set_galois_key(0, galois_keys);
    server_B.set_galois_key(0, galois_keys);
    server_C.set_galois_key(0, galois_keys);

    // generate random database.
    random_device rd;
    auto db_A(make_unique<uint8_t[]>(number_of_items * size_per_item));
    auto db_B(make_unique<uint8_t[]>(number_of_items * size_per_item));
    auto db_C(make_unique<uint8_t[]>(number_of_items * size_per_item));
    auto db_A_copy(make_unique<uint8_t[]>(number_of_items * size_per_item));
    auto db_B_copy(make_unique<uint8_t[]>(number_of_items * size_per_item));
    auto db_C_copy(make_unique<uint8_t[]>(number_of_items * size_per_item));
    for (uint64_t i = 0; i < number_of_items; i++) {
        for (uint64_t j = 0; j < size_per_item; j++) {
            uint8_t val_A = rd() % 80;
            uint8_t val_B = rd() % 80;
            uint8_t val_C = rd() % 80;
            db_A.get()[(i * size_per_item) + j] = val_A;
            db_B.get()[(i * size_per_item) + j] = val_B;
            db_C.get()[(i * size_per_item) + j] = val_C;
            db_A_copy.get()[(i * size_per_item) + j] = val_A;
            db_B_copy.get()[(i * size_per_item) + j] = val_B;
            db_C_copy.get()[(i * size_per_item) + j] = val_C;
        }
    }
    cout << "Main: Generated random database successfully." << endl;

    server_A.set_database(move(db_A), number_of_items, size_per_item);
    server_B.set_database(move(db_B), number_of_items, size_per_item);
    server_C.set_database(move(db_C), number_of_items, size_per_item);
    server_A.preprocess_database();
    server_B.preprocess_database();
    server_C.preprocess_database();

    vector<uint64_t> desired_index_vec;
    for (int i = 0; i < 10; i ++ ) {
        // discrete indices.
        desired_index_vec.push_back(rd() % number_of_items);
    }
    for (int i = 0; i < 10; i ++ ) {
        // constant indices.
        desired_index_vec.push_back(i + 10000);
    }
    cout << "Main: Generated 20(10 discrete, 10 constant) random desired indices for batch query." << endl;
    cout << endl;

    vector<Index> elem_index_with_ptr;
    list<FvInfo> fv_info_list;
    vector<PirQuery> batch_pir_query;
    batch_pir_query = client.generate_batch_query(desired_index_vec, elem_index_with_ptr, fv_info_list);
    cout << "Main: Generated batch query for desired indices." << endl;
    cout << "Main: Number of batched pir queries is " << batch_pir_query.size() << endl;
}