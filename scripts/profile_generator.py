import sqlite3
import numpy as np

def process_profile(db: str, y = 0, ztype: str ='f') :
    conn = sqlite3.connect(db)
    cur = conn.cursor()
    for row in cur.execute(f"SELECT * from PROFILE where rowid >= ? order by rowid asc",[y]):
        a = yield np.frombuffer(row[2],dtype=ztype)
        if a:
            break;
        
    conn.close()


