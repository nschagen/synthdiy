# Arduino based MIDI to CV converter

Based on the design by HAGIWO but made a number of tweaks. I was also inspired
by the design of erica synths black MIDI-CV module, which supports two pairs of
CV/Gate and quite a clever configuration mechanism.

I had the 4N35 opto-coupler, which apparently requires an
additional transistor for amplification to correctly work. [See this stackexchange discussion](https://electronics.stackexchange.com/questions/314150/4n35-opto-isolator-for-midi-input-does-not-work)

I use two MCP4922's to get 4 DAC's. 
