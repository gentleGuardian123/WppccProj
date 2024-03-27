#include "pir.hpp"
#include "pir_server.hpp"
#include "pir_client.hpp"
#include "imp_data.hpp"

#include <seal/seal.h>
#include <iomanip>
#include <chrono>

// #define DEBUG
#ifdef DEBUG
    #include <fstream>
#endif

using namespace std::chrono;
using namespace std;
using namespace seal;

int main(int argc, char *argv[]) {
    uint8_t dim_of_items_number = 12;
    uint64_t number_of_items = (1UL << dim_of_items_number);
    uint64_t size_per_item = 1024; // in bytes
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
            uint8_t val = rd() % 80;
            db_A.get()[(i * size_per_item) + j] = val;
            db_B.get()[(i * size_per_item) + j] = val;
            db_C.get()[(i * size_per_item) + j] = val;
            db_A_copy.get()[(i * size_per_item) + j] = val;
            db_B_copy.get()[(i * size_per_item) + j] = val;
            db_C_copy.get()[(i * size_per_item) + j] = val;
            #ifdef DEBUG
                // db_A.get()[(i * size_per_item) + j] = 1;
                // db_A_copy.get()[(i * size_per_item) + j] = 1;
                // db_B.get()[(i * size_per_item) + j] = 1;
                // db_B_copy.get()[(i * size_per_item) + j] = 1;
                // db_C.get()[(i * size_per_item) + j] = 1;
                // db_C_copy.get()[(i * size_per_item) + j] = 1;
            #endif
        }
    }
    cout << "Main: Generated random database successfully." << endl;
    cout << endl;

    auto time_pre_s = high_resolution_clock::now();
    server_A.set_database(move(db_A), number_of_items, size_per_item);
    server_B.set_database(move(db_B), number_of_items, size_per_item);
    server_C.set_database(move(db_C), number_of_items, size_per_item);
    server_A.preprocess_database();
    server_B.preprocess_database();
    server_C.preprocess_database();
    auto time_pre_e = high_resolution_clock::now();
    auto time_pre_us = duration_cast<microseconds>(time_pre_e - time_pre_s).count();

    uint64_t elem_index = rd() % number_of_items;
    uint64_t fv_index = client.get_fv_index(elem_index);
    uint64_t fv_offset = client.get_fv_offset(elem_index);
    cout << "Main: Generated single query element index for Server A,B,C." << endl;
    cout << "Main: Desired element index for Server A,B,C: " << elem_index << "; fv index: " << fv_index << "; fv offset: " << fv_offset << "." << endl; 
    cout << endl;

    auto time_query_s = high_resolution_clock::now();
    PirQuery query = client.generate_query(fv_index);
    auto time_query_e = high_resolution_clock::now();
    auto time_query_us = duration_cast<microseconds>(time_query_e - time_query_s).count();


    stringstream client_stream;
    stringstream server_stream;
    auto time_s_query_s = high_resolution_clock::now();
    int query_size = client.generate_serialized_query(elem_index, client_stream);
    auto time_s_query_e = high_resolution_clock::now();
    auto time_s_query_us =
        duration_cast<microseconds>(time_s_query_e - time_s_query_s).count();
    cout << "Main: Query serialized." << endl;

    auto time_deserial_s = high_resolution_clock::now();
    PirQuery deserialized_query = server_A.deserialize_query(client_stream);
    auto time_deserial_e = high_resolution_clock::now();
    auto time_deserial_us =
         duration_cast<microseconds>(time_deserial_e - time_deserial_s).count();
    cout << "Main: Query deserialized." << endl;


    auto time_server_s = high_resolution_clock::now();
    cout << "Main: Assume Server A is the host server generating random numbers for confusion." << endl;
    uint64_t r1, r2, r3;
    server_A.gen_rand_trio(r1, r2, r3);
    cout << "Server A: Generated random number triple, and output it to Server B,C." << endl;
    cout << "Server A: r1, r2, r3 are " << r1 << ", " << r2 << ", " << r3 << " respectively, satisfying r1+r2+r3 = "
         << (r1 + r2 + r3) % enc_params.plain_modulus().value() << " (mod plain_modulus)." << endl;
    cout << endl;

    PirReply reply_A = server_A.generate_reply_with_add_confusion(query, 0, r1);
    PirReply reply_B = server_B.generate_reply_with_add_confusion(query, 0, r2);
    PirReply reply_C = server_C.generate_reply_with_add_confusion(query, 0, r3);
    cout << "Servers: Generated replies utilizing additive confusion." << endl;
    auto time_server_e = high_resolution_clock::now();
    auto time_server_us = duration_cast<microseconds>(time_server_e - time_server_s).count();

    int reply_size = server_A.serialize_reply(reply_A, server_stream);
    reply_size += server_B.serialize_reply(reply_B, server_stream);
    reply_size += server_C.serialize_reply(reply_C, server_stream);

    auto time_decode_s = chrono::high_resolution_clock::now();
    vector<PirReply> replies({reply_A, reply_B, reply_C});
    vector<uint8_t> elems_deconfused = client.deconfuse_and_decode_replies(replies, fv_offset);
    cout << "Client: Deconfused and decoded replies." << endl;
    cout << endl;
    auto time_decode_e = chrono::high_resolution_clock::now();
    auto time_decode_us = duration_cast<microseconds>(time_decode_e - time_decode_s).count();

    cout << "Client: Result from deconfused and decoded replies:" << endl;
    for (int i = 0; i < size_per_item / 64; i ++) {
        for (int j = 0; j < 64; j ++) {
            cout << setfill('0') << setw(2) << hex
             << (int)elems_deconfused[i * (size_per_item/64) + j]
             << " ";
        }
        cout << endl;
    }

    cout << "Client: Original data (db_A + db_B + db_C):" << endl;
    for (int i = 0; i < size_per_item / 64; i ++) {
        for (int j = 0; j < 64; j ++) {
            cout << setfill('0') << setw(2) << hex
             << (int)db_A_copy.get()[(elem_index * size_per_item) + i * (size_per_item/64) + j]
                + (int)db_B_copy.get()[(elem_index * size_per_item) + i * (size_per_item/64) + j]
                + (int)db_C_copy.get()[(elem_index * size_per_item) + i * (size_per_item/64) + j]
             << " ";
        }
        cout << endl;
    }

