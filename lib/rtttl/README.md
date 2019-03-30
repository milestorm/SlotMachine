# Non-blocking playback of RTTTL melodies

RTTTL is Nokia's ***R***ing ***T***one ***T***ext ***T***ransfer ***L***anguage, from when mobile phones could only play a single, squeaky note at a time. People could text each other ringtones like this example...

```BringMeSunshine:d=8,o=5,b=140:c.6,b.,4a.,2e.,c.6,b.,2a,p,c.6,b.,4a,16p,2f.,16p,c.6,b.,2a,p, e.6,d#.6,4d.6,c6,b.,4a#.,16p,g.,g#.,a.,f.,a,e.6,4d.6,16p,c.6,b.,4a.,d.6,c.6,4b.,e.6,e.6,2c.6```

Combine the term "RTTTL" and your favourite old-skool tune in a google search for more examples.

This Arduino library provides non-blocking playback of RTTTL melodies using a Piezoelectric transducer. Non-blocking means it can update the tones being played by an ATMEGA chip (controlled by [hardware timers](https://learn.adafruit.com/multi-tasking-the-arduino-part-2/timers)) to match with a loaded melody, allowing other functions continue to be performed by the microcontroller alongside the melody.

It has been tested on [Shrimp](http://start.shrimping.it/project/shrimp/), and Arduino Uno. Wire it like the Piezo part of our [Alarm Clock build](http://start.shrimping.it/project/alarmclock/build.html#step12) (Shrimp) or [this 'Tone' example](https://www.arduino.cc/en/Tutorial/ToneMelody) (Arduino). [Example sketches](https://github.com/cefn/non-blocking-rtttl-arduino/tree/master/rtttl/examples) showing RTTTL playback are provided. 

If you have followed our [@ShrimpingIt configuration steps](http://start.shrimping.it/project/shrimp/program.html) then the RTTTL library is already installed. Otherwise, follow [these instructions](https://www.arduino.cc/en/Guide/Libraries) to use the library on its own. 

Although based on [ponty's arduino-rttl-player](https://github.com/ponty/arduino-rtttl-player) changing it to non-blocking playback leaves very little left (@ponty's was a blocking library). Thanks to @ponty for all the RTTTL decoding logic, and the stimulus to create this library.

# Key functions:

* **pollSong()** since the hardware handles timer-driven playback, you can continue to service other microcontroller functions between calls to pollSong(), which will only change the tone output if it is time to do so (according to the melody) and returns immediately.
* **stepSong()** this progresses the melody, but also blocks until a single note is finished. It is useful, for example, to write simple code which couples lighting effects to notes.
* **finishSong()** this plays out the whole melody once, blocking until it's finished. It's useful if a tune needs to be completed before the next action should be taken (e.g. playout when a game has ended and before it resets).

***N.B. If you use this non-blocking strategy, ```pollSong()``` should be called regularly alongside the other operations in your sketch to ensure the melody timing is correct.***
