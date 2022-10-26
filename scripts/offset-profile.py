#!/usr/bin/env python3

import numpy as np
import argparse
import sqlite3
import sys

def process_profile(db: str, offset : float) :
    conn = sqlite3.connect(db)
    cur = conn.cursor()
    cur2 = conn.cursor()
    for row in cur.execute(f"SELECT rowid,z from PROFILE"):
        z_readonly = np.frombuffer(row[1],dtype='f')
        z = z_readonly + offset
        cur2.execute("update PROFILE set z=? where rowid=?", (z,row[0]))

    conn.commit()
    conn.close()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Add offset to all z values of belt')
    parser.add_argument("db", help="DB's to query", type=str, nargs='+')
    parser.add_argument("--offset","-o", type=float, help="Pad profile to this value", default=0) 

    args = parser.parse_args()
    for db_file in args.db:
      process_profile(db_file,args.offset)
