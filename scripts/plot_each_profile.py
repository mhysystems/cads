import profile_generator as pg
import matplotlib.pyplot as plt
import numpy as np
import argparse



if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Plot each profile')
    parser.add_argument("db", help="Sqlite belt profiles")
    parser.add_argument("--y", help="Y start", default=0)
    parser.add_argument("--i", help="Index start", default=0)
    parser.add_argument("--s", help="Sub NaN with", default=-32.75)
    parser.add_argument("--hist","-t", help="Histogram plot", action='store_true')

    args = parser.parse_args()
    for z0 in pg.process_profile(args.db,args.y,args.i):
        if not args.hist:
            z = np.copy(z0)
            z[np.isnan(z)] = args.s
            plt.plot(z)
        else:
            plt.hist(z0[~np.isnan(z0)] ,bins=100)
        plt.show()
        #input()
