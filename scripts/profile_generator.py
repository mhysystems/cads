import sqlite3
import numpy as np

def process_profile(db: str, q : str, ztype: str ='f') :
    conn = sqlite3.connect(db)
    cur = conn.cursor()
        
    for row in cur.execute(q):
        a = yield np.frombuffer(row[0],dtype='f')
        if a:
            break;
        
    conn.close()


