import sqlite3
import numpy as np
import argparse
from pydoc import importfile
from functools import reduce

def process_zs(db: str, rowid = 0, ztype: str ='f') :
    conn = sqlite3.connect(db)
    cur = conn.cursor()
    #for row in cur.execute(f"SELECT z,rowid from PROFILE where rowid >= ? ",[rowid]):
    #    a = yield (np.frombuffer(row[0],dtype=ztype),row[1])
    for row in cur.execute(f"SELECT y,x_off,z,rowid from PROFILE where rowid >= ? ",[rowid]):
        a = yield (row[0],row[1],np.frombuffer(row[2],dtype=ztype),row[3])
        if a:
            break;
        
    conn.close()

def process_profile_out(db: str, trans, ztype: str ='f') :
    conn = sqlite3.connect(db)
    cur = conn.cursor()
    cur.execute(f"CREATE TABLE IF NOT EXISTS PROFILE (revid INTEGER NOT NULL, idx INTEGER NOT NULL,y REAL NOT NULL, x_off REAL NOT NULL, z BLOB NOT NULL, PRIMARY KEY (revid,idx))")
    
    query = "INSERT INTO PROFILE (revid,idx,y,x_off,z) VALUES (0,?,?,?,?)"

    while(True):
        (q,row) = (yield)
        
        if q:
            break
        
        (y,x_off,z,rowid) = row 
        #(y,x_off,z,rowid) = trans(row)
        cur.execute(query,(rowid,y,x_off,z))    
    
    conn.commit()
    conn.close()

def id(x):
    return x

def insert_parameters(db:str) :
  row = [0] * 3
  row[0] = 0.793   # x_res
  row[1] = 0.017  # z_res
  row[2] =  -217.375   # z_off
  conn = sqlite3.connect(db)
  cur = conn.cursor()
  cur.execute(f"CREATE TABLE IF NOT EXISTS PARAMETERS (x_res REAL NOT NULL, z_res REAL NOT NULL, z_off REAL NOT NULL)")
  cur.execute(f"INSERT into PARAMETERS(x_res,z_res,z_off) VALUES(?,?,?)",row)
  conn.commit()
  conn.close()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Input a scan, perform a transform on each row and write to new scan. Tranform is hard coded in file')
    parser.add_argument("db", type=str,help="Input DB name")
    parser.add_argument("-t","--trans", type=str, action='append',help="File containing function transform_profile")
    parser.add_argument("--odb", type=str,help="Output DB name", default="result.db")
    parser.add_argument("-r","--row", type=int ,help="First row id", default=0)

    args = parser.parse_args()
    
    trans = [id] if not args.trans else [importfile(t).transform_profile for t in args.trans]

    t = reduce(lambda acc,v : lambda x: v(acc(x)), trans)
    insert_parameters(args.odb)
    profile_out = process_profile_out(args.odb,t)
    next(profile_out)
    for row in process_zs(args.db,args.row):
        profile_out.send((False,row))
    
    profile_out.send((True,()))
    