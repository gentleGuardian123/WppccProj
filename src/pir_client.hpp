#pragma once

#include "pir.hpp"
#include <memory>
#include <vector>
#include <list>

using namespace std;

class PIRClient {
public:
  PIRClient(const seal::EncryptionParameters &encparms,
            const PirParams &pirparams);
  std::vector<PirQuery> generate_batch_query(std::vector<std::uint64_t> &desired_index_vec, 
                                             std::vector<Index> &index_vec_with_fv,
                                             std::list<FvInfo> &fv_info_list_dest);

  PirQuery generate_query(std::uint64_t desiredIndex);
  // Serializes the query into the provided stream and returns number of bytes
  // written
  int generate_serialized_query(std::uint64_t desiredIndex,
                                std::stringstream &stream);
  seal::Plaintext decode_reply(PirReply &reply);

  std::vector<uint64_t> extract_coeffs(seal::Plaintext pt);
  std::vector<uint64_t> extract_coeffs(seal::Plaintext pt,
                                       std::uint64_t offset);
  std::vector<uint8_t> extract_bytes(seal::Plaintext pt, std::uint64_t offset);

  std::vector<std::vector<uint8_t>> debatch_reply(std::vector<PirReply> &batch_reply, 
                                                       std::vector<Index> &elem_index_with_ptr);

  std::vector<uint8_t> decode_reply(PirReply &reply, uint64_t offset);

  std::vector<uint8_t> deconfuse_and_decode_replies(std::vector<PirReply> &replies, std::uint64_t offset);

  std::vector<vector<uint8_t>> batch_deconfuse_and_decode_replies(std::vector<PirBatchReply> multi_party_batch_reply, std::uint32_t party_num, std::vector<Index> &elem_index_with_ptr);

  seal::Plaintext decrypt(seal::Ciphertext ct);

  seal::GaloisKeys generate_galois_keys();

  // Index and offset of an element in an FV plaintext
  uint64_t get_fv_index(uint64_t element_index);
  uint64_t get_fv_offset(uint64_t element_index);

  // Only used for simple_query
  seal::Ciphertext get_one();

  seal::Plaintext replace_element(seal::Plaintext pt,
                                  std::vector<std::uint64_t> new_element,
                                  std::uint64_t offset);

private:
  seal::EncryptionParameters enc_params_;
  PirParams pir_params_;

  std::unique_ptr<seal::Encryptor> encryptor_;
  std::unique_ptr<seal::Decryptor> decryptor_;
  std::unique_ptr<seal::Evaluator> evaluator_;
  std::unique_ptr<seal::KeyGenerator> keygen_;
  std::unique_ptr<seal::BatchEncoder> encoder_;
  std::shared_ptr<seal::SEALContext> context_;

  vector<uint64_t> indices_; // the indices for retrieval.
  vector<uint64_t> inverse_scales_;

  friend class PIRServer;
};
