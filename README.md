Stackduino
==========
Stackduino is a controller for automating focus stacking which you can build yourself. 

Focus stacking is a technique commonly used in macro-photography which combines multiple images taken at different focus distances (using a linear stage) to give a resulting image with a greater depth of field than any of the individual source images.
Focus stacking can be done manually by adjusting the linear stage to the next position, releasing the camera shutter and repeating. There are some drawbacks to doing it by hand however:

- Adjusting the stage accurately by eye can be difficult and repeating that accuracy for every focus slice, even more so. Any slices which are too deep will create an out of focus band in the final image
- Touching the equipment introduces vibration and small movements which can effect the final image adversely
- Manual stacking is time-consuming. If photographing a live subject the window of opportunity may be lost before the stack is complete

Attaching a small stepper motor to the linear stage's actuator and controlling it electronically avoids these problems. Focus stacks become quick, accurate and repeatable.

Stackduino is a focus stacking controller which coordinates a stepper motor and a camera's shutter release to automate the stacking process.
With some tweaks to the code, it could also be used for any other application which involves moving a camera and taking a picture - such as 3d scanning, time-lapse, panorama stitching etc.

This repository includes the gerber files required to produce a pcb, a parts list and the code required to run the controller. In addition, you will need a focus stacking setup to connect it to(!) and optionally an enclosure to put the controller into.

Features:
Manual fwd/ bwd control of stepper motor to set up the shot
Menu options:
- Set step size
- Set number of steps
- Set pause time between steps
- Set number of images to take per step (HDR support)
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
- smaller
- more efficient use of batteries
- built in micro-usb port for programming
