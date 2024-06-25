//Embedded Challenge Code
//By: Pierson Cai(pc2819), Traman Gupta (tg2204), and Elaina Zodiatis(ez2241)
#include "mbed.h"
#include "arm_math.h"
#include "drivers/LCD_DISCO_F429ZI.h" 

#define WINDOW_SIZE 3 // Window size, adjust as needed

// Define Regs & Configurations --> Gyroscope's settings
#define CTRL_REG1 0x20
#define CTRL_REG1_CONFIG 0b01'10'1'1'1'1
#define CTRL_REG4 0x23 // Second configure to set the DPS // page 33
#define CTRL_REG4_CONFIG 0b0'0'01'0'00'0

#define CTRL_REG3 0x22 // page 32
#define CTRL_REG3_CONFIG 0b0'0'0'0'1'000

#define OUT_X_L 0x28

#define SPI_FLAG 1
#define DATA_READY_FLAG 2

#define SCALING_FACTOR (17.5f * 0.017453292519943295769236907684886f / 1000.0f)

#define FILTER_COEFFICIENT 0.05f // Adjust this value as needed
#define PEAK_THRESHOLD 0.1f // Adjust this threshold as needed

#define NUM_TAPS 31 // Use the same number of taps as in the example
#define BLOCK_SIZE 1

const float32_t firCoeffs32[NUM_TAPS] = {
    -0.00222951f, -0.00465854f, -0.0086943f, -0.01488284f, -0.02286169f, -0.03121282f,
    -0.03763622f, -0.03943306f, -0.03419443f, -0.02052307f,  0.00140974f,  0.02963956f,
     0.06060674f,  0.08975984f,  0.11244049f,  0.12484703f,  0.12484703f,  0.11244049f,
     0.08975984f,  0.06060674f,  0.02963956f,  0.00140974f, -0.02052307f, -0.03419443f,
    -0.03943306f, -0.03763622f, -0.03121282f, -0.02286169f, -0.01488284f, -0.0086943f
    -0.00465854f, -0.00222951f 
}; //calculated coefficients using numpy on python, can run the provided file and change if desired

static float32_t firStateF32[BLOCK_SIZE + NUM_TAPS - 1]; // State buffer

// EventFlags object declaration
EventFlags flags;

LCD_DISCO_F429ZI lcd;  // Create an LCD object

arm_fir_instance_f32 S; //FIR instance for bandpass FIR filter

// spi callback function
void spi_cb(int event) {
    flags.set(SPI_FLAG);
}

// data ready callback function
void data_cb() {
    flags.set(DATA_READY_FLAG);
}

void drawApproximateArc(int xCenter, int yCenter, int radiusX, int radiusY, int startAngle, int endAngle, int numSegments = 20) {
    float prevX, prevY;
    float angleStep = (endAngle - startAngle) / (float)numSegments;
    // Start from the end angle and go backwards
    for (int i = numSegments; i >= 0; i--) {  
        float angle = startAngle + i * angleStep;
        float x = xCenter + radiusX * cos(angle * 3.14159 / 180); 
        // Invert Y-coordinate to flip the arc
        float y = yCenter - radiusY * sin(angle * 3.14159 / 180);  
        if (i < numSegments) { 
            lcd.DrawLine(prevX, prevY, x, y); 
        }
        prevX = x;
        prevY = y;
    }
}

// Function to draw a sad face on the LCD screen
void drawSadFace(uint32_t textColor) {
    lcd.SetTextColor(textColor);

    // Eyes (combined for efficiency)
    lcd.FillCircle(70, LINE(5), 25);   // Left eye
    lcd.FillCircle(170, LINE(5), 25);  // Right eye

    lcd.SetTextColor(LCD_COLOR_WHITE); // Inner eye (white)
    lcd.FillCircle(75, LINE(5), 5);    // Left
    lcd.FillCircle(165, LINE(5), 5);   // Right

    // Sad mouth (changed to an arc)
    lcd.SetTextColor(textColor);
    drawApproximateArc(120, LINE(12), 50, 50, 0, 180); 
}

