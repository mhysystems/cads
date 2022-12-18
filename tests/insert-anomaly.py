import sqlite3
import numpy as np
import argparse
import math

def process_profile(db: str, x: int, y:int, width : int, length :int, z : float) :
    conn = sqlite3.connect(db)
    cur = conn.cursor()
    cur2 = conn.cursor()
    for row in cur.execute(f"SELECT rowid,z from PROFILE where rowid >= ? limit ?",[y,length]):
        
        zdb = np.frombuffer(row[1],dtype='f').copy()
        zdb[x:x+width] = z
        cur2.execute("update PROFILE set z=? where rowid=?", (zdb,row[0]))

    conn.commit()
    conn.close()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Plot each profile')
    parser.add_argument("db", help="DB's to query", type=str, nargs='+')
    parser.add_argument("--x", help="z index", default=2, type=int)
    parser.add_argument("--y", help="rowid", default=2, type=int)
    parser.add_argument("--width", help="Number of columns", default=2, type=int)
    parser.add_argument("--length", help="Number of rows", default=2, type=int)
    parser.add_argument("--z", help="Number of rows", default=25.0, type=float)
    
    args = parser.parse_args()
    for db_file in args.db:
      print(db_file)
      process_profile(db_file, args.x, args.y,args.width, args.length, args.z)
