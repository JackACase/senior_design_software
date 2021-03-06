# Crash n’ Learn Car Platform Software

## Background

​	The main goal of MSD project P19393 was to design a replacement for the aging NXP Cup car platform. The details of this project may be found on our [EDGE page](http://edge.rit.edu/edge/P19393/public/Home). While the focus of the project was on the mechanical and electrical design of the product, this software was written to demonstrate its capabilities at the annual Imagine RIT event. A video of our car driving around the track running this software can be seen on YouTube [here](https://youtu.be/WNO-WMuB2no).

​	This software was developed in NXP’s MCUXpresso IDE, which provided access to the latest hardware drivers, as well as provided built-in support for FreeRTOS. MCUXpresso can be downloaded [here](https://www.nxp.com/support/developer-resources/software-development-tools/mcuxpresso-software-and-tools:MCUXPRESSO). MCUXpresso made it easy to develop for the NXP K64F development board as it allowed peripheral and pin mux configuration through easy-to-use GUI tools which generate the necessary code. The built-in FreeRTOS support was also useful for debugging and profiling the performance of the system. 

## Build Instructions

To import this software repository into MCUXpresso as a project, first create a workspace, as prompted when starting the IDE. From there use *File>Open Projects from File System...* Next in the *Installed SDKs* panel, check the box next to *SDK_2.x_FRDM-K64F* if it is installed, otherwise it can be downloaded from [this page](https://mcuxpresso.nxp.com/en/select). Search for *K64* in the search box and select *FRDM-K64F* from the list of suggestions. Click on *Build MCUXpresso SDK*, and when that is done select your development environment settings and click on *Download SDK*. To install it, simply drag the SDK zip file into the *Installed SDKs* panel. You will now be able to build the project and load it onto your K64. 