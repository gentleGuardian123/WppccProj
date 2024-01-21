import csv

dic = dict()

with open('dataset_B.csv', newline='') as csvfile: 
    reader = csv.DictReader(csvfile)
    for row in reader:
        index = int(row['用户id'][:8], 16)
        if (dic.get(row['用户id'][:8]) == None):
            dic[row['用户id'][:8]] = 1
            # print("Add: " + row['用户id'][:8])
        else: 
            print("Repeat: " + row['用户id'][:8])
