
import profile_generator as pg
import matplotlib.pyplot as plt
import numpy as np
import argparse
from scipy.interpolate import CubicSpline

def sum(a,b):
    if len(a) == 0:
        return b
    if b.size  == 0:
        return a
    return np.nan_to_num(a) + np.nan_to_num(b)[:a.size]

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Plot each profile')
    parser.add_argument("db", help="Sqlite belt profiles")

    q = f"select z from profile where idx >= 0;"
    args = parser.parse_args()
    a = np.array([])
    n = 0
    for z0 in pg.process_profile(args.db,q):
        a = sum(a,z0)
        n = n + 1

    a = a / n
    a = a[54:109]
    #plt.plot(a)  
    i = 1 / (a + 32) 
    print(i.tolist())
    plt.plot(i*(a+32) -32)
    plt.show()
        #input()
