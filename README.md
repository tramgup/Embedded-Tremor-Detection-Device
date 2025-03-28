# Parkinson's Tremor Detection Device

**Final Project for the Graduate-Level Real-Time Embedded Systems Course at NYU Tandon School of Engineering**  
Developed using **PlatformIO** on **VS Code**  

## Objective  
The goal of this project was to detect **Parkinson’s resting tremors (3-6 Hz)** using the gyroscope on the **STM32 F429 Discovery Board**. Tremor detection was achieved by continuously reading gyroscope data and processing it with **Finite Impulse Response (FIR) and low-pass filters**.  

When a tremor was detected, a **message and visual alert** appeared on the LCD screen.  

## Implementation  

### Signal Processing  
- The **MBED DSP library** was used for the **FIR filter**.  
- FIR filter **coefficients** were computed using a custom **Python script** with SciPy, Matplotlib, and NumPy.  
- Frequency response graphs helped verify the filter design.  

### Tremor Detection Algorithm  
- Filtered **X, Y, and Z-axis** data was analyzed for peaks within the **3-6 Hz range**.  
- A **peak detection algorithm** counted tremor occurrences based on the number of cycles per second.  
- If a significant number of peaks were detected in this range, the system classified it as a **tremor event**.  

### LCD Screen Alert System  
- Used the STM32’s **built-in LCD library**.  
- Displayed the warning message:  
"Tremor detected, Proceed with caution!"
- A **sad face** icon appeared to visually indicate tremor detection.  

## Demo  
Check out the **project in action** on YouTube:  
[![Watch Demo](https://img.shields.io/badge/Watch%20Demo-YouTube-red?style=for-the-badge&logo=youtube)](https://youtu.be/VMxEjceP52M?si=IoP2tkF-8fLa6Y03)  
