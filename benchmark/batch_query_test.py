import subprocess
import csv


elf_path = "../bin/batch_query_test"
para_csv = "batch_test_para.csv"
res_csv = "batch_test_res.csv"
combined_csv = "batch_test.csv"
para_head = ["dim_of_items_number", "size_per_item", "batch_num", "constant_index_rate", "slot_count"]
res_head = ["pre_process_time(/ms)", "query_gen_time(/ms)", "query_seri_time(/ms)", "query_deseri_time(/us)", "reply_gen_time(/ms)", "answer_dec_time(/ms)", "query_size(/kb)", "compressed_query_num", "reply_size(/kb)"]


def init_test():
    with open(para_csv, "w", newline="") as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(para_head)


def add_test(dim_of_items_number: int, size_per_item: int, batch_num: int, constant_index_rate: int,  slot_count: int):
    with open(para_csv, "a", newline="") as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow([dim_of_items_number, size_per_item, batch_num, constant_index_rate, slot_count])

def run_test():
    res = []
    with open(para_csv, "r", newline="") as csvfile:
        reader = csv.reader(csvfile)
        next(reader, None)
        for row in reader:
            out = subprocess.run([elf_path] + row, capture_output=True, text=True)
            res.append(out.stdout.split('\n')[-1].split(','))
    with open(res_csv, "w", newline="") as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(res_head)
        writer.writerows(res)


def combine():
    with open(para_csv, "r", newline="") as para, open(res_csv, "r", newline="") as res, open(combined_csv, "w", newline="") as combined:
        para_reader = csv.reader(para)
        res_reader = csv.reader(res)
        combined_writer = csv.writer(combined)
        combined_writer.writerows([t[0] + t[1] for t in zip(para_reader, res_reader)])
        

if __name__ == "__main__":
    init_test()

    # vary in `dim_of_items_number`.
    add_test(10, 512, 500, 80, 4096)
    add_test(12, 512, 500, 80, 4096)
    add_test(14, 512, 500, 80, 4096)
    add_test(16, 512, 500, 80, 4096)
    add_test(18, 512, 500, 80, 4096)

    # vary in `size_per_item`.
    add_test(14, 128, 1000, 70, 4096)
    add_test(14, 256, 1000, 70, 4096)
    add_test(14, 512, 1000, 70, 4096)
    add_test(14, 1024, 1000, 70, 4096)
    add_test(14, 2048, 1000, 70, 4096)

    # vary in `batch_num`.
    add_test(13, 1024, 2000, 75, 4096)
    add_test(13, 1024, 4000, 75, 4096)
    add_test(13, 1024, 6000, 75, 4096)
    add_test(13, 1024, 8000, 75, 4096)
    add_test(13, 1024, 10000, 75, 4096)

    # vary in `constant_index_rate`.
    add_test(16, 512, 1000, 25, 4096)
    add_test(16, 512, 1000, 45, 4096)
    add_test(16, 512, 1000, 65, 4096)
    add_test(16, 512, 1000, 85, 4096)
    add_test(16, 512, 1000, 100, 4096)

    run_test()
    combine()