// Function to clear the sad face from the LCD screen
void clearSadFace(uint32_t backColor) {
    // Eyes
    lcd.SetTextColor(backColor);
    lcd.FillCircle(70, LINE(5), 25);
    lcd.FillCircle(75, LINE(5), 5);
    lcd.FillCircle(170, LINE(5), 25);
    lcd.FillCircle(165, LINE(5), 5);

    // Sad mouth
    Point sadMouth[] = {{60, LINE(10)}, {100, LINE(15)}, {140, LINE(10)}};
    lcd.FillPolygon(sadMouth, 3);
}

//function to flash the displayed screen when tremor is detected
void flashMessageAndSadFace(const char* message1, const char* message2, uint32_t textColor, uint32_t backColor, int flashInterval, int duration) {
    int elapsedTime = 0;

    while (elapsedTime < duration) {
        // Turn the text and sad face on
        lcd.SetTextColor(textColor);
        lcd.DisplayStringAt(0, LINE(0), (uint8_t*)message1, CENTER_MODE);
        lcd.DisplayStringAt(0, LINE(1), (uint8_t*)message2, CENTER_MODE);
        drawSadFace(textColor);
        ThisThread::sleep_for(chrono::milliseconds(flashInterval));
        
        // Turn the text, eyes, and sad face off
        lcd.Clear(backColor);
        ThisThread::sleep_for(chrono::milliseconds(flashInterval));

        elapsedTime += 2 * flashInterval; 
    }
}

// Function to count peaks in a given data array
int count_peaks(float* data, int size) {
    int count = 0;
    for (int i = 1; i < size - 1; ++i) {
        if (data[i] > data[i - 1] && data[i] > data[i + 1] && data[i] > PEAK_THRESHOLD) {
            count++;
        }
    }
    return count;
}

