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

        stringstream hex_indice(row[0].substr(0, 8));
        uint64_t indice; 
        hex_indice >> hex >> indice;

        // cout << hex_indice.str() << endl;
        // cout << indice << endl;

        db.get()[indice] = ( stoi(row[1], nullptr, 10) << 1 ) + stoi(row[2], nullptr, 2);

        // if (! stoi(row[2], nullptr, 2) ) {
        //     cout << "Number of phone account:\t" << row[1] << endl;
        //     cout << "Whether in blacklist:\t\t" << row[2] << endl;
        //     cout << "Combined data:\t\t\t" << uint32_t(db.get()[indice]) << endl;
        // }

        if (++count == number_of_items)
            return ;
    }

    fin.close();
}
