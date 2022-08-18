#!/usr/bin/env python3

import numpy as np
import argparse
import sqlite3
import sys

def process_profile(db: str, y = 0) :
    conn = sqlite3.connect(db)
    cur = conn.cursor()
    for row in cur.execute(f"SELECT * from PROFILE where y >= ? order by y asc limit 1",[y]):
        a = yield [row[0],row[1],np.frombuffer(row[2],dtype='f')]
        if a:
            break;
        
    conn.close()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Sqlite DB belt Profile extraction')
    parser.add_argument("db", help="DB's to query", type=str, nargs='+')
    parser.add_argument("--y", help="Y start", default=0)

    args = parser.parse_args()
    for db_file in args.db:
        for p in process_profile(db_file,args.y):
            print("%f,%f,%s" % (p[0],p[1],','.join(map(str,p[2]))))
