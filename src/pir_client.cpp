#include "pir_client.hpp"

// #define DEBUG

#ifdef DEBUG
    #include <iomanip>
    #include <fstream>
#endif

using namespace std;
using namespace seal;
using namespace seal::util;

PIRClient::PIRClient(const EncryptionParameters &enc_params,
                     const PirParams &pir_params)
    : enc_params_(enc_params), pir_params_(pir_params) {

  context_ = make_shared<SEALContext>(enc_params, true);

  keygen_ = make_unique<KeyGenerator>(*context_);

  PublicKey public_key;
  keygen_->create_public_key(public_key);
  SecretKey secret_key = keygen_->secret_key();

  if (pir_params_.enable_symmetric) {
    encryptor_ = make_unique<Encryptor>(*context_, secret_key);
  } else {
    encryptor_ = make_unique<Encryptor>(*context_, public_key);
  }

  decryptor_ = make_unique<Decryptor>(*context_, secret_key);
  evaluator_ = make_unique<Evaluator>(*context_);
  encoder_ = make_unique<BatchEncoder>(*context_);
}

int PIRClient::generate_serialized_query(uint64_t desiredIndex,
                                         std::stringstream &stream) {

  int N = enc_params_.poly_modulus_degree();
  int output_size = 0;
  indices_ = compute_indices(desiredIndex, pir_params_.nvec);
  Plaintext pt(enc_params_.poly_modulus_degree());

  for (uint32_t i = 0; i < indices_.size(); i++) {
    uint32_t num_ptxts = ceil((pir_params_.nvec[i] + 0.0) / N);
    // initialize result.
    // cout << "Client: index " << i + 1 << "/ " << indices_.size() << " = "
    //      << indices_[i] << endl;
    // cout << "Client: number of ctxts needed for query = " << num_ptxts << endl;

    for (uint32_t j = 0; j < num_ptxts; j++) {
      pt.set_zero();
      if (indices_[i] >= N * j && indices_[i] <= N * (j + 1)) {
        uint64_t real_index = indices_[i] - N * j;
        uint64_t n_i = pir_params_.nvec[i];
        uint64_t total = N;
        if (j == num_ptxts - 1) {
          total = n_i % N;
        }
        uint64_t log_total = ceil(log2(total));

        // cout << "Client: Inverting " << pow(2, log_total) << endl;
        pt[real_index] =
            invert_mod(pow(2, log_total), enc_params_.plain_modulus());
      }

      if (pir_params_.enable_symmetric) {
        output_size += encryptor_->encrypt_symmetric(pt).save(stream);
      } else {
        output_size += encryptor_->encrypt(pt).save(stream);
      }
    }
  }

  return output_size;
}

PirQuery PIRClient::generate_query(uint64_t desiredIndex) {

  indices_ = compute_indices(desiredIndex, pir_params_.nvec);

  PirQuery result(pir_params_.d);
  int N = enc_params_.poly_modulus_degree();

  Plaintext pt(enc_params_.poly_modulus_degree());
  for (uint32_t i = 0; i < indices_.size(); i++) {
    uint32_t num_ptxts = ceil((pir_params_.nvec[i] + 0.0) / N);
    // initialize result.
    // cout << "Client: index " << i + 1 << "/ " << indices_.size() << " = "
    //      << indices_[i] << endl;
    // cout << "Client: number of ctxts needed for query = " << num_ptxts << endl;

    for (uint32_t j = 0; j < num_ptxts; j++) {
      pt.set_zero();
      if (indices_[i] >= N * j && indices_[i] <= N * (j + 1)) {
        uint64_t real_index = indices_[i] - N * j;
        uint64_t n_i = pir_params_.nvec[i];
        uint64_t total = N;
        if (j == num_ptxts - 1) {
          total = n_i % N;
        }
        uint64_t log_total = ceil(log2(total));

        // cout << "Client: Inverting " << pow(2, log_total) << endl;
        pt[real_index] =
            invert_mod(pow(2, log_total), enc_params_.plain_modulus());
      }
      Ciphertext dest;
      if (pir_params_.enable_symmetric) {
        encryptor_->encrypt_symmetric(pt, dest);
      } else {
        encryptor_->encrypt(pt, dest);
      }
      result[i].push_back(dest);
    }
  }

  return result;
}

uint64_t PIRClient::get_fv_index(uint64_t element_index) {
  return static_cast<uint64_t>(element_index /
                               pir_params_.elements_per_plaintext);
}

