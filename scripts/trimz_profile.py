import numpy as np

def transform_profile(row):
    (y,x_off,z,rowid) = row
    z = z - 3.0
    z[z < 1.0 ] = 0.0
    return (y,x_off,z,rowid)    
