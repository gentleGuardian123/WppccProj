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
  //生成随机数
  void refresh_and_set_add_rand_vec(std::vector<PirQuery> &query) ;
  void gen_add_rand(std::uint64_t& dest_rand1, std::uint64_t& dest_rand2,int i);
  std::vector<std::uint64_t>& getAddRandVec1();
  std::vector<std::uint64_t>& getAddRandVec2();
  std::vector<std::uint64_t>& getAddRandVec3();
  std::vector<std::uint64_t>& getAddRandVec4();


private:
  seal::EncryptionParameters enc_params_; // SEAL parameters
  PirParams pir_params_;                  // PIR parameters
  std::unique_ptr<Database> db_;
  bool is_db_preprocessed_;
  std::map<int, seal::GaloisKeys> galoisKeys_;
  std::unique_ptr<seal::Evaluator> evaluator_;
  std::unique_ptr<seal::BatchEncoder> encoder_;
  std::shared_ptr<seal::SEALContext> context_;
  std::vector<std::uint64_t> add_rand_vec1;  // 成员变量，用于存储生成的随机数1 r1
  std::vector<std::uint64_t> add_rand_vec2;  // 成员变量，用于存储生成的随机数2 r2
  std::vector<std::uint64_t> add_rand_vec3;  // 成员变量，用于存储生成的随机数-1-2 r3
  std::vector<std::uint64_t> add_rand_vec4;  // 成员变量，用于存储生成的随机数(12)^-1 r4

  // This is only used for simple_query
  seal::Ciphertext one_;

  void multiply_power_of_X(const seal::Ciphertext &encrypted,
                           seal::Ciphertext &destination, std::uint32_t index);
};
