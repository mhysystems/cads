from scipy import signal
import matplotlib.pyplot as plt
import numpy as np
from scipy import fft, ifft

#arr = np.loadtxt("../cads-edge/debug/what.txt",
#                 delimiter=",", dtype=int, usecols=(0))

arr,arrf = np.loadtxt("../cads-edge/debug/filt.txt",
                 delimiter=",", dtype=int, unpack=True, usecols=(0,1))

sos = signal.iirfilter(9, 5, rs=60, btype='low',analog=False, ftype='cheby2', fs=980, output='sos')
b,a = signal.iirfilter(9, 5, rs=60, btype='low',analog=False, ftype='cheby2', fs=980)
print(sos)
#print(b[::-1],a[::-1])
#w, gd = signal.group_delay((b, a),fs=980)
#plt.plot(w, gd)
w, h = signal.sosfreqz(sos, 2000, fs=980)

plt.plot(np.append([x for x in range(1,198)],arr))
plt.plot(arrf)
#plt.plot(signal.sosfilt(sos,arr))
#plt.plot(signal.lfilter(b,a,arr))




fig = plt.figure()
ax = fig.add_subplot(1, 1, 1)
ax.semilogx(w, 20 * np.log10(np.maximum(abs(h), 1e-5)))
ax.set_title('Chebyshev Type II bandpass frequency response')
ax.set_xlabel('Frequency [Hz]')
ax.set_ylabel('Amplitude [dB]')
ax.axis((0, 1000, -100, 10))
ax.grid(which='both', axis='both')
plt.show()
