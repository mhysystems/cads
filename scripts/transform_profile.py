import sqlite3
import numpy as np
import argparse
from pydoc import importfile
from functools import reduce

def process_profile(db: str, rowid = 0, ztype: str ='f') :
    conn = sqlite3.connect(db)
    cur = conn.cursor()
    for row in cur.execute(f"SELECT y,x_off,z,rowid from PROFILE where rowid >= ? ",[rowid]):
        a = yield (row[0],row[1],np.frombuffer(row[2],dtype=ztype),row[3])
        if a:
            break;
        
    conn.close()

def process_profile_out(db: str, trans, ztype: str ='f') :
    conn = sqlite3.connect(db)
    cur = conn.cursor()
    cur.execute("create table PROFILE (y REAL , x_off REAL NOT NULL, z BLOB NOT NULL)")
    
    query = "INSERT INTO PROFILE (rowid,y,x_off,z) VALUES (?,?,?,?)"

    while(True):
        (q,row) = (yield)
        
        if q:
            break
        
        (y,x_off,z,rowid) = trans(row) 
        cur.execute(query,(rowid,y,x_off,z))    
    
    conn.commit()
    conn.close()

def id(x):
    return x

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Input a scan, perform a transform on each row and write to new scan. Tranform is hard coded in file')
    parser.add_argument("db", type=str,help="Input DB name")
    parser.add_argument("-t","--trans", type=str, action='append',help="File containing function transform_profile")
    parser.add_argument("--odb", type=str,help="Output DB name", default="result.db")
    parser.add_argument("-r","--row", type=int ,help="First row id", default=0)

    args = parser.parse_args()
    
    trans = [id] if not args.trans else [importfile(t).transform_profile for t in args.trans]

    t = reduce(lambda acc,v : lambda x: v(acc(x)), trans)
    
    profile_out = process_profile_out(args.odb,t)
    next(profile_out)
    for row in process_profile(args.db,args.row):
        profile_out.send((False,row))
    
    profile_out.send((True,()))
