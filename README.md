Stackduino
==========
Stackduino is a diy focus stacking controller for automating the process of taking image slices at high magnification to compile afterwards with software. With some tweaks to the code, it could also be used for any other application which involves moving a camera and taking a picture - such as 3d scanning, time-lapse, panorama stitching etc.

It uses an ATMega MCU, an easydriver and optocouplers to move the camera/ subject a chosen distance, take a photo and then repeat the process a specified number of times until enough image slices have been taken to compile into a stack.

Features:
Manual fwd/ bwd control of stepper motor to set up the shot
Menu options:
- Set step size
- Set number of steps
- Set pause time between steps
- Toggle whether stage returns to start position at end of stack
- Select unit of measurement for steps: microns, mm or cm
- Adjust speed of stepper motor
- Disable Easydriver when not stepping: runs a lot cooler and uses less energy, but your stacker mechanics must be able to hold position without the help of motor torque to use this

Thanks in particular to Flickr users bert01980 and nsomniius for bug reports and input on development.
Various prototyping images and discussion are available at: http://www.flickr.com/photos/reallysmall/sets/72157632341602394/with/10761814804/.

Notes on Stackduino code:

- The sketch is produced in the Arduino IDE
- The DigitalToggle folder should be unzipped into your arduino libraries folder. It's required for the sketch to compile

Notes on Stackduino hardware:

- The pcb was designed in Fritzing
- It's a pretty straightforward circuit and I can confirm it works but usual disclaimers apply - build at your own risk. There's a small risk the world will blow up when you plug it in
- Battery/ wall adapter power selection is done with a simple diode OR system. The supply with the highest voltage is the one that gets used, so to work as expected your battery voltage should be a little lower than your dc adapter's
- All stepper, camera and limit switch signals are output through a DB9 port. You'll need a second DB9 port at the other end of the cable to break these signals out again at your stacker
- The board provides a pinout for programming the MCU with a FT232RL USB > Serial Adapter

Next steps:

v1 of the Stackduino pcb probably won't be developed any further but tweaks to the code will be ongoing. Pull requests with improvements always well received.

Tinkering has started on Stackduino v2. This will be largely based on SMD components and build on v1 with various additions and improvements:
- much smaller
- much more efficient use of batteries
- built in micro-usb port for programming
