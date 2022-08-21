#!/usr/bin/env python3

import numpy as np
import argparse
import sqlite3
import sys

def process_profile(db: str) :
    conn = sqlite3.connect(db)
    cur = conn.cursor()
    for row in cur.execute(f"SELECT y,y,z from PROFILE order by y asc"):
        a = yield [row[0],row[1],np.frombuffer(row[2],dtype='f')]
        if a:
            break;
        
    conn.close()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Sqlite DB belt Profile extraction')
    parser.add_argument("db", help="DB's to query", type=str, nargs='+')
    parser.add_argument("--z", type=float, help="Search above z", default=33)

    args = parser.parse_args()
    for db_file in args.db:
        for p in process_profile(db_file):
            z = p[2]
            if z[z > args.z].size > 0:
                print(p[0],p[1])
