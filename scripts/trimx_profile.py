import numpy as np

def transform_profile(row):
    (y,x_off,z,rowid) = row
    return (y,x_off,z[0:1890],rowid)    
