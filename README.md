# Embedded-Tremor-Detection-Device
Final Project for the Graduate level Real time Embedded Systems course at NYU Tandon School of engineering. This project was done using the Platform IO library on VS Code.

# Objective
The main objective for our project was to utilize the gyroscope on the STM32 F429 Discovery board to detect a Parkinson’s resting tremor between 3 to 6 Hz. Data from the gyroscope was continuously read and then processed using FIR and low pass filters. The data for the X, Y, and Z axis were filtered, and the peaks within the frequency window were counted. When any axis value fell within the 3 to 6 Hz frequency range, a 
tremor was detected and a message was displayed on the LCD screen.