int main() {
    //creating values for the filtered data we will be using
    float filtered_data_x[WINDOW_SIZE];
    float filtered_data_y[WINDOW_SIZE];
    float filtered_data_z[WINDOW_SIZE];

    // Initialize filtered values outside the loop (avoids undefined behavior on first iteration
    float32_t filtered_gx = 0.0f;
    float32_t filtered_gy = 0.0f;
    float32_t filtered_gz = 0.0f;

    // Define the message being displayed text and colors
    const char* message1 = "Tremor detected.";
    const char* message2 = "Proceed with caution!";
    const uint32_t textColor = LCD_COLOR_RED;
    const uint32_t backColor = LCD_COLOR_BLACK;
    const int flashInterval = 250; // 500 ms = 0.25 seconds
    const int duration = 3000; // Flash for 3 seconds

    //creating variables for when detecting peak counts
    int peak_count_x = 0;
    int peak_count_y = 0;
    int peak_count_z = 0;

    //spi initialization
    SPI spi(PF_9, PF_8, PF_7, PC_1, use_gpio_ssel);
    uint8_t write_buf[32], read_buf[32];

    //interrupt initialization
    InterruptIn int2(PA_2, PullDown);
    int2.rise(&data_cb);

    // Initialize FIR filter instance
    arm_fir_init_f32(&S, NUM_TAPS, (float32_t *)&firCoeffs32[0], &firStateF32[0], BLOCK_SIZE);
    
    //spi format and frequency
    spi.format(8, 3);
    spi.frequency(1'000'000);

    // Write to control registers --> spi transfer
    write_buf[0] = CTRL_REG1;
    write_buf[1] = CTRL_REG1_CONFIG;
    spi.transfer(write_buf, 2, read_buf, 2, spi_cb);
    flags.wait_all(SPI_FLAG);

    write_buf[0] = CTRL_REG4;
    write_buf[1] = CTRL_REG4_CONFIG;
    spi.transfer(write_buf, 2, read_buf, 2, spi_cb);
    flags.wait_all(SPI_FLAG);

    write_buf[0] = CTRL_REG3;
    write_buf[1] = CTRL_REG3_CONFIG;
    spi.transfer(write_buf, 2, read_buf, 2, spi_cb);
    flags.wait_all(SPI_FLAG);

    write_buf[1] = 0xFF;

    Timer timer;
    timer.start();

    //(polling for\setting) data ready flag
    if (!(flags.get() & DATA_READY_FLAG) && (int2.read() == 1)) {
        flags.set(DATA_READY_FLAG);
    }

    while (1) {
        int16_t raw_gx, raw_gy, raw_gz;
        float gx, gy, gz;

        flags.wait_all(DATA_READY_FLAG);
        write_buf[0] = OUT_X_L | 0x80 | 0x40;

        spi.transfer(write_buf, 7, read_buf, 7, spi_cb);
        flags.wait_all(SPI_FLAG);

        // Process raw data
        raw_gx = (((uint16_t)read_buf[2]) << 8) | ((uint16_t)read_buf[1]);
        raw_gy = (((uint16_t)read_buf[4]) << 8) | ((uint16_t)read_buf[3]);
        raw_gz = (((uint16_t)read_buf[6]) << 8) | ((uint16_t)read_buf[5]);

        gx = ((float)raw_gx) * SCALING_FACTOR;
        gy = ((float)raw_gy) * SCALING_FACTOR;
        gz = ((float)raw_gz) * SCALING_FACTOR;

        // Apply FIR filter from dsp lib (using our calculated coefficients)
        arm_fir_f32(&S, &gx, &filtered_gx, BLOCK_SIZE);
        arm_fir_f32(&S, &gy, &filtered_gy, BLOCK_SIZE);
        arm_fir_f32(&S, &gz, &filtered_gz, BLOCK_SIZE);

        // Apply Simple low-pass filter
        filtered_gx = FILTER_COEFFICIENT * gx + (1 - FILTER_COEFFICIENT) * filtered_gx;
        filtered_gy = FILTER_COEFFICIENT * gy + (1 - FILTER_COEFFICIENT) * filtered_gy;
        filtered_gz = FILTER_COEFFICIENT * gz + (1 - FILTER_COEFFICIENT) * filtered_gz;

          // Shift the existing data in the array to make space for new data
        for(int j = WINDOW_SIZE - 1; j > 0; j--) {
                filtered_data_x[j] = filtered_data_x[j - 1];
                filtered_data_y[j] = filtered_data_y[j - 1];
                filtered_data_z[j] = filtered_data_z[j - 1];
        }

        // Add the new filtered data at the beginning of the array
        filtered_data_x[0] = filtered_gx;
        filtered_data_y[0] = filtered_gy;
        filtered_data_z[0] = filtered_gz;

        // Count peaks for each axis within the window
        peak_count_x += count_peaks(filtered_data_x, WINDOW_SIZE);
        peak_count_y += count_peaks(filtered_data_y, WINDOW_SIZE);
        peak_count_z += count_peaks(filtered_data_z, WINDOW_SIZE);

        // Check if one second has elapsed
        if (timer.read_ms() >= 1000) {
            // printf("Peak count in one second - X: %d, Y: %d, Z: %d\n", peak_count_x, peak_count_y, peak_count_z);

            // Check if peak count is between 3 and 6 (3 Hz and 6 Hz)
            if (peak_count_x >= 3 && peak_count_x <= 6) {
                flashMessageAndSadFace(message1, message2, textColor, backColor, flashInterval, duration);
            }
            if(peak_count_y >= 3 && peak_count_y <= 6){
                flashMessageAndSadFace(message1, message2, textColor, backColor, flashInterval, duration);
            }
            if(peak_count_z >= 3 && peak_count_z <= 6){
                flashMessageAndSadFace(message1, message2, textColor, backColor, flashInterval, duration);
            }

            // Reset timer and peak count
            timer.reset();
            peak_count_x = 0;
            peak_count_y = 0;
            peak_count_z = 0;
        }


        thread_sleep_for(100);
    }
}


