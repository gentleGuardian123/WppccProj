#include "pir.hpp"
#include "pir_client.hpp"
#include "pir_server.hpp"
#include "imp_data.hpp"
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <random>

using namespace std::chrono;
using namespace std;
using namespace seal;

int main()
{

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
    gen_encryption_params(N, logt, enc_params);
    gen_pir_params(number_of_items, size_per_item, d, enc_params, pir_params,
                    use_symmetric, use_batching, use_recursive_mod_switching);

    random_device rd;
    for (int i = 0; i < 100; i ++) {
        uint64_t rand = rd() % enc_params.plain_modulus().value();
        uint64_t inv_rand = invert_mod(rand, enc_params.plain_modulus());
        cout << "Rand and Inv_Rand of loop " << i << ":\t" << rand << ", " << inv_rand << endl;
    }
}
