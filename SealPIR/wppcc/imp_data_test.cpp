#include "imp_data.hpp"
#include <iostream>

using namespace std;

int main()
{
    uint8_t dim_of_items_number = 8;
    uint64_t number_of_items = 1UL << dim_of_items_number;
    uint64_t size_per_item = 1; // byte

    auto db(make_unique<uint8_t[]>(number_of_items * size_per_item));
    const string csv_name = "../../data/_check_test.csv";

    import_and_parse_data(db, csv_name, dim_of_items_number);
}
