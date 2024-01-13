#include "imp_data.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;

void import_and_parse_data(std::unique_ptr<uint8_t[]> &db, const string csv_name, const uint8_t dim_of_items_number)
{
    fstream fin;
    fin.open(csv_name, ios::in);

    vector<string> row;
    string label, line, word, tmp;
    uint64_t count = 0;
    uint64_t number_of_items = 1UL << dim_of_items_number;

    // Parse labels first;
    fin >> label;

    while (fin >> line) {
        stringstream ss(line);

        row.clear();
        while (getline( ss, tmp, ',' )) {
            row.push_back(tmp);
        }

        // get the high 31-bit of userid as index in the database.
        stringstream hex_indice(row[0].substr(0, 8));
        uint64_t indice; 
        hex_indice >> hex >> indice;
        indice = indice & 0xfffffffe;

        // assume the high 31-bit is x, then 
        // `x || 0`(32-bit) indicates the phone number sum of `x` in database, 
        // `x || 1`(32-bit) indicates whether or not `x` is in the blacklist.
        uint8_t sum = stoi(row[1], nullptr, 10);
        db.get()[indice] = sum;
        bool flag = stoi(row[2], nullptr, 2);
        if (flag) {
            uint8_t rand8_t = gen_rand(sum);
            db.get()[indice + 1UL] = rand8_t;
        } else {
            db.get()[indice + 1UL] = 1;
        }

        cout << hex_indice.str() << endl;
        cout << indice << endl;

        if (! stoi(row[2], nullptr, 2) ) {
            cout << "Number of phone account:\t" << row[1] << endl;
            cout << "Whether in blacklist:\t\t" << row[2] << endl;
            cout << "Combined data:\t\t\t" << uint32_t(db.get()[indice]) << endl;
        }

        if (++count == number_of_items)
            return ;
    }

    fin.close();
}

uint8_t gen_rand(uint8_t seed) {
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    std::mt19937 gen(static_cast<std::uint64_t>(timestamp) + seed);
    std::uniform_int_distribution<std::uint64_t> dis(1, std::numeric_limits<std::uint64_t>::max());

    return dis(gen) % 256;
}
