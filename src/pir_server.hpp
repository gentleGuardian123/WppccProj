#pragma once

#include "pir.hpp"
#include "pir_client.hpp"
#include <map>
#include <memory>
#include <vector>

class PIRServer {
public:
  PIRServer(const seal::EncryptionParameters &enc_params,
            const PirParams &pir_params);

  // NOTE: server takes over ownership of db and frees it when it exits.
  // Caller cannot free db
  void set_database(std::unique_ptr<std::vector<seal::Plaintext>> &&db);
  void set_database(const std::unique_ptr<const std::uint8_t[]> &bytes,
                    std::uint64_t ele_num, std::uint64_t ele_size);
  void preprocess_database();

  std::vector<seal::Ciphertext> expand_query(const seal::Ciphertext &encrypted,
                                             std::uint32_t m,
                                             std::uint32_t client_id);

  PirQuery deserialize_query(std::stringstream &stream);
  PirReply generate_reply(PirQuery &query, std::uint32_t client_id);

  // Serializes the reply into the provided stream and returns the number of
  // bytes written
  int serialize_reply(PirReply &reply, std::stringstream &stream);

  void set_galois_key(std::uint32_t client_id, seal::GaloisKeys galkey);

  // Below simple operations are for interacting with the database WITHOUT PIR.
  // So they can be used to modify a particular element in the database or
  // to query a particular element (without privacy guarantees).
  void simple_set(std::uint64_t index, seal::Plaintext pt);
  seal::Ciphertext simple_query(std::uint64_t index);
  void set_one_ct(seal::Ciphertext one);

  PirReply generate_reply_with_add_confusion(PirQuery &query, std::uint32_t client_id, std::uint64_t rand_num);
  std::vector<PirReply> gen_batch_reply(std::vector<PirQuery> &batch_pir_query, std::uint32_t client_id);
  void refresh_and_set_rand_vec(size_t batch_query_size) ;
  void gen_rand_trio(std::uint64_t &dest_rand1, std::uint64_t &dest_rand2, std::uint64_t &dest_rand3);
  void set_rand_vec_to_use(std::vector<std::uint64_t> &rand_vec_to_use);
  void output_rand_vec_to_send(std::vector<std::uint64_t> &rand_vec_to_send1, std::vector<std::uint64_t> &rand_vec_to_send2);
  seal::Plaintext gen_rand_pt(std::uint64_t rand_num);
  
private:
  seal::EncryptionParameters enc_params_; // SEAL parameters
  PirParams pir_params_;                  // PIR parameters
  std::unique_ptr<Database> db_;
  bool is_db_preprocessed_;
  std::map<int, seal::GaloisKeys> galoisKeys_;
  std::unique_ptr<seal::Evaluator> evaluator_;
  std::unique_ptr<seal::BatchEncoder> encoder_;
  std::shared_ptr<seal::SEALContext> context_;

  std::vector<std::uint64_t> rand_vec_to_send1_;
  std::vector<std::uint64_t> rand_vec_to_send2_;
  std::vector<std::uint64_t> rand_vec_to_use_;
  bool is_refreshed_;

  // This is only used for simple_query
  seal::Ciphertext one_;

  void multiply_power_of_X(const seal::Ciphertext &encrypted,
                           seal::Ciphertext &destination, std::uint32_t index);
};
