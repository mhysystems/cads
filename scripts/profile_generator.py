import sqlite3
import numpy as np

def process_profile(db: str, y = 0, i = 0, ztype: str ='f') :
    conn = sqlite3.connect(db)
    cur = conn.cursor()
    q = "SELECT z from PROFILE where y >= ? order by y asc"
    p = y
    if i != 0:
        q = "SELECT z from PROFILE where rowid >= ? order by rowid asc"
        p = i
        
    for row in cur.execute(q,[p]):
        a = yield np.frombuffer(row[0],dtype=ztype)
        if a:
            break;
        
    conn.close()


