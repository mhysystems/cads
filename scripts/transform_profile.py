import sqlite3
import numpy as np
import argparse

def process_profile(db: str, y = 0, ztype: str ='f') :
    conn = sqlite3.connect(db)
    cur = conn.cursor()
    for row in cur.execute(f"SELECT * from PROFILE where y >= ? order by y asc",[y]):
        a = yield [row[0],row[1],np.frombuffer(row[2],dtype=ztype)]
        if a:
            break;
        
    conn.close()

def process_profile_out(db: str, y = 0, ztype: str ='f') :
    conn = sqlite3.connect(db)
    cur = conn.cursor()
    cur.execute("drop table if exists PROFILE")
    cur.execute("create table PROFILE (y REAL PRIMARY KEY, x_off REAL NOT NULL, z BLOB NOT NULL)")
    
    query = "INSERT INTO PROFILE (y,x_off,z) VALUES (?,?,?)"

    while(True):
        (q,row) = (yield)
        
        if q:
            break

        cur.execute(query,(row[0],row[1], (row[2] - 1.0)* 0.0107 + 21.0897 ))    

    conn.commit()
    conn.close()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Plot each profile')
    parser.add_argument("db", help="Belt data with columns")
    parser.add_argument("--y", help="Y start", default=0)
    parser.add_argument("--odb", help="Output DB name", default="result.db")
    

    args = parser.parse_args()
    profile_out = process_profile_out(args.odb)
    next(profile_out)
    for row in process_profile(args.db,args.y):
        profile_out.send((False,row))
    
    profile_out.send((True,[]))
