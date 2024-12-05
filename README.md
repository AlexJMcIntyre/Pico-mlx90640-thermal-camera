# Pico-mlx90640-thermal-camera
Code to display images from a mlx90640 thermal camera on a Pimoroni pico display, using a Raspberry Pi Pico

Features:
A nice colour-coded scale. 
highlights the hottest and coldest parts of the image, and gives you those temperatures in degrees Celsius. 
Overclocks the Pico to get the best i2c baud rate. 

Hardware required:
- Raspberry Pi Pico. 
- MLX90640 Thermal Camera Breakout (https://shop.pimoroni.com/products/mlx90640-thermal-camera-breakout)
- Either a Pico Display Pack (https://shop.pimoroni.com/products/pico-display-pack)
   or a Pico Display Pack 2.8" (https://shop.pimoroni.com/products/pico-display-pack-2-8)

Putting it together:
To connect the camera, you can either solder the pins directly:
  SDA -> GPIO 4
  SCL -> GPIO 5
  3V3 -> 3V3
  GND -> GND

  or alternatively there is a nice no-solder solution if you're using the 2.8" display, since the good folks at Pimoroni included a QW/ST connector on the display. To do it this way, you'll also need:
  - Qw/ST to Breakout Garden Adapter (https://shop.pimoroni.com/products/stemma-qt-qwiic-to-breakout-garden-adapter)
  - 4 Pin JST-SH Cable (https://shop.pimoroni.com/products/jst-sh-cable-qwiic-stemma-qt-compatible , the JST-SH to JST-SH one)

Just pop the camera into the breakout garden adapter, and connect the Qw/ST ports on the breakout garden and the display. 

Software:
I've included both .uf2 files and .cpp source code for both display variants. 
If you want to use the .uf2 files, Put your pico into bootloader mode and just upload the correct variant. 
