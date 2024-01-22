#include "pir.hpp"
#include "pir_server.hpp"
#include "pir_client.hpp"
#include "imp_data.hpp"

#include <seal/seal.h>

using namespace std;
using namespace seal;

int main(int argc, char *argv[]) {
    uint8_t dim_of_items_number = 16;
    uint64_t number_of_items = (1UL << dim_of_items_number);
    uint64_t size_per_item = 1024; // in bytes
    uint32_t N = 4096;
    uint32_t logt = 20;
    uint32_t d = 2;
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
    server_A.set_galois_key(0, galois_keys);
    PIRServer server_B(enc_params, pir_params);
    server_B.set_galois_key(0, galois_keys);
    PIRServer server_C(enc_params, pir_params);
    server_C.set_galois_key(0, galois_keys);

    // generate random database.
    random_device rd;
    auto db(make_unique<uint8_t[]>(number_of_items * size_per_item));
    auto db_copy(make_unique<uint8_t[]>(number_of_items * size_per_item));
    for (uint64_t i = 0; i < number_of_items; i++) {
        for (uint64_t j = 0; j < size_per_item; j++) {
            uint8_t val = rd() % 256;
            db.get()[(i * size_per_item) + j] = val;
            db_copy.get()[(i * size_per_item) + j] = val;
        }
    }
    cout << "Generated random database successfully." << endl;
    cout << endl;

    server_A.set_database(move(db), number_of_items, size_per_item);
    server_A.preprocess_database();
    server_B.set_database(move(db), number_of_items, size_per_item);
    server_B.preprocess_database();
    server_C.set_database(move(db), number_of_items, size_per_item);
    server_C.preprocess_database();

    uint64_t elem_index_A = rd() % number_of_items;
    uint64_t elem_index_B = rd() % number_of_items;
    uint64_t elem_index_C = rd() % number_of_items;
    uint64_t fv_index_A = client.get_fv_index(elem_index_A);
    uint64_t fv_index_B = client.get_fv_index(elem_index_B);
    uint64_t fv_index_C = client.get_fv_index(elem_index_C);
    uint64_t fv_offset_A = client.get_fv_offset(elem_index_A);
    uint64_t fv_offset_B = client.get_fv_offset(elem_index_B);
    uint64_t fv_offset_C = client.get_fv_offset(elem_index_C);
    cout << "Main: Generated single query element index for Server A,B,C respectively." << endl;
    cout << "Main: Desired element index for Server A: " << elem_index_A << "; fv index: " << fv_index_A << "; fv offset: " << fv_offset_A << "." << endl; 
    cout << "Main: Desired element index for Server B: " << elem_index_B << "; fv index: " << fv_index_B << "; fv offset: " << fv_offset_B << "." << endl; 
    cout << "Main: Desired element index for Server C: " << elem_index_C << "; fv index: " << fv_index_C << "; fv offset: " << fv_offset_C << "." << endl; 
    cout << endl;

    PirQuery query_A = client.generate_query(fv_index_A);
    PirQuery query_B = client.generate_query(fv_index_B);
    PirQuery query_C = client.generate_query(fv_index_C);

    cout << "Main: Assume Server A is the host server generating random numbers for confusion." << endl;
    uint64_t r1, r2, r3;
    server_A.gen_rand_trio(r1, r2, r3);
    cout << "Server A: Generated random number triple, and output it to Server B,C." << endl;
    cout << "Server A: r1, r2, r3 are " << r1 << ", " << r2 << ", " << r3 << " respectively, satisfying r1+r2+r3 = 0 (mod plain_modulus)." << endl;
    cout << endl;

    PirReply reply_A = server_A.generate_reply_with_add_confusion(query_A, 0, r1);
    PirReply reply_B = server_B.generate_reply_with_add_confusion(query_B, 0, r2);
    PirReply reply_C = server_C.generate_reply_with_add_confusion(query_C, 0, r3);
    cout << "Servers: Generated replies utilizing additive confusion." << endl;

}