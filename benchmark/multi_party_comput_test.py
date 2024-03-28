import subprocess
import csv


elf_path = "../bin/multi_party_comput_test"
para_csv = "mpc_test_para.csv"
res_csv = "mpc_test_res.csv"
combined_csv = "mpc_test.csv"
para_head = ["dim_of_items_number", "size_per_item", "slot_count"]
res_head = ["pre_process_time(/ms)", "query_gen_time(/us)", "query_seri_time(/us)", "query_deseri_time(/us)", "reply_gen_time(/ms)", "answer_dec_time(/us)", "query_size(/kb)", "reply_ciphertexts_num", "reply_size(/kb)"]


def init_test():
    with open(para_csv, "w", newline="") as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(para_head)


def add_test(dim_of_items_number: int, size_per_item: int, slot_count: int):
    with open(para_csv, "a", newline="") as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow([dim_of_items_number, size_per_item, slot_count])

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

    add_test(10, 512, 4096)
    add_test(11, 512, 4096)
    add_test(12, 512, 4096)
    add_test(13, 512, 4096)
    add_test(14, 512, 4096)
    add_test(10, 1024, 4096)
    add_test(11, 1024, 4096)
    add_test(12, 1024, 4096)
    add_test(13, 1024, 4096)
    add_test(14, 1024, 4096)
    add_test(15, 512, 8192)
    add_test(16, 512, 8192)
    add_test(15, 1024, 8192)
    add_test(16, 1024, 8192)
    run_test()
    combine()
