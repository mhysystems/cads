import sqlite3
import numpy as np
import argparse
from PIL import Image

def process_belt(db) :
    
    conn = sqlite3.connect(db)
    conn_o = sqlite3.connect(db + "out")
    cur_o = conn_o.cursor()
    cur_o.execute("drop table if exists PROFILE")
    cur_o.execute("create table PROFILE (y INTEGER PRIMARY KEY, x_off REAL NOT NULL, z BLOB NOT NULL)")
    query = "INSERT INTO PROFILE (y,x_off,z) VALUES (?,?,?)"
    cur = conn.cursor()

    for row in cur.execute(f"SELECT * from PROFILE order by y asc"):
        z = np.frombuffer(row[2],dtype='i2').astype(np.float32)
        cur_o.execute(query,(row[0],row[1], (z + 2391) * 0.0107 ))    

    
    conn_o.commit()
    cur_o.close()
    cur.close()
    conn_o.close()
    conn.close()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Extract profile data from sqlitedb')
    parser.add_argument("db", help="Belt data with columns")

    args = parser.parse_args()
    process_belt(args.db)
