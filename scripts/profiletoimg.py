import sqlite3
import numpy as np
import argparse
from PIL import Image

def process_belt(db,nan) :
    
    maxrows = 60000
    conn = sqlite3.connect(db)
    cur = conn.cursor()
    m = 99999999999
    rowcnt = 1    
    ss = np.vectorize(lambda x :x if not np.isnan(x) else nan ) 
    yindex = 0
    while rowcnt > 0:
        rowcnt = 0
        b = []
        for row in cur.execute(f"SELECT z from PROFILES where rowid >= ? order by rowid asc limit ?",(yindex,maxrows)):
            rowcnt = rowcnt + 1
            z = np.frombuffer(row[0],dtype='f')
            m = min(m,len(z))
            b.append(ss(z))
        if rowcnt > 0: 
            #i = Image.fromarray(np.array([a[:m] for a in b ]) * 0.2)
            v = np.array([a[:m] for a in b ]) 
            i = Image.fromarray(255 * (v - v.min()) / (v.max() - v.min()))
            i.convert("L").save(f"{db}-{yindex}.png")
            yindex = yindex + maxrows
    conn.close()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Generate PNG from profile DB')
    parser.add_argument("db", help="Belt DB")
    parser.add_argument("--nan", type=float, help="Substitute NaN", default=-25.0)

    args = parser.parse_args()
    process_belt(args.db,args.nan)
