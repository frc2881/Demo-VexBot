VexDemoRobot
============

The robot program for 2881 Vex EDR robots used at demos.

This project is written in C and must be compiled with GCC.  It is not [ROBOTC](http://www.robotc.net/download/vexrobotics/)!
To build and deploy this program, you must install the PROS development kit from <https://pros.cs.purdue.edu/>.

Be sure to scan the [Getting started](https://pros.cs.purdue.edu/getting-started/) and
[Tutorials](https://pros.cs.purdue.edu/tutorials/) sections on the PROS website.  It has lots of good information.

### CLion IDE Setup (optional)

By default, PROS installs the [Atom](https://atom.io/) editor and builds using GCC and Make.

As an alternative to Atom, you can install and use the [JetBrains CLion IDE](https://www.jetbrains.com/clion/).
CLion is commercial software, but a 30-day trial is available and students with a school e-mail address can obtain a
free license from https://www.jetbrains.com/student/. Check the list of school systems
[here](https://github.com/JetBrains/swot/blob/master/lib/domains/) to see if your school is included.  At the time
of writing, the list included:

 * `<student>@austin.cc.tx.us` - Austin Community College
 * `<student>@austinisd.org` - Austin Independent School District
 * `<student>@eanesisd.net` - Eanes Independent School District
 * `<student>@leanderisd.org` - Leander Independent School District
 * `<student>@roundrockisd.org` - RoundRock Independent School District

Once you have downloaded and installed CLion, follow the instructions in [CMakeLists.txt](CMakeLists.txt) to configure it for
cross-compiling to the Vex CPU.   CLion compiles using using the same GCC compiler used by the default PROS install,
but it uses CMake instead of Make to run the build so there's a different top-level make file (`CMakeLists.txt`
instead of `Makefile` and a different PROS configuration file (`project-cmake.pros` instead of `project.pros`).

Note that if you use Lion, you still need to install PROS to get GCC and the `pros` command-line utility.


### Deploying to the Robot

You'll need the Vex [Programming Hardware Kit](https://www.vexrobotics.com/276-2186.html) to deploy software to
the robot.

The most reliable way to deploy software is over USB, not WiFi:

 * Remove the WiFi dongles from the robot and joystick
 * Connect the robot and the joystick with a regular USB cable
 * Connect the joystick and your computer with the Programming Hardware Kit
 * The result should look like this:
     ```
     Robot <-[usb cable]-> Joystick <-[phone cord]-> Programming Adapter <-[usb cable]-> Computer
     ```
 * Turn on the robot so that it's powered by the battery, not by the computer over USB.
 * If you mount the robot on blocks so the wheels aren't touching the ground, it's easy to deploy & test,
   deploy & test while staying connected to the computer.

Technically, is is possible to deploy to a robot that's connected to the joystick via WiFi, but it's not
as reliable.  Expect the deploy process to be slower and to fail occasionally.

If you're using Atom, follow the instructions in the [PROS documentation](https://pros.cs.purdue.edu/tutorials/uploading/)
to build & deploy.

If you're using CLion, follow the instructions in [CMakeLists.txt](CMakeLists.txt) to create a Run configuration
to deploy to the robot.


### PROS Cli Quick Start

With the Atom and CLion IDEs you generally don't need to run anything from the command-line.  But if you want to
understand what those IDEs are doing under the hood, PROS provides a command-line utility with a number of useful
commands, including the following:

`pros lsusb` - Check to see if you've successfully connected your computer to the robot.

`pros poll` - Check robot health, battery levels etc.

`pros make` - Build the project using the `Makefile` provided by the PROS project template (not CMake).

`pros flash` - Deploys the program to the robot.  Preqreq: `pros make`

`pros mu` - Build and deploy the program in one step.

`pros mut` - Build and deploy the program, then start a terminal to catch `printf` statements from the Robot.  Be
careful debugging via `printf`--the debug log uses the same connection to the robot as deploying new programs, and
if you print too much you can saturate the connection and cause problems with new deploys.

`pros conduct new` - Create brand new Vex C project from template.


### Help!

In the unlikely event that the robot is completely stuck to the point where deploy just doesn't work, you should be
able to reset everything to a clean slate by installing [ROBOTC](http://www.robotc.net/download/vexrobotics/) and
using it to upload a fresh copy of the robot firmware.
