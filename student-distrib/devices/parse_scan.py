#! /usr/bin/python3
import csv 
import pprint
import re 
import os 


scan_codes = {} 

pp = pprint.PrettyPrinter()

exp = re.compile('F[0-9]+')

with open('keyboard.csv', newline='') as csvfile : 
    reader = csv.reader(csvfile)
    for row in reader : 
        if len(row) == 2 and ('(keypad)' not in row[1]) and ('pressed' in row[1]) :
            if exp.search(row[1]) == None : 
                temp_str = row[1].split(' ')
                scan_codes[int(row[0],base=16)] = temp_str[0]


# new_file = open('new.csv', mode='w')

# for key in scan_codes :
#     new_file.write(', '.join([str(key),scan_codes[key]]))
#     new_file.write('\n')

c_array = open('c_array.txt','w')
c = 0 
for i in range(256) :
    if i in scan_codes : 
        c_array.write('\'{0}\', ' .format(scan_codes[i].lower()))
    else : 
        c_array.write('0, ')
    c+=1 
    if c % 10 == 0 : 
        c_array.write('\n')

c_array.close()








