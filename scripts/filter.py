from scipy import signal
import matplotlib.pyplot as plt
import numpy as np
from scipy import fft, ifft

# Change dtype depending on source. Usually int of float
# arr is the barrel height signal, 
# arrf is the C++ iirfilter of above.   
arr,arrf = np.loadtxt("../cads-edge/debug/filt.txt",
                 delimiter=",", dtype=float, unpack=True, usecols=(0,1))

sos = signal.iirfilter(19, 4, rs=120, btype='low',analog=False, ftype='cheby2', fs=980, output='sos')

# Uncomment to get filter parameters 
print(sos)

# Insert delay in front using usless data.
#plt.plot(np.append([0 for x in range(1,228)],arr))
plt.plot(np.append([0 for x in range(1,20)],arr))
#plt.plot(arr)

#plt.plot(arrf)
plt.plot(signal.sosfilt(sos,arr))




#Plot frequency response
if True:
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