uint64_t PIRClient::get_fv_offset(uint64_t element_index) {
  return element_index % pir_params_.elements_per_plaintext;
}

Plaintext PIRClient::decrypt(Ciphertext ct) {
  Plaintext pt;
  decryptor_->decrypt(ct, pt);
  return pt;
}

vector<uint8_t> PIRClient::decode_reply(PirReply &reply, uint64_t offset) {
  Plaintext result = decode_reply(reply);
  return extract_bytes(result, offset);
}

vector<uint64_t> PIRClient::extract_coeffs(Plaintext pt) {
  vector<uint64_t> coeffs;
  encoder_->decode(pt, coeffs);
  return coeffs;
}

std::vector<uint64_t> PIRClient::extract_coeffs(seal::Plaintext pt,
                                                uint64_t offset) {
  vector<uint64_t> coeffs;
  encoder_->decode(pt, coeffs);

  uint32_t logt = floor(log2(enc_params_.plain_modulus().value()));

  uint64_t coeffs_per_element =
      coefficients_per_element(logt, pir_params_.ele_size);

  return std::vector<uint64_t>(coeffs.begin() + offset * coeffs_per_element,
                               coeffs.begin() +
                                   (offset + 1) * coeffs_per_element);
}

std::vector<uint8_t> PIRClient::extract_bytes(seal::Plaintext pt,
                                              uint64_t offset) {
  uint32_t N = enc_params_.poly_modulus_degree();
  uint32_t logt = floor(log2(enc_params_.plain_modulus().value()));
  uint32_t bytes_per_ptxt =
      pir_params_.elements_per_plaintext * pir_params_.ele_size;

  // Convert from FV plaintext (polynomial) to database element at the client
  vector<uint8_t> elems(bytes_per_ptxt);
  vector<uint64_t> coeffs;
  encoder_->decode(pt, coeffs);
  coeffs_to_bytes(logt, coeffs, elems.data(), bytes_per_ptxt,
                  pir_params_.ele_size);
  return std::vector<uint8_t>(elems.begin() + offset * pir_params_.ele_size,
                              elems.begin() +
                                  (offset + 1) * pir_params_.ele_size);
}

Plaintext PIRClient::decode_reply(PirReply &reply) {
  EncryptionParameters parms;
  parms_id_type parms_id;
  if (pir_params_.enable_mswitching) {
    parms = context_->last_context_data()->parms();
    parms_id = context_->last_parms_id();
  } else {
    parms = context_->first_context_data()->parms();
    parms_id = context_->first_parms_id();
  }
  uint32_t exp_ratio = compute_expansion_ratio(parms);
  uint32_t recursion_level = pir_params_.d;

  vector<Ciphertext> temp = reply;
  uint32_t ciphertext_size = temp[0].size();

  uint64_t t = enc_params_.plain_modulus().value();

  for (uint32_t i = 0; i < recursion_level; i++) {
    // cout << "Client: " << i + 1 << "/ " << recursion_level
    //      << "-th decryption layer started." << endl;
    vector<Ciphertext> newtemp;
    vector<Plaintext> tempplain;

    for (uint32_t j = 0; j < temp.size(); j++) {
      Plaintext ptxt;
      decryptor_->decrypt(temp[j], ptxt);
// #ifdef DEBUG
//       cout << "Client: reply noise budget = "
//            << decryptor_->invariant_noise_budget(temp[j]) << endl;
// #endif

      // cout << "decoded (and scaled) plaintext = " << ptxt.to_string() <<
      // endl;
      tempplain.push_back(ptxt);

// #ifdef DEBUG
//       cout << "recursion level : " << i << " noise budget :  ";
//       cout << decryptor_->invariant_noise_budget(temp[j]) << endl;
// #endif

      if ((j + 1) % (exp_ratio * ciphertext_size) == 0 && j > 0) {
        // Combine into one ciphertext.
        Ciphertext combined(*context_, parms_id);
        compose_to_ciphertext(parms, tempplain, combined);
        newtemp.push_back(combined);
        tempplain.clear();
        // cout << "Client: const term of ciphertext = " << combined[0] << endl;
      }
    }
    // cout << "Client: done." << endl;
    // cout << endl;
    if (i == recursion_level - 1) {
      assert(temp.size() == 1);
      return tempplain[0];
    } else {
      tempplain.clear();
      temp = newtemp;
    }
  }

  // This should never be called
  assert(0);
  Plaintext fail;
  return fail;
}

