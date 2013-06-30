Stackduino
==========
Stackduino is a focus stacking controller for automating the process of taking image slices at high magnification to compile afterwards with software.

It uses an arduino, an easydriver and some optocouplers to move the camera/ subject a designated distance, take a photo and then repeat the process a chosen number of times until enough image slices have been taken to compile into a stack.

Features:
Manual fwd/ bwd control of stepper motor to set up the shot |
Set step size |
Set number of steps |
Set pause time between steps |
Toggle whether stage returns to start position at end of stack |
Select unit of measurement for steps: microns, mm or cm |
Select speed of stepper motor |

Thanks in particular to Flickr users bert01980 and nsomniius for bug reports and input on development.
Previous development discussion is available at:http://www.flickr.com/photos/reallysmall/8270022807/

The DigitalToggle folder should be unzipped into your arduino libraries folder. It's required for the sketch to compile.
