#include "pico/stdlib.h"
#include "hardware/vreg.h"
#include "hardware/clocks.h"
#include "stdio.h"
#include "drivers/st7789/st7789.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>  // For stringstream formatting
#include <sstream>  // For stringstream

#include "mlx90640.hpp"
#include "pico_display.hpp"  // Include Pico Display header

using namespace pimoroni;

ST7789 st7789(PicoDisplay::WIDTH, PicoDisplay::HEIGHT, ROTATE_0, false, get_spi_pins(BG_SPI_FRONT));
PicoGraphics_PenRGB332 graphics(st7789.width, st7789.height, nullptr);

Pen BG = graphics.create_pen(0, 0, 0);    // Background pen
Pen TEXT_PEN = graphics.create_pen(255, 255, 255);  // White text pen
Pen HOT_PEN = graphics.create_pen(255, 0, 0);       // Red for hottest point
Pen COLD_PEN = graphics.create_pen(0, 0, 255);      // Blue for coldest point

// Display size for Pico Display
const uint8_t WIDTH = 240;  // Pico Display width
const uint8_t HEIGHT = 135; // Pico Display height

// MLX90640 sensor resolution
const uint8_t SENSOR_WIDTH = 32;
const uint8_t SENSOR_HEIGHT = 24;

// min and max temperature range (in degrees C) for scaling false colour
const float temp_min = 18.0f;
const float temp_max = 38.0f;

// colour brightness - crushes dynamic range so use wisely!
const float brightness = 0.5f;

// Dirty hack to overclock the Pico before class initialization takes place
// since i2c uses the current clock frequency when determining baudrate.
class OC {
public:
    OC(uint32_t freq_khz, vreg_voltage voltage) {
        vreg_set_voltage(voltage);
        sleep_us(100);
        set_sys_clock_khz(freq_khz, true);
    }
};

OC oc(266000, VREG_VOLTAGE_1_20);

// 1MHz i2c for higher framerates
I2C i2c(4, 5, 1000000UL);

MLX90640 mlx90640(&i2c);

// Function to set a block of pixels for each sensor pixel
void set_pixel_false_colour(int x, int y, float v, int block_width, int block_height) {
    const int colours = 8;
    static float color[colours][3] = {
        {0, 0, 0},
        {0, 0, 255.0f},
        {0, 255.0f, 255.0f},
        {0, 255.0f, 0},
        {255.0f, 255.0f, 0},
        {255.0f, 0, 0},
        {255.0f, 0, 255.0f},
        {255.0f, 255.0f, 255.0f}
    };
    int idx1, idx2;
    float blend = 0.0f;
    const float temp_range = temp_max - temp_min;
    v -= temp_min;
    v /= temp_range;
    if (v <= 0) { idx1 = idx2 = 0; }
    else if (v >= 1) { idx1 = idx2 = colours - 1; }
    else {
        v *= (colours - 1);
        idx1 = std::floor(v);
        idx2 = idx1 + 1;
        blend = v - float(idx1);
    }

    int r = (int)((((color[idx2][0] - color[idx1][0]) * blend) + color[idx1][0]) * brightness);
    int g = (int)((((color[idx2][1] - color[idx1][1]) * blend) + color[idx1][1]) * brightness);
    int b = (int)((((color[idx2][2] - color[idx1][2]) * blend) + color[idx1][2]) * brightness);

    // Set a block of pixels for each sensor value
    graphics.set_pen(r, g, b);
    for (int dy = 0; dy < block_height; dy++) {
        for (int dx = 0; dx < block_width; dx++) {
            graphics.pixel(Point(x + dx, y + dy));
        }
    }
}

// Helper function to convert a float to a string with 1 decimal place and no trailing zeros
std::string format_float(float value) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(1) << value;
    return ss.str();
}

int main() {
    stdio_init_all();

    mlx90640.setup(SENSOR_WIDTH);  // Initialize the MLX90640 sensor with a resolution of 32x24

    // Calculate block size to scale the 32x24 sensor grid to the 240x135 display
    int block_width = WIDTH / SENSOR_WIDTH;
    int block_height = HEIGHT / SENSOR_HEIGHT;

    while (true) {
        mlx90640.get_frame();

        graphics.set_pen(BG);
        graphics.clear();  // Clear the display before drawing

        // Variables to track the hottest and coldest points
        float hottest_temp = -1000.0f;
        float coldest_temp = 1000.0f;
        int hottest_x = 0, hottest_y = 0;
        int coldest_x = 0, coldest_y = 0;

        // Loop through the sensor grid to get temperature values
        for (auto y = 0u; y < SENSOR_HEIGHT; y++) {
            for (auto x = 0u; x < SENSOR_WIDTH; x++) {
                int offset = y * SENSOR_WIDTH + x;
                float v = mlx90640.mlx90640To[offset];  // Temperature value from the MLX90640

                // Track hottest and coldest temperatures
                if (v > hottest_temp) {
                    hottest_temp = v;
                    hottest_x = x;
                    hottest_y = y;
                }
                if (v < coldest_temp) {
                    coldest_temp = v;
                    coldest_x = x;
                    coldest_y = y;
                }

                // Mirror the X-axis and calculate the position on the display, scaling to fit the screen
                int display_x = WIDTH - (x * block_width + block_width);  // Mirrored X-coordinate
                int display_y = y * block_height;

                // Set pixel block based on temperature value
                set_pixel_false_colour(display_x, display_y, v, block_width, block_height);
            }
        }

        // Mark the hottest and coldest points
        graphics.set_pen(HOT_PEN);  // Red for hottest point
        int display_hottest_x = WIDTH - (hottest_x * block_width + block_width);  // Mirrored X-coordinate for hottest point
        int display_hottest_y = hottest_y * block_height;
        graphics.circle(Point(display_hottest_x + block_width / 2, display_hottest_y + block_height / 2), 5);

        graphics.set_pen(COLD_PEN);  // Blue for coldest point
        int display_coldest_x = WIDTH - (coldest_x * block_width + block_width);  // Mirrored X-coordinate for coldest point
        int display_coldest_y = coldest_y * block_height;
        graphics.circle(Point(display_coldest_x + block_width / 2, display_coldest_y + block_height / 2), 5);

        // Display hottest and coldest temperatures, rounded to 1 decimal place and without trailing zeros
        graphics.set_pen(TEXT_PEN);  // White text
        graphics.text("Cold: " + format_float(coldest_temp) + "C", Point(10, HEIGHT - 20), 240, 2);  // Bottom left
        graphics.text("Hot: " + format_float(hottest_temp) + "C", Point(WIDTH - 100, HEIGHT - 20), 240, 2);  // Bottom right

        // Refresh the display
        st7789.update(&graphics);
    }

    return 0;
}
