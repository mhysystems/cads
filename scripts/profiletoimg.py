import sqlite3
import numpy as np
import argparse
from PIL import Image

def process_belt(db,off,num) :
    
    #maxrows = 2000
    maxrows = 60000
    conn = sqlite3.connect(db)
    cur = conn.cursor()
    m = 99999999999
    rowcnt = 1    
    #ss = np.vectorize(lambda x :x if x != -32768 else -2627 ) 
    ss = np.vectorize(lambda x :x if not np.isnan(x) else -33.0 ) 
    yindex = 0
    while rowcnt > 0:
        rowcnt = 0
        b = []
        for row in cur.execute(f"SELECT z from PROFILE where idx >= ? order by idx asc limit ?",(yindex,maxrows)):
            rowcnt = rowcnt + 1
            #z = np.frombuffer(row[2],dtype='i2')
            z = np.frombuffer(row[0],dtype='f')
            m = min(m,len(z))
            b.append(ss(z))
        if rowcnt > 0: 
            #i = Image.fromarray(np.array([a[:m] for a in b ]) * 0.2)
            v = np.array([a[:m] for a in b ]) 
            i = Image.fromarray(255 * (v - v.min()) / (v.max() - v.min()))
            i.convert("L").save(f"whaleback-{yindex}.png")
            yindex = yindex + maxrows
    conn.close()

def process_beltf(db,off,num) :

    conn = sqlite3.connect(db)
    cur = conn.cursor()
    b = []
    m = 99999999999
    for row in cur.execute(f"SELECT * from PROFILE where y > 60000 and y < 120000 order by y asc"):
        z = np.frombuffer(row[2],dtype='f')
        fz = [x for x in z.tolist() if np.isfinite(x) ]
        m = min(m,len(fz))
        b.append(fz)
    conn.close()
    aa = np.array([a[:m] for a in b ])
    imageio.imwrite('result1.bmp', aa.astype(np.single))

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Extract profile data from sqlitedb')
    parser.add_argument("db", help="Belt data with columns")
    parser.add_argument("--off", type=int, help="offset",default = 0)

    args = parser.parse_args()
    process_belt(args.db,args.off,10)
