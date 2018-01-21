## Synposis
This is my attempt at recreating the old DOS Asteroids game on an ARM Cortex M4 board.

## Preview
<div align="center">
  <img src="https://github.com/cpagravel/asteroids/blob/master/demo.gif" height="200"/>
  <img src="https://github.com/cpagravel/asteroids/blob/master/snapshot.jpg" height="200"/>
</div>

## Installation
Simply load the project file, compile and upload to your board using Keil 4.

## Tweaking
To adjust the speed of things, change the delay values in `Tasks.c`. For example, if you want smoother graphics, then reduce the value of `lcd_frame_delay`. If you want the physics to be calculated more often (makes game appear faster) than decrease the value of `physics_delay`.