#!/usr/bin/env python3

import numpy as np
import argparse
import sqlite3
import sys

def process_profile(fromDb: str, toDb:str, y = 0, i = 0, limit = 1) :
    s = 0
    f = "WHERE y >= ? "
    if i != 0:
        f = "WHERE idx >= ? "
        s = i

    conn = sqlite3.connect(toDb)
    cur = conn.cursor()
    cur.execute(f"ATTACH DATABASE '{fromDb}' AS fromDB")
    cur.execute(f"CREATE TABLE PARAMETERS as select * from fromDB.PARAMETERS")
    cur.execute(f"CREATE TABLE PROFILE as select * from fromDB.PROFILE {f} LIMIT ?",[s,limit])
    cur.execute(f"UPDATE PROFILE SET idx=rowid-1")
    cur.execute(f"DETACH fromDB")

    conn.commit();
    conn.close()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Extract Subset of Profiles')
    parser.add_argument("db", help="Sqlite belt profiles")
    parser.add_argument("--fromDb","-f", help="Sqlite belt profiles", required=True)
    parser.add_argument("--y", help="Y start", default=0)
    parser.add_argument("--i", help="Index start", default=0)
    parser.add_argument("--n", help="Limit", default=1)

    args = parser.parse_args()
    process_profile(args.fromDb,args.db,args.y,args.i,args.n)
