
# Younokio (2011)

[![younokio](http://img.youtube.com/vi/6guHZoSqejA/0.jpg)](http://www.youtube.com/watch?v=6guHZoSqejA "younokio")

YouNokio is an _interactive installation that allows people to connect to a string operated puppet_.
KinectSpace is an application that sends Kinect Skeleton data via OSC (Hands, Kness (x) shoulders (y) tilt (z)).

## How to build it

Build your own kinect marionette

![younokio_diagram](https://user-images.githubusercontent.com/724604/67327163-ca623f00-f517-11e9-840d-de3dd6f17e4a.png)

### Kinect (hardware and software)

Download and unzip [Kinect SDK](http://research.microsoft.com/en-us/um/redmond/projects/kinectsdk/download.aspx), get [cinder](http://libcinder.org/)). Install visual studio 2011 (trial) from Microsoft. Fast forward learn about c++, openGL, kinect sdk, cinder, and how to make a new Cinder project in VS.

Thanks to [bantherewind](http://bantherewind.com/kinect-sdk-block-for-cinder) it was very easy to get the skeleton data. I made a small visualization to understand how the 3d vectors work then adapted the code to limit to one active skeleton and send via OSC hands and knees (and later pan and tilt) in float values from 0 to 1. 

#### USB-SSC32 (hardware)

Got bunch of L and C connectors, servo brackets and couple of servos that can take some torque. Got a [USB-SSC32](http://www.lynxmotion.com/images/html/build136.htm#softver) board and [installed it](http://www.ftdichip.com/Drivers/VCP.htm) - so it can be accessed via USB, but seen as a virtual COM port. Tested it using telnet (connect at baud-rate 115200) and issued commands. If all works well, the same commands can be issued from code, in my case I chose [Processing](www.processing.org) because it is more comfortable to code (but it might as well be c++)

#### USB-SSC32 (software)

I used [Processing](www.processing.org) and the serial and [oscP5](http://www.sojamo.de/libraries/oscP5/index.html) and [controlp5](http://www.sojamo.de/libraries/controlP5/index.html) libraries to write a [small schetch](https://github.com/one1zero1one/Younokio/tree/master/Processing.org) that would allow me to translate recieved OSC float values to the servo motors, while being able to visually select which OSC goes to what servo ~~and calibrate on the fly~~. It did its job, but due to limited time it's not really optimized code, more of a hack.

#### Wooden frame and marionette

Using L connectors and a power-drill I build a wooden holder for the servos and kinect, and in the last day purchased a marionette from local hobby shop (those things are so hard to find). I hacked a way to connect the servos and hang the puppet, and then I used the processing bits above to calibrate with some steady OSC input coming from Mixbox. [Movie](https://www.youtube.com/watch?v=w1y_69nvrjs) from the calibration process (thx Alina).

### Thanks

Ama, Alina, Aaron, Toma, Elena, Miruna, Teo, Ana, Vlad
