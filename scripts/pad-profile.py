#!/usr/bin/env python3

import numpy as np
import argparse
import sqlite3
import sys

def process_profile(db: str, width : int, pad : float) :
    conn = sqlite3.connect(db)
    cur = conn.cursor()
    cur2 = conn.cursor()
    tmp = np.empty((1, int(width / np.dtype(np.float32).itemsize)),dtype='f')
    for row in cur.execute(f"SELECT rowid,z from PROFILE where length(z) != ?",[width]):
        tmp.fill(pad)
        z = np.frombuffer(row[1],dtype='f')
        if tmp.size >= z.size :
            tmp[0,:z.size] = z
        else :
            tmp = z[0,:tmp.size]

        cur2.execute("update PROFILE set z=? where rowid=?", (tmp,row[0]))

    conn.commit()
    conn.close()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Pad profile to match width of other belt profiles')
    parser.add_argument("db", help="DB's to query", type=str, nargs='+')
    parser.add_argument("--width","-w", type=int, help="Pad profile to this width", required=True) 
    parser.add_argument("--pad","-p", type=float, help="Pad profile to this value", default=0) 

    args = parser.parse_args()
    process_profile(args.db[0],args.width,args.pad)
