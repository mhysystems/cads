#!/usr/bin/env python3

import argparse
import sqlite3
from datetime import datetime
  
import sys

def sqlite3_date_compare(db:str, sql:str):
  date = '-'.join(db.split('-')[2:])
  db_date = datetime.strptime(date,"%Y-%m-%d-%H%M%S")
  sql_date = datetime.strptime(sql.split('.')[0],"%Y-%m-%d %H:%M:%S")
  return db_date == sql_date



def process_profile(db: str, nb:float, commit:bool) :
    conn = sqlite3.connect(db)
    cur = conn.cursor()
    cur.execute(f"SELECT count(y), max(y) from profile")
    n,s = cur.fetchone()
    scale = nb / s
    print(f"scaling belt profiles by {scale}")
    if commit:
      cur.execute(f"update profile set y = y * ?",[scale])
      conn.commit()
    conn.close()

    return nb / n

def process_appdb(db: str, file:str, y_res:float, y_max:float, commit:bool) :
  conn = sqlite3.connect(db)
  cur = conn.cursor()
  cur.execute(f"SELECT chrono,rowid from beltinfo")
  r = list(filter(lambda n: sqlite3_date_compare(file,n[0]), cur.fetchall()))
  if len(r) < 1 :
    return
  print(f"Updating rowid:{r[0][1]} with y_res = {y_res}, Ymax = {y_max}")
  if commit:
    cur.execute(f"update beltinfo set y_res = ?, Ymax = ? where rowid = ?",[y_res,y_max,r[0][1]])
    conn.commit()
  conn.close()
    
    


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Sqlite DB belt Profile extraction')
    parser.add_argument("db", help="DB's to query", type=str, nargs='+')
    parser.add_argument("--appdb", type=str, help="webapp db", default="conveyors.db")
    parser.add_argument("--commit", help="commit changes", action='store_true')
    parser.add_argument("--beltlength", type=float, help="new prefered beltlength", default=906000.0)

    args = parser.parse_args()
  
    for db_file in args.db:
      print(db_file)
      y_res = process_profile(db_file,args.beltlength, args.commit)
      process_appdb(args.appdb,db_file,y_res,args.beltlength, args.commit)