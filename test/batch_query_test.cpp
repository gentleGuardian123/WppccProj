#include "pir.hpp"
#include "pir_server.hpp"
#include "pir_client.hpp"

#include <seal/seal.h>
#include <chrono>

// #define DEBUG

using namespace std;
using namespace seal;
using namespace std::chrono;


int main(int argc, char *argv[]) {
    uint8_t dim_of_items_number = argc > 1 ? stoi(argv[1]) : 18;
    uint64_t number_of_items = (1UL << dim_of_items_number);
    uint64_t size_per_item = argc > 2 ? stoi(argv[2]) : 512; // in bytes
    uint64_t batch_num = argc > 3 ? stoi(argv[3]) : 500;
    uint8_t constant_index_rate = argc > 4 ? stoi(argv[4]) : 80; // percents
    uint64_t constant_index_num = (uint32_t) (batch_num * constant_index_rate / 100.0);
    uint32_t N = argc > 5 ? stoi(argv[5]) : 4096;
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
    assert(constant_index_rate <= 100);
    cout << "Main: The constant index rate is: " << (int)constant_index_rate << "%." << endl;

    PIRClient client(enc_params, pir_params);
    GaloisKeys galois_keys = client.generate_galois_keys();
    PIRServer server(enc_params, pir_params);
    server.set_galois_key(0, galois_keys);

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
    cout << "Main: Generated random database successfully." << endl;
    auto time_pre_s = high_resolution_clock::now();
    server.set_database(move(db), number_of_items, size_per_item);
    server.preprocess_database();
    auto time_pre_e = high_resolution_clock::now();
    auto time_pre_us = duration_cast<microseconds>(time_pre_e - time_pre_s).count();

    // generate random 32-bit desired index vec.
    vector<uint64_t> desired_index_vec;
    for (uint64_t i = 0; i < batch_num - constant_index_num; i ++ ) {
        // discrete indices.
        desired_index_vec.push_back(rd() % number_of_items);
    }
    uint64_t start_point = rd() % (number_of_items - constant_index_num);
    for (uint64_t i = 0; i < constant_index_num; i ++ ) {
        // constant indices.
        desired_index_vec.push_back(i + start_point);
    }

#ifdef DEBUG
    for (auto it = desired_index_vec.begin(); it != desired_index_vec.end(); it ++ ) {
        cout << "desired_index_vec[]:" << *it << endl;
    }
#endif

    // generate batch query for these desired index vec.
    auto time_query_s = high_resolution_clock::now();
    vector<Index> elem_index_with_ptr;
    list<FvInfo> fv_info_list;
    vector<PirQuery> batch_pir_query;
    batch_pir_query = client.generate_batch_query(desired_index_vec, elem_index_with_ptr, fv_info_list);
    auto time_query_e = high_resolution_clock::now();
    auto time_query_us = duration_cast<microseconds>(time_query_e - time_query_s).count();
    cout << "Main: Number of batched pir queries is " << batch_pir_query.size() << "." << endl;

#ifdef DEBUG
    for (auto it = fv_info_list.begin(); it != fv_info_list.end(); it ++ ) {
        cout << "&fv_info_list[]: " << &(*it) << endl;
        cout << "fv_info_list[].index_value: " << it->index_value << endl;
        cout << "fv_info_list[].fv_offset: " << it->fv_offset << endl;
        cout << "fv_info_list[].reply_id: " << it->reply_id << endl;
    }

    for (auto it = elem_index_with_ptr.begin(); it != elem_index_with_ptr.end(); it ++ ) {
        cout << "elem_index_with_ptr[]->index_value: " << it->index_value << endl;
        cout << "elem_index_with_ptr[]->fv_info_ptr: " << it->fv_info_ptr << endl;
        cout << "elem_index_with_ptr[]->fv_info_ptr->index_value: " << it->fv_info_ptr->index_value << endl;
        cout << "elem_index_with_ptr[]->fv_info_ptr->fv_offset: " << it->fv_info_ptr->fv_offset << endl;
        cout << "elem_index_with_ptr[]->fv_info_ptr->reply_id: " << it->fv_info_ptr->reply_id << endl;
    }
#endif


    stringstream client_stream;
    stringstream server_stream;
    auto time_s_query_s = high_resolution_clock::now();
    int query_size = 0;
    for (FvInfo fv: fv_info_list) {
        query_size += client.generate_serialized_query(fv.index_value, client_stream);
    }
    auto time_s_query_e = high_resolution_clock::now();
    auto time_s_query_us = duration_cast<microseconds>(time_s_query_e - time_s_query_s).count();
    cout << "Main: Query serialized." << endl;


    auto time_deserial_s = high_resolution_clock::now();
    PirQuery query2 = server.deserialize_query(client_stream);
    auto time_deserial_e = high_resolution_clock::now();
    auto time_deserial_us = duration_cast<microseconds>(time_deserial_e - time_deserial_s).count();
    cout << "Main: Query deserialized." << endl;


    // generate batch reply for batch query.
    auto time_server_s = high_resolution_clock::now();
    vector<PirReply> batch_pir_reply;
    size_t count = 0;
    for ( auto query: batch_pir_query ) {
        if (! (count % (batch_pir_query.size() / 10))) { cout << "Server: Generating " << count + 1 << "-th reply..." << endl; }
        PirReply reply = server.generate_reply(query, 0);
        batch_pir_reply.push_back(reply);
        count ++;
    }
    cout << "Server: Generated " << batch_pir_reply.size() << " replies." << endl;
    auto time_server_e = high_resolution_clock::now();
    auto time_server_us = duration_cast<microseconds>(time_server_e - time_server_s).count();
    

    int reply_size = 0;
    for ( PirReply reply: batch_pir_reply) {
        reply_size += server.serialize_reply(reply, server_stream);
    }


    // decode batch relpy.
    auto time_decode_s = chrono::high_resolution_clock::now();
    vector<vector<uint8_t>> elems = client.debatch_reply(batch_pir_reply, elem_index_with_ptr);
    auto time_decode_e = chrono::high_resolution_clock::now();
    auto time_decode_us = duration_cast<microseconds>(time_decode_e - time_decode_s).count();

    assert(elems.size() == desired_index_vec.size());
    for ( auto it = elems.begin(); it != elems.end(); it ++ ) {
        assert((*it).size() == size_per_item);
    }
    bool failed = false;

    for (uint64_t i = 0UL; i < elems.size(); i ++) {
        for (uint32_t j = 0; j < size_per_item; j ++) {
            if (elems[i][j] != db_copy.get()[desired_index_vec[i] * size_per_item + j]) {
                cout << "Main: elems " << (int)elems[i][j] << ", db"
                    << db_copy.get()[desired_index_vec[i] * size_per_item + i] << endl;
                cout << "Main: PIR result wrong at " << i << endl;
                failed = true;
            }
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
    cout << "Main: Query size: " << query_size / (1024.0 * 1024.0) << " MB" << endl;
    cout << "Main: Compressed query numbers: " << batch_pir_query.size() << endl;
    cout << "Main: Reply size: " << reply_size / (1024.0 * 1024.0) << " MB" << endl;

    if (argc > 5) {
        cout << time_pre_us / 1000 << ","
            << time_query_us / 1000 << ","
            << time_s_query_us / 1000 << ","
            << time_deserial_us << ","
            << time_server_us / 1000 << ","
            << time_decode_us /1000 << ","
            << query_size / (1024.0 * 1024.0) << ","
            << batch_pir_query.size() << ","
            << reply_size / (1024.0 * 1024.0);
    }

    return 0;

}
