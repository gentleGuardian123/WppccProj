#include "pir.hpp"
#include "pir_server.hpp"
#include "pir_client.hpp"
#include "imp_data.hpp"

#include <seal/seal.h>

using namespace std;
using namespace seal;

int main(int argc, char *argv[]) {
    uint8_t dim_of_items_number = 12;
    uint64_t number_of_items = (1UL << dim_of_items_number);
    uint64_t size_per_item = 1024; // in bytes
    uint32_t N = 4096;
    uint32_t logt = 20;
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

    SEALContext context = SEALContext(enc_params, true);
    Evaluator evaluator = Evaluator(context);
    KeyGenerator keygen = KeyGenerator(context);
    PublicKey pk;
    keygen.create_public_key(pk);
    SecretKey sk = keygen.secret_key();
    Encryptor encryptor = Encryptor(context, pk);
    Decryptor decryptor = Decryptor(context, sk);

    PIRClient client(enc_params, pir_params);
    GaloisKeys galois_keys = client.generate_galois_keys();
    PIRServer server(enc_params, pir_params);

    uint64_t r1, r2, r3;
    server.gen_rand_trio(r1, r2, r3);
    cout << "Main: Random number r1,r2,r3: " << r1 << ", " << r2 << ", " << r3 << "." << endl;
    cout << "Main: (r1+r2+r3) = " << (r1+r2+r3) << endl;
    cout << "Main: (r1+r2+r3) mod plain_modulus = "
         << (r1+r2+r3) % enc_params.plain_modulus().value() << endl;
    Plaintext rand_pt1 = server.gen_rand_pt(r1);
    Plaintext rand_pt2 = server.gen_rand_pt(r2);
    Plaintext rand_pt3 = server.gen_rand_pt(r3);
    cout << "Main: Random plain text 1: " << rand_pt1.to_string() << endl;
    cout << "Main: Random plain text 2: " << rand_pt2.to_string() << endl;
    cout << "Main: Random plain text 3: " << rand_pt3.to_string() << endl;
    // evaluator.transform_to_ntt_inplace(rand_pt1, context.first_parms_id());
    // evaluator.transform_to_ntt_inplace(rand_pt2, context.first_parms_id());
    // evaluator.transform_to_ntt_inplace(rand_pt3, context.first_parms_id());

    Ciphertext sum;
    cout << "Main: Modulus in `main` is: " 
         << enc_params.plain_modulus().value() << endl;
    encryptor.encrypt(rand_pt1, sum);
    evaluator.add_plain_inplace(sum, rand_pt2);
    evaluator.add_plain_inplace(sum, rand_pt3);
    
    Plaintext result;
    decryptor.decrypt(sum, result);
    cout << "Main: Directly computing result: " << result.to_string() << endl;

}