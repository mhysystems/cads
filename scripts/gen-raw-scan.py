#!/usr/bin/env python3

import argparse
import sqlite3
from datetime import datetime
from pathlib import Path
import math
import shutil
import numpy as np
import padprofile

def create_scan_db(db: str) :
    conn = sqlite3.connect(db)
    cur = conn.cursor()
    cur.execute(f"CREATE TABLE IF NOT EXISTS PROFILE (revid INTEGER NOT NULL, idx INTEGER NOT NULL, y REAL NOT NULL, x_off REAL NOT NULL, z BLOB NOT NULL)")
    cur.execute(f"CREATE TABLE IF NOT EXISTS PARAMETERS (x_res REAL NOT NULL, z_res REAL NOT NULL, z_off REAL NOT NULL)")
    conn.commit()
    conn.close()

def fill_scan_db(db: str, width: int, n: int) :
    
    conn = sqlite3.connect(db)
    cur = conn.cursor()
    z = np.empty(width, dtype = 'f')    
    z.fill(30.0)
    step = 6.0
    x_off = -width / 2
    
    for i in range(0,n):
        cur.execute(f"INSERT INTO PROFILE (revid,idx,y,x_off,z) VALUES (0,?,?,?,?)",(i,i * step, x_off,z))
        if not i % 100 : 
            conn.commit()
    
    conn.commit()
    conn.close()

def insert_parameters(db:str) :
  row = [0] * 3
  row[0] = 1.0     # x_res
  row[1] = 0.01    # z_res
  row[2] = 0.0     # z_off
  conn = sqlite3.connect(db)
  cur = conn.cursor()
  cur.execute(f"INSERT into PARAMETERS(x_res,z_res,z_off) VALUES(?,?,?)",row)
  conn.commit()
  conn.close()

def insert_splice(db: str, offset : float, i0 : int, i1 : int) :
    conn = sqlite3.connect(db)
    cur = conn.cursor()
    cur2 = conn.cursor()
    for row in cur.execute(f"SELECT idx,z from PROFILE where ? <= idx and idx < ?",(i0,i1)):
        z_readonly = np.frombuffer(row[1],dtype='f')
        z = z_readonly + offset
        cur2.execute("update PROFILE set z=? where idx=?", (z,row[0]))
    
    conn.commit()
    conn.close()



if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Generate raw scan')
    parser.add_argument("--db", type=str, help="webapp db", default="rawprofile.db")
    parser.add_argument("--rows", type=int, help="rows", default=2000)
    parser.add_argument("--cols", type=int, help="cols", default=2000)
    parser.add_argument("--splice", type=int, help="splice length", default=20)
    parser.add_argument("--pad", type=int, help="cols", default=100)
    
    args = parser.parse_args()
    
    row_off = 10000
    rows = args.rows + row_off # iirfilter burn in
    splice_pos = math.floor(args.rows * 0.1)

    create_scan_db(args.db)
    fill_scan_db(args.db,args.cols-(2*args.pad),rows)
    insert_parameters(args.db)
    insert_splice(args.db,-5,row_off+splice_pos,row_off + splice_pos+args.splice)
    insert_splice(args.db,-5,rows - (splice_pos+args.splice),rows - splice_pos)
    padprofile.process_profile(args.db,args.cols,0)