#ifdef DEBUG
    ofstream debuglog;
    debuglog.open("../src/debug/debug_log2.txt");
    debuglog << "Client: Results from deconfused and decoded replies:" << endl;
    for (int i = 0; i < size_per_item / 64; i ++) {
        for (int j = 0; j < 64; j ++) {
            debuglog << setfill('0') << setw(2) << hex
             << (int)elems_deconfused[i * (size_per_item/64) + j]
             << " ";
        }
        debuglog << endl;
    }

    debuglog << "Client: Original data (db_A + db_B + db_C):" << endl;
    for (int i = 0; i < size_per_item / 64; i ++) {
        for (int j = 0; j < 64; j ++) {
            debuglog << setfill('0') << setw(2) << hex
             << (int)db_A_copy.get()[(elem_index * size_per_item) + i * (size_per_item/64) + j]
                + (int)db_B_copy.get()[(elem_index * size_per_item) + i * (size_per_item/64) + j]
                + (int)db_C_copy.get()[(elem_index * size_per_item) + i * (size_per_item/64) + j]
             << " ";
        }
        debuglog << endl;
    }
    debuglog.close();
#endif

    size_t differ = 0;
    for (int i = 0; i < size_per_item; i ++) {
        if ((int)elems_deconfused[i] != 
                ((int)db_A_copy.get()[elem_index*size_per_item + i]
                 + (int)db_B_copy.get()[elem_index*size_per_item + i]
                 + (int)db_C_copy.get()[elem_index*size_per_item + i])) {
            differ ++;
        }
    }
    if (differ) {
        cout << "Client: Wrong! number of different data piece: " << dec << differ << endl;
    } else {
        cout << "Client: Correct!" << endl;
    }

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
    cout << "Main: Query size: " << dec << query_size << " bytes" << endl;
    cout << "Main: Reply num ciphertexts: " << reply_A.size() + reply_B.size() + reply_C.size() << endl;
    cout << "Main: Reply size: " << dec << reply_size << " bytes" << endl;

}