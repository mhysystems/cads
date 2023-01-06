import sqlite3
import numpy as np
import argparse
from datetime import datetime

#datetime.now().strftime("%Y-%m-%d-%H%M%S")

def insertAppDb(db:str,row):
  conn = sqlite3.connect(db)
  cur = conn.cursor()
  cur.execute(f"INSERT into beltinfo VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?)",row)
  conn.commit()
  conn.close()

def gen_row(i: int, width: int) :
  return np.array([30 for x in range(0,width)],dtype='f')


def process_profile_out(db: str, width: int, length: int) :
    
  conn = sqlite3.connect(db)
  cur = conn.cursor()
  cur.execute("drop table if exists PROFILE")
  cur.execute("create table PROFILE (y REAL NOT NULL, x_off REAL NOT NULL, z BLOB NOT NULL)")

  query = "INSERT INTO PROFILE (rowid,y,x_off,z) VALUES (?,?,?,?)"
  dy = 0.5
  x_off = -1
  for i in range(0,length):
    cur.execute(query,(i,i*dy,x_off,gen_row(i,width)))    

  conn.commit()
  conn.close()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Plot each profile')
    parser.add_argument("--odb", help="Output DB name", default="test.db")
    parser.add_argument("--appdb", help="Input WebApp DB", type=str)
    parser.add_argument("--width", help="Number of columns", default=12, type=int)
    parser.add_argument("--length", help="Number of rows", default=12, type=int)
    
    args = parser.parse_args()
    process_profile_out(args.odb, args.width, args.length)
    if args.appdb :
      row = ["whaleback","cv405","2022-12-17 00:00:00",1.0,1.0,1.0,1.0,0,35,0,args.length,args.length,args.width]
      insertAppDb(args.appdb,row)

