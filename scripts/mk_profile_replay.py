#!/usr/bin/env python3

import numpy as np
import argparse
import sqlite3
import math

def offsetZ(db: str, offset : float) :
    conn = sqlite3.connect(db)
    cur = conn.cursor()
    cur2 = conn.cursor()
    for row in cur.execute(f"SELECT rowid,z from PROFILE"):
        z_readonly = np.frombuffer(row[1],dtype='f')
        z = z_readonly + offset
        cur2.execute("update PROFILE set z=? where rowid=?", (z,row[0]))

    conn.commit()
    conn.close()


def getWidth(db:str):
    conn = sqlite3.connect(db)
    cur = conn.cursor()
    r = cur.execute(f"SELECT length(z) from PROFILE limit 1").fetchone()[0]
    conn.close()
    return r

# Pad profile to immitate the barrel
def process_profile(db: str, width : int, pad : float) :
    conn = sqlite3.connect(db)
    cur = conn.cursor()
    cur2 = conn.cursor()
    tmp = np.empty((1, int(width / np.dtype(np.float32).itemsize)),dtype='f')
    for row in cur.execute(f"SELECT rowid,z from PROFILE where length(z) != ?",[width]):
        tmp.fill(pad)
        z = np.frombuffer(row[1],dtype='f')
        if tmp.size >= z.size :
          off = math.floor((tmp.size - z.size) / 2)
          tmp[0,off:z.size+off] = z
        else :
          tmp = z[0,:tmp.size]

        cur2.execute("update PROFILE set z=? where rowid=?", (tmp,row[0]))

    conn.commit()
    conn.close()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Pad profile to match width of other belt profiles')
    parser.add_argument("db", help="DB's to query", type=str, nargs='+')
    parser.add_argument("--width","-w", type=int, help="Extend width by this many samples", required=True) 
    parser.add_argument("--pad","-p", type=float, help="Pad profile to this value", default=0) 
    parser.add_argument("--offset","-o", type=float, help="Pad profile to this value", default=0) 
    
    args = parser.parse_args()
    width = getWidth(args.db[0])
    print(width)
    #process_profile(args.db[0],args.width*np.dtype(np.float32).itemsize + width,args.pad)
    offsetZ(args.db[0],args.offset)
