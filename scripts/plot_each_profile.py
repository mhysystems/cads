import profile_generator as pg
import matplotlib.pyplot as plt
import numpy as np
import argparse



if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Plot each profile')
    parser.add_argument("db", help="Sqlite belt profiles")
    parser.add_argument("--y", help="Y start", default=0)
    parser.add_argument("--hist","-t", help="Histogram plot", action='store_true')

    args = parser.parse_args()
    for z in pg.process_profile(args.db,args.y):
        if not args.hist:
            plt.plot(z[~np.isnan(z)])
        else:
            plt.hist(z[~np.isnan(z)] ,bins=1000)
        plt.show()
        #input()
