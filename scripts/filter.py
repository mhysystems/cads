from scipy import signal
import matplotlib.pyplot as plt
import numpy as np
from scipy import fft, ifft


def dcfilter(x):
    yn = 0
    xn = 0
    r = []
    
    for e in x:
        y = e - xn + 0.95 * yn
        xn = e
        yn = y
        r.append(y)
    
    return r 



# Change dtype depending on source. Usually int of float
# arr is the barrel height signal, 
# arrf is the C++ iirfilter of above.   
#arr,arrf = np.loadtxt("../cads-edge/debug/filt.txt",
#                 delimiter=",", dtype=float, unpack=True, usecols=(0,1))

arr = np.loadtxt("filt.txt")
#Whaleback CV405
#sos = signal.iirfilter(19, 4, rs=120, btype='low',analog=False, ftype='cheby2', fs=980, output='sos')

#Jimblebar CV001
sos = signal.iirfilter(19, 4, rs=120, btype='low',analog=False, ftype='cheby2', fs=980, output='sos')


# Uncomment to get filter parameters 
#print(repr(sos))

# Insert delay in front using usless data.
plt.plot(np.append([0 for x in range(1,334)],arr))
#plt.plot(np.append([0 for x in range(1,334)],arrf))
#plt.plot(arr)
#np.savetxt("profile.txt",arr)
#plt.plot(arrf)

plt.plot(signal.sosfilt(sos,arr))




#Plot frequency response
if False:
    w, h = signal.sosfreqz(sos, 2000, fs=980)
    fig = plt.figure()
    ax = fig.add_subplot(1, 1, 1)
    ax.semilogx(w, 20 * np.log10(np.maximum(abs(h), 1e-5)))
    ax.set_title('Chebyshev Type II bandpass frequency response')
    ax.set_xlabel('Frequency [Hz]')
    ax.set_ylabel('Amplitude [dB]')
    ax.axis((0, 1000, -100, 10))
    ax.grid(which='both', axis='both')

plt.show()
