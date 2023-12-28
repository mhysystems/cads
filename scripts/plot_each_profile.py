import profile_generator as pg
import matplotlib.pyplot as plt
import numpy as np
import argparse
from scipy.interpolate import CubicSpline


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Plot each profile')
    parser.add_argument("db", help="Sqlite belt profiles")
    parser.add_argument("--begin","-b", help="Begin Position", default=0)
    parser.add_argument("--table","-l", help="Source Table", default="profile")
    parser.add_argument("--s", help="Sub NaN with", default=-15.00)
    parser.add_argument("--hist","-t", help="Histogram plot", action='store_true')

    args = parser.parse_args()
    q = ""
    match args.table:
        case "transient":
            q = "select z from ErroredProfile where id like 'raw_%'"
        case _:
            q = f"select z from profiles where rowid > {args.begin};"

    for z0 in pg.process_profile(args.db,q):
        if not args.hist:
            z = np.copy(z0)
            #x = np.arange(len(z))
            #x2 = x[~np.isnan(z)]
            #z2 = z[~np.isnan(z)]
            #cs = CubicSpline(x2,z2)
            #plt.plot(cs(x))
            z[np.isnan(z)] = args.s
            plt.plot(z)
        else:
            z = z0[~np.isnan(z0)]
            plt.hist(z ,bins=100)
        plt.show()
        #input()
