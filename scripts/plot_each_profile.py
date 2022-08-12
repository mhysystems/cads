import profile_generator as pg
import matplotlib.pyplot as plt
import numpy as np
import argparse

def me(z):
    r = [z[0]]
    for i in range(0,len(z)-3):
        w = z[i:i+4]
        s = w[1] - w[0] + w[2] - w[1] + w[3] - w[2]
        
        if s == 0:
            z[i+1] = w[0]
            #r.append(w[1])
        #else:
            #r.append(w[0])

        #r.append(w[1] if s > 0 else w[0])
    return z




if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Plot each profile')
    parser.add_argument("db", help="Belt data with columns")
    parser.add_argument("--y", help="Y start", default=0)

    args = parser.parse_args()
    for z in pg.process_profile(args.db,args.y):
        plt.plot(z[~np.isnan(z)])
        #plt.hist(z[ z != -32768] ,bins=100)
        plt.show()
        #input()
