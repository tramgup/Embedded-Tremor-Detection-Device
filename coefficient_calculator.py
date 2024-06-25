#python code to calculate coeffiicents used for the fir Filter, need to pip install scipy and matplot
import numpy as np
from scipy.signal import firwin, freqz
import matplotlib.pyplot as plt

# Filter specifications
num_taps = 32  # Number of filter taps (filter order + 1)
sampling_rate = 100  # Sampling frequency in Hz
low_cutoff = 3  # Low cutoff frequency in Hz
high_cutoff = 6  # High cutoff frequency in Hz

# Generate bandpass FIR coefficients using firwin
fir_coeffs = firwin(num_taps, [low_cutoff, high_cutoff], pass_zero=False, fs=sampling_rate)

# Frequency response of the filter
freq, response = freqz(fir_coeffs, worN=8000)
response_magnitude = 20 * np.log10(np.maximum(abs(response), 1e-10))  # Convert to dB to avoid log of zero

# Plotting the frequency response
plt.figure()
plt.plot(freq / (2 * np.pi) * sampling_rate, response_magnitude)
plt.title('Frequency Response of the FIR Filter')
plt.xlabel('Frequency (Hz)')
plt.ylabel('Magnitude (dB)')
plt.grid(True)
plt.show()

# Output the coefficients for use in C/C++
print("FIR Coefficients:")
print(fir_coeffs)

