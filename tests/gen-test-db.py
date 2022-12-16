import sqlite3
import numpy as np
import argparse

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
    parser.add_argument("--width", help="Number of columns", default=12, type=int)
    parser.add_argument("--length", help="Number of rows", default=12, type=int)
    
    args = parser.parse_args()
    profile_out = process_profile_out(args.odb, args.width, args.length)
