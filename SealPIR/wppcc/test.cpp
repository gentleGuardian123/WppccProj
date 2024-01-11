std::vector<std::uint64_t> add_rand_vec1()  //发送给S2
std::vector<std::uint64_t> add_rand_vec2()  //发送给S3

void gen_add_rand(std::uint64_t& dest_rand1, std::uint64_t& dest_rand2) {
  // 设置随机数生成器，使用随机设备作为种子
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<std::uint64_t> dis(0, std::numeric_limits<std::uint64_t>::max());

  // 生成两个随机数，并分别赋值给 dest_rand1 和 dest_rand2
  dest_rand1 = dis(gen);
  dest_rand2 = dis(gen);
}

void refresh_and_set_add_rand_vec(std::vector<std::vector<seal::Ciphertext>> &query)
{  
    int m = query.size();
    std::uint64_t dest_rand1;
    std::uint64_t dest_rand2;
    // 初始化并为 add_rand_vec1 赋予 m 个随机数
    add_rand_vec1.clear();
    add_rand_vec1.reserve(m);
    add_rand_vec2.clear();
    add_rand_vec2.reserve(m);
    for (int i = 0; i < m; i++) {
      gen_add_rand(dest_rand1,dest_rand2)
      add_rand_vec1.push_back(dest_rand1());
      add_rand_vec2.push_back(dest_rand2());
    }
}

