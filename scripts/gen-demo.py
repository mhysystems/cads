#!/usr/bin/env python3

import argparse
import sqlite3
from datetime import datetime
from pathlib import Path
import math
import shutil
import numpy as np

def shift(arr, num, fill_value=0):
    if num == 0:
        return arr;
    elif num > 0:
        return np.concatenate((np.full(num, fill_value), arr[:-num]))
    else:
        return np.concatenate((arr[-num:], np.full(-num, fill_value)))  

def filename_components(db:str):
  fcomps = db.split('-')
  site =  fcomps[0]
  conveyor = fcomps[1]
  date = '-'.join(fcomps[2:])
  db_date = datetime.strptime(date,"%Y-%m-%d-%H%M%S")
  return (site,conveyor,date,db_date.strftime("%Y-%m-%d %H:%M:%S"))


def process_profile(db: str, localdb:str, n:int) :

    shutil.copy(db,localdb)

    conn = sqlite3.connect(localdb)
    cur = conn.cursor()
    cur.execute(f"DELETE from PROFILE where rowid > ?",[n])
    conn.commit()
    cur.execute("VACUUM")
    conn.close()

def process_profile2(db: str, localdb:str, y:float) :

    shutil.copy(db,localdb)

    conn = sqlite3.connect(localdb)
    cur = conn.cursor()
    cur.execute(f"select max(y) from PROFILE")
    ymax = cur.fetchone()[0]
    cur.execute(f"DELETE from PROFILE where y > ? and y < ?",[y/2,ymax - (y/2)])
    conn.commit()
    cur.execute("VACUUM")
    cur.execute(f"update PROFILE set rowid = rowid - 1 ")
    cur.execute(f"select max(rowid) from PROFILE")
    n = cur.fetchone()[0] + 1
    cur.execute(f"update PROFILE set y = rowid * ? ",[y/n])
    conn.commit()
    conn.close()

def retrieve_appdb(db: str, site: str, conveyor: str, chrono: str, y_max: float) :
  conn = sqlite3.connect(db)
  cur = conn.cursor()
  cur.execute(f"SELECT * from beltinfo where site = ? and conveyor = ? and chrono = ?",[site,conveyor,chrono])
  row = list(cur.fetchone())
  conn.close()
  row[1] = 'site1'
  row[2] = 'belt1'
  row[10] = y_max
  row[11] = math.floor(y_max / row[5])
  return (row[11],row)

def insert_localdb(db: str, row) :
  conn = sqlite3.connect(db)
  cur = conn.cursor()
  cur.execute(f"INSERT into beltinfo VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?)",row)
  conn.commit()
  conn.close()

def first_row(db: str) :
  conn = sqlite3.connect(db)
  cur = conn.cursor()
  cur.execute(f"SELECT z from profile limit 1")
  row = list(cur.fetchone())
  return np.frombuffer(row[0],dtype='f')

def last_row(db: str) :
  conn = sqlite3.connect(db)
  cur = conn.cursor()
  cur.execute(f"SELECT rowid,z from profile order by rowid desc limit 1")
  row = list(cur.fetchone())
  return (row[0],np.frombuffer(row[1],dtype='f'))


def shift_profile(db: str) :
    fr = first_row(db)
    n,sr = last_row(db)
    width = len(sr)
    cp = np.argmax(np.correlate(fr,sr,"full"))
    dr = (width - cp - 1) / n

    conn = sqlite3.connect(db)
    cur = conn.cursor()
    cur2 = conn.cursor()
    i = 0
    for row in cur.execute(f"SELECT rowid,z from PROFILE"):
        
      z = shift(np.frombuffer(row[1],dtype='f'),math.floor(dr*i))
      cur2.execute("update PROFILE set z=? where rowid=?", [z,row[0]])
      i = i + 1
    
    conn.commit()
    conn.close()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Conversion of belt to demo belt')
    parser.add_argument("db", help="DB's to query", type=str, nargs='+')
    parser.add_argument("--appdb", type=str, help="webapp db", default="conveyors.db")
    parser.add_argument("--beltlength", type=float, help="new prefered beltlength", default=300000.0)

    args = parser.parse_args()

    for db_file in args.db:
      site,conveyor,date,db_date = filename_components(Path(db_file).name)
      n,row = retrieve_appdb(Path(db_file).parent / Path(args.appdb).name,site,conveyor,db_date,args.beltlength)
      localprofile = f"site1-belt1-{date}"
      process_profile2(db_file,Path(args.appdb).parent / localprofile,args.beltlength)
      insert_localdb(args.appdb,row)

      #shift_profile(localprofile)