GaloisKeys PIRClient::generate_galois_keys() {
  // Generate the Galois keys needed for coeff_select.
  vector<uint32_t> galois_elts;
  int N = enc_params_.poly_modulus_degree();
  int logN = get_power_of_two(N);

  // cout << "printing galois elements...";
  for (int i = 0; i < logN; i++) {
    galois_elts.push_back((N + exponentiate_uint(2, i)) /
                          exponentiate_uint(2, i));
    //#ifdef DEBUG
    // cout << galois_elts.back() << ", ";
    //#endif
  }
  GaloisKeys gal_keys;
  keygen_->create_galois_keys(galois_elts, gal_keys);
  return gal_keys;
}

Plaintext PIRClient::replace_element(Plaintext pt, vector<uint64_t> new_element,
                                     uint64_t offset) {
  vector<uint64_t> coeffs = extract_coeffs(pt);

  uint32_t logt = floor(log2(enc_params_.plain_modulus().value()));
  uint64_t coeffs_per_element =
      coefficients_per_element(logt, pir_params_.ele_size);

  assert(new_element.size() == coeffs_per_element);

  for (uint64_t i = 0; i < coeffs_per_element; i++) {
    coeffs[i + offset * coeffs_per_element] = new_element[i];
  }

  Plaintext new_pt;

  encoder_->encode(coeffs, new_pt);
  return new_pt;
}

Ciphertext PIRClient::get_one() {
  Plaintext pt("1");
  Ciphertext ct;
  if (pir_params_.enable_symmetric) {
    encryptor_->encrypt_symmetric(pt, ct);
  } else {
    encryptor_->encrypt(pt, ct);
  }
  return ct;
}

/* 
    Above are the codes of SealPIR; below are the new codes for the current project.
 */

// Input desired index vec; result stores in `elem_index_with_ptr` and `fv_info_list`.
vector<PirQuery> PIRClient::generate_batch_query(vector<uint64_t> &desired_index_vec, vector<Index> &elem_index_with_ptr, list<FvInfo> &fv_info_list) {

    vector<PirQuery> batch_pir_query;
    vector<uint64_t> batch_fv_index;
  
    for (uint64_t i = 0UL; i < desired_index_vec.size(); i ++) {
        FvInfo fv_info;
        fv_info.index_value = desired_index_vec[i];
        fv_info_list.push_back(fv_info);
        Index index;
        index.index_value = desired_index_vec[i];
        index.fv_info_ptr = &fv_info_list.back();
        elem_index_with_ptr.push_back(index);
    }

    fv_info_list.sort(
        [](const FvInfo &_f1, const FvInfo &_f2) { 
            return (_f1.index_value < _f2.index_value); 
        }
    );

    uint64_t first_fv_index = get_fv_index(fv_info_list.front().index_value);
    uint64_t reply_id = 0UL;
    batch_fv_index.push_back(first_fv_index);
    PirQuery first_query = generate_query(first_fv_index);
    batch_pir_query.push_back(first_query);

    for (auto &fv_info: fv_info_list) {
        uint64_t fv_index = get_fv_index(fv_info.index_value);
        fv_info.fv_offset = get_fv_offset(fv_info.index_value);
        
#ifdef DEBUG

#ifdef DEBUG
        cout << "fv_info_list[].fv_index:" <<  fv_index << endl;
        cout << "batch_fv_index.back():" << batch_fv_index.back() << endl;
#endif

#endif

        if (fv_index > batch_fv_index.back()) {
            batch_fv_index.push_back(fv_index);
            PirQuery query = generate_query(fv_index);
            batch_pir_query.push_back(query);
            fv_info.reply_id = ++reply_id;
        } else {
            fv_info.reply_id = reply_id;
        }
    }

    return batch_pir_query;
  
}

vector<vector<uint8_t>> PIRClient::debatch_reply(vector<PirReply> &batch_reply, vector<Index> &elem_index_with_ptr) {
    vector<vector<uint8_t>> elems;

    for (auto it = elem_index_with_ptr.begin(); it < elem_index_with_ptr.end(); it++) {
        uint64_t reply_id = it->fv_info_ptr->reply_id;
        uint64_t offset = it->fv_info_ptr->fv_offset;
        PirReply reply = batch_reply[reply_id];
        elems.push_back(decode_reply(reply, offset));
    }

    return elems;
}

