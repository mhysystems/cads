#!/usr/bin/env python3

import argparse
import sqlite3
from datetime import datetime
from pathlib import Path
import math
import shutil
import numpy as np

def create_scan_db(db: str) :
    conn = sqlite3.connect(db)
    cur = conn.cursor()
    cur.execute(f"CREATE TABLE IF NOT EXISTS PROFILE (y REAL NOT NULL, x_off REAL NOT NULL, z BLOB NOT NULL)")
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
        cur.execute(f"INSERT INTO PROFILE (rowid,y,x_off,z) VALUES (?,?,?,?)",(i,i * step, x_off,z))
        if not i % 100 : 
            conn.commit()
    
    conn.commit()
    conn.close()


def gen_row(width : int, n : int) :
  row = [0] * 12
  row[0] = 'localdev' # site
  row[1] = 'test' # conveyor
  row[2] = '2023-03-08 12:00:00' # chono
  row[3] = 1.0     # x_res
  row[4] = 6.0     # y_res
  row[5] = 0.01    # z_res
  row[6] = 0.0     # z_off
  row[7] = 31.0    # z_max
  row[8] = 0.0     # z_min
  row[9] = n * 6.0 
  row[10] = n
  row[11] = width
  return row

def insert_scan(appdb: str, row) :
  conn = sqlite3.connect(appdb)
  cur = conn.cursor()
  last_row = cur.execute(f"select count(rowid) from belt").fetchone()[0]
  cur.execute(f"INSERT into belt VALUES({last_row+1},?,?,?,?,?,?,?,?,?,?,?,?)",row)
  conn.commit()
  conn.close()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Generate Gui test belt and insert')
    parser.add_argument("--appdb", type=str, help="webapp db", default="conveyors.db")
    parser.add_argument("--rows", type=int, help="rows", default=2000)
    parser.add_argument("--cols", type=int, help="cols", default=1800)
    
    filename = 'localdev-test-2023-03-08-12000'
    args = parser.parse_args()

    scan_db = Path(args.appdb).parent / filename

    create_scan_db(scan_db)
    fill_scan_db(scan_db,args.cols,args.rows)
    r = gen_row(args.cols,args.rows)
    insert_scan(args.appdb,r)
