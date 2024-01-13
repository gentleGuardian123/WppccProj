#include "pir.hpp"
#include "pir_server.hpp"
#include "pir_client.hpp"
#include "imp_data.hpp"

#include <seal/seal.h>

using namespace std;
using namespace seal;

int main(int argc, char *argv[]) {
    uint8_t dim_of_items_number = 32;
    uint64_t number_of_items = 1 << dim_of_items_number;
    uint64_t size_per_item = 1; // in bytes
    uint32_t N = 4096;
    uint32_t logt = 20;
    uint32_t d = 2;
    bool use_symmetric = true; 
    bool use_batching = true;  
    bool use_recursive_mod_switching = true;
    EncryptionParameters enc_params(scheme_type::bfv);
    PirParams pir_params;

    gen_encryption_params(N, logt, enc_params);
    gen_pir_params(number_of_items, size_per_item, d, enc_params, pir_params,
                   use_symmetric, use_batching, use_recursive_mod_switching);
    verify_encryption_params(enc_params);

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

    // generate random 32-bit desired index vec.
    vector<uint64_t> desired_index_vec;
    {
        uint64_t index_mod = 1UL << 32;
        for (int i = 0; i < 10; i ++ ) {
            desired_index_vec.push_back(rd() % index_mod);
        }
    }
/* 
    for (auto it = desired_index_vec.begin(); it != desired_index_vec.end(); it ++ ) {
        cout << "desired_index_vec[]:" << *it << endl;
    }
 */
    // generate batch query for these desired index vec.
    vector<Index> elem_index_with_ptr;
    list<FvInfo> fv_info_list;
    vector<PirQuery> batch_pir_query;
    batch_pir_query = client.generate_batch_query(desired_index_vec, elem_index_with_ptr, fv_info_list);
/* 
    for (auto it = fv_info_list.begin(); it != fv_info_list.end(); it ++ ) {
        cout << "&fv_info_list[]: " << &(*it) << endl;
        cout << "fv_info_list[].index_value: " << it->index_value << endl;
        cout << "fv_info_list[].fv_offset: " << it->fv_offset << endl;
        cout << "fv_info_list[].reply_id: " << it->reply_id << endl;
    }

    for (auto it = elem_index_with_ptr.begin(); it != elem_index_with_ptr.end(); it ++ ) {
        cout << "elem_index_with_ptr[]->index_value:" << it->index_value << endl;
        cout << "elem_index_with_ptr[]->fv_info_ptr:" << it->fv_info_ptr << endl;
        cout << "elem_index_with_ptr[]->fv_info_ptr->index_value:" << it->fv_info_ptr->index_value << endl;
        cout << "elem_index_with_ptr[]->fv_info_ptr->fv_offset:" << it->fv_info_ptr->fv_offset << endl;
        cout << "elem_index_with_ptr[]->fv_info_ptr->reply_id:" << it->fv_info_ptr->reply_id << endl;
    }
 */

    // generate reply for batch query.
    vector<PirReply> batch_pir_reply;
    for ( PirQuery query: batch_pir_query ) {
        PirReply reply = server.generate_reply(query, 0);
        batch_pir_reply.push_back(reply);
    }

    // decode batch relpy.
    vector<vector<uint8_t>> elems = client.decode_batch_reply(batch_pir_reply, elem_index_with_ptr);

    assert(elems.size() == desired_index_vec.size());
    for (auto it = elems.begin(); it != elems.end(); it ++ ) {
        assert((*it).size() == size_per_item);
    }
    bool failed = false;

    // notice that we assume that the data size is **1 byte**.
    for (uint64_t i = 0UL; i < elems.size(); i ++ ) {
        if (elems[i][0] != db_copy.get()[desired_index_vec[i]]) {
            cout << "Main: elems " << (int)elems[i][0] << ", db"
                << db_copy.get()[desired_index_vec[i]] << endl;
            cout << "Main: PIR result wrong at " << i << endl;
            failed = true;
        }
    }

    if (failed) {
      return -1;
    }

}