vector<uint8_t> PIRClient::deconfuse_and_decode_replies(vector<PirReply> &replies, uint64_t offset) {
    uint64_t mod = enc_params_.plain_modulus().value();
    uint32_t logt = floor(log2(enc_params_.plain_modulus().value()));
    uint32_t N = enc_params_.poly_modulus_degree();
    uint64_t ele_per_ptxt = pir_params_.elements_per_plaintext;
    uint64_t ele_size = pir_params_.ele_size;
    uint64_t coeff_per_ele = coefficients_per_element(logt, ele_size);
    uint64_t coeff_per_ptxt = ele_per_ptxt * coeff_per_ele;
    assert(coeff_per_ptxt <= N);

    Plaintext result;
    vector<uint64_t> result_coeffs;
    vector<vector<uint64_t>> decoded_coeffs;

    for (auto reply: replies) {
        Plaintext decoded_reply = decode_reply(reply);
        vector<uint64_t> coeffs;
        encoder_->decode(decoded_reply, coeffs);
        decoded_coeffs.push_back(coeffs);
    }
#ifdef DEBUG
    ofstream debuglog;
    debuglog.open("../src/debug/debug_log3.txt");
    size_t cnt = 0;
    debuglog << "Client: `deconfuse_and_decode_replies` debug:" << endl;
    for (auto reply: replies) {
        debuglog << "decoding reply " << ++cnt << "..." << endl;
        Plaintext decoded_reply = decode_reply(reply);
        vector<uint8_t> elems = extract_bytes(decoded_reply, offset);
        for (int i = 0; i < ele_size / 64; i ++) {
            for (int j = 0; j < 64; j ++) {
                debuglog << setfill('0') << setw(2) << hex
                 << (int)elems[i * (ele_size/64) + j]
                 << " ";
            }
            debuglog << endl;
        }
    }

    cnt = 0;
    for (auto coeffs: decoded_coeffs) {
        debuglog << "retaining elems from decoded coeffs " << ++cnt << "..." << endl;
        Plaintext pt;
        encoder_->encode(coeffs, pt);
        vector<uint8_t> elems = extract_bytes(pt, offset);
        for (int i = 0; i < ele_size / 64; i ++) {
            for (int j = 0; j < 64; j ++) {
                debuglog << setfill('0') << setw(2) << hex
                 << (int)elems[i * (ele_size/64) + j]
                 << " ";
            }
            debuglog << endl;
        }
    }
    debuglog << endl;
    debuglog.close();
#endif

#ifdef DEBUG
    debuglog.open("../src/debug/debug_log1.txt");
#endif

    for (uint64_t i = 0UL; i < coeff_per_ptxt; i ++) {
        uint64_t temp = 0UL;
        for (uint64_t j = 0UL; j < decoded_coeffs.size(); j ++) {
            temp = (temp + decoded_coeffs[j][i]) % mod;
            // temp = (temp + decoded_coeffs[j][i]);
#ifdef DEBUG
            if (i >= offset * coeff_per_ele && i < (offset+1) * coeff_per_ele) {
                debuglog << dec << "j = " << j << ", i = " << i << ". temp = ";
                debuglog << setfill('0') << setw(16) << hex << temp << endl;
            }
#endif
        }
        result_coeffs.push_back(temp);
    }

#ifdef DEBUG
    debuglog << endl;
    debuglog.close();
#endif

    encoder_->encode(result_coeffs, result);

    return extract_bytes(result, offset);
}

#define DEBUG_BATCH_XXX

vector<vector<uint8_t>> PIRClient::batch_deconfuse_and_decode_replies(vector<PirBatchReply> multi_party_batch_reply, uint32_t party_num, vector<Index> &elem_index_with_ptr) {
    vector<vector<uint8_t>> results;
    if (multi_party_batch_reply.size() != party_num)
        return results;
    
#ifdef DEBUG_BATCH_XXX
    cout << "Here" << endl;
#endif
    
    for (auto e : elem_index_with_ptr) {
        vector<PirReply> multi_party_reply;
        for (auto batch_reply : multi_party_batch_reply) {
            multi_party_reply.push_back(batch_reply[e.fv_info_ptr->reply_id]);
        }
#ifdef DEBUG_BATCH_XXX
        cout << "Here" << endl;
#endif
        vector<uint8_t> result = deconfuse_and_decode_replies(multi_party_reply, e.fv_info_ptr->fv_offset);
        results.push_back(result);
    }

    return results;
}
