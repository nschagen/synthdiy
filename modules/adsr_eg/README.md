# Some notes

Here I try to describe how the circuit works. I do this based on my own limited
knowledge so I might be wrong about things. I collected this information to
help me debug the circuit.

The ADSR circuit revolves around charging and discharging the main capacitor.
There are 3 (dis)charge paths connected to the capacitor. These paths all use
diodes to ensure the capacitor can either charge or discharge via that path.

* Attack: Connects pin 3 (output) of the 555 via a diode to the capacitor to charge it
* Decay / Sustain: Creates a reference voltage using the sustain know. The
  decay knob allows the capacitor to discharge until it hits the ref voltage.
* Release: Connects the (pre-processed) gate signal to the capacitor via a
  diode. When the gate goes low, there will be a voltage across the diode
  allowing the capacitor to discharge.

The 555 is used to control the transition from the attack phase into the
decay/sustain phase as soon as the capacitor is sufficiently charged.

Initially, the capacitor is empty. When the input gate goes high, there is a
gate-to-trigger converter circuit, which will cause the voltage on pin 2 of the
555 to briefly go down. This will 'set' the flipflop in the 555 and the
capacitor will be charged from the pin 3 (output) via the Attack potentiometer
and diode.

This will continue until the voltage across the capacitor reaches 2/3 of the
supply voltage (8V in this case). In that case, the voltage on pin 6
(threshold) will exceed 8 volts, causing the flipflop to be reset. This in
turn, causes pin 3 (output) to go low again and stops the charging process.

Now that the flip-flop of the 555 is low, pin 7 (discharge) pin will be
connected to ground. This causes the 4.7k resistor and the sustain
potentiometer to form a voltage divider that sets the sustain voltage which is
lower than the voltage across the capacitor.

The capacitor will now discharge until the voltage across it equals the sustain
voltage. 

At this point, the gate is still high. When it goes low, the capacitor can also
discharge via the release potentiometer and the open transistor to ground.

In my circuit, I added an opamp inbetween the sustain pot and the decay pot to
make sure these two potentiometers do not influence eachother. After quite some
experiments, I also added a precision rectifier before pin 4 of the 555. I do
not understand exactly how, but this improved the performance of my ADSR.
