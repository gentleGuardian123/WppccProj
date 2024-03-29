# Parameters Explanation

## `batch_query_test.py`:

- `dim_of_items`: 数据库总数据条目数的维数。

- `size_per_item`：数据库单条数据的大小（/Byte）。

- `batch_num`：待批量查询的数据条目索引总数。

- `constant_index_rate`：待批量查询索引中连续索引的比例。

- `slot_count`：编码多项式的度/多项式系数的总数/可插入的多项式空槽数。

- `pre_process_time(/ms)`：**服务端**预处理数据库时间。

- `query_gen_time(/ms)`：**客户端**生成**批量**查询的时间。

- `query_seri_time(/ms)`：**客户端**序列化查询的时间。

- `query_deseri_time(/ms)`：**服务端**反序列化查询的时间。

- `reply_gen_time(/ms)`：**服务端**生成**批量**回复的时间。

- `answer_dec_time(/ms)`：**客户端**解码**批量**回复的时间。

- `query_size(/kb)`：**客户端**生成的**批量**问询总大小。

- `compressed_query_num`：**客户端**批量化待查询索引后生成的批量问询总数。

- `reply_size(/kb)`：**服务端**生成的**批量**回复总大小。

## `multi_party_comput_test.py`:

- `query_gen_time(/ms)`：**客户端**生成查询的时间。

- `reply_gen_time(/ms)`：**服务端**生成回复的时间。

- `answer_dec_time(/ms)`：**客户端**解码回复的时间。

- `query_size(/kb)`：**客户端**生成的问询总大小。

- `reply_ciphertexts_num`：**服务端**回复中包含的密文多项式总数。

- `reply_size(/kb)`：**服务端**生成的回复总大小。

其余同`batch_query_test.py`。
