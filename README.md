# Meteor Storm, a microgame for attiny85 (with an SSD1306-based OLED screen)

![Img 1](http://albertgonzalez.coffee/projects/attiny85_meteor_storm/img/1_400.png)
![Img 2](http://albertgonzalez.coffee/projects/attiny85_meteor_storm/img/2_400.png)

A pretty raw single-button microarcade game (similar to the well-known _Flappy Bird_ and many others)
made in a bunch of hours to test some __attiny85__ + __SSD1306 OLED screen__ stuff.

(as a future __TODO__, the code can probably be improved, cleaned and/or expanded - maybe I'll continue this in the future, who knows...).

Also check [the project page on hackaday.io](https://hackaday.io/project/186084-meteor-storm-a-microgame-for-attiny85)!

## Setup and install
Connect an __I2C__ SSD1306-based screen to the attiny85 __SDA__ and __SCL__ pins, add one button on __PB1__ and run _make_.

### How to play
Avoid hitting the screen borders and the big white squares (aka "meteors"). Hold the button to move up and release it to start falling
(some acceleration comes into play, here!). There's a non-persistent __high-score function__ + __current score display__ while playing.

## Additional libraries
### SSD1306 driver
I used the same driver I wrote for screen handling on my [Game of Life implementation on an attiny85](https://github.com/theisolinearchip/gameoflife_attiny85).
It's basically a set of command / data writing functions + some initial configurations and flash memory reading operations.

### I2C driver
I also wrote [the I2C driver](https://github.com/theisolinearchip/i2c_attiny85_twi) using the TWI interface for attiny85's micros
(the _wait times_ are tweaked a little bit for speed improvement - it seems to work fine with this specific screens).

## Links
- [Project page on hackaday.io](https://hackaday.io/project/186084-meteor-storm-a-microgame-for-attiny85)
- [Attiny Arduino Games](https://github.com/andyhighnumber/Attiny-Arduino-Games), by Andy Jackson; more attiny85-based microgames and similar stuff
(the numbers font I'm using here is from the font file that can be found there!).
- My [Game of life on an attiny85](https://hackaday.io/project/181421-game-of-life-on-an-attiny85) project page on hackaday.io; I wrote some custom
stuff for the SSD1306 screens when working on this project. Additional info about the display can be found there.
- My [I2C libraries for attiny85-like AVR micros](https://hackaday.io/project/183814-i2c-libraries-for-attiny85-like-avr-micros) project page on hackaday.io;
I also wrote some custom libraries for __I2C__ operations on attiny85's and similar AVR micros. Additional info can be found there.
