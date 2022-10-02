import sqlite3
import math
import random
from datetime import datetime


conn = sqlite3.connect('anomalies.db')
cur = conn.cursor()

columns = [('anomaly_ID','integer not null primary key'),('start', 'real'), ('length', 'real'), ('x_lower','real'), ('x_upper', 'real'), ('z_depth', 'real'), ('area', 'real'),('volume', 'real'), ('time', 'text'), ('epoch', 'integer'), ('contour_x', 'real'), ('contour_theta', 'real'), ('category', 'text'), ('danger', 'integer'),('comment','text')]
category = ["join","tear","rip"]
danger = ['benign','caution','malignment','operate','stop belt now']


cur.execute('DROP TABLE IF EXISTS A_TRACKING')
cur.execute('DROP TABLE IF EXISTS GUI')

cur.execute(f'CREATE TABLE GUI (anomaly_ID integer not null primary key, visible integer )')
cur.execute(f'CREATE TABLE A_TRACKING ( {",".join([ a + " " + b for (a,b) in columns])})')
for x in range(1,10):
    tmp = random.uniform(0,2*math.pi) #start
    v = (x,
            tmp,
            random.uniform(0,2*math.pi), #length
            random.uniform(0,750), #x_lower
            random.uniform(750,1500), #x_upper
            random.uniform(0,4), #z_depth
            random.uniform(0,1000), #area
            random.uniform(0,5000), #volume
            str(datetime.now()), #time
            random.randint(1,1000), #epoch
            random.uniform(1,1000), #contour_x
            random.uniform(0,2*math.pi), #contour_theta
            random.choice(category), #category
            random.randint(1,10), #random.choice(danger)
            "" #comment

            )
    cur.execute(f'INSERT INTO A_TRACKING ({",".join([ a for (a,b) in columns])}) VALUES ({",".join([ "?" for (a,b) in columns])})',v)
    cur.execute(f'INSERT INTO GUI (anomaly_ID,visible) VALUES (?,?)',(x,1))


conn.commit()
conn.close()
