# Front-panels

My front-panels are laser-cut 2mm MDF. I glue two layers together to get a 4mm
thick panel. The reasoning is that 2mm MDF still bends and can break when
applying force trying to connect a cable. On the other hand, the jack sockets I bought
do not have screw thread to go through the 4mm panel, hence the front-layer has
  a small 6mm hole for the socket while the back-panel has a rectangular cutout that
  fits the socket body. That also prevents the sockets from turning over time.
Also note that the small eurorack screws are not long enough to go through both
layers of MDF, which is why the front-panel extends to the top and bottom a bit
more to accomodate the mounting holes. 

Most components like switches and pots can penetrate both layers, hence the
measurements of the holes for these in both layers is the same.

In most module designs, I spaced the potentiometers apart in a multiple of 0.1
inch such that the pins of the pots fit into my stripboard. This can be a bit
tricky since the front-panel designer uses milimeters (And I'm used to those).
This is mainly how I connect the front-panel firmly to the stripboard but
sometimes (when using enough wires), I leave it dangling. I always make sure
that my stripboard does not extend more than 10cm into my case because my case
is not too deep.

In some designs I use standoffs to connect the panel (in parallel) to the PCB.
This works for modules that are a bit wider. The nylon M3 screws that I'm using
to connect the standoffs are not too big so the only penetrate one of the two
MDF panels. I make the hole in the front panel 5mm wide to fit the head of the
screw while the back-panel has a small 3mm hole.

Some measurements I use to design my front-panels:

* Panel Height is 128.4mm
* Potentiomter hole is 6mm
* Jack socket hole diameter is 6mm
* Switch hole diameter is 6mm
* LED hole diameter is 5mm

Some measurements I use for the second layer (support) panels:

* Panel height is 111.6mm
* Potentiomter hole is 6mm
* Jack socket rectanglular cutout is 9.5mm wide and 11mm high
* Switch hole diameter is 6mm
* LED hole diameter is 5mm

Some important notes:

* Vertial spacing between potentiometer-holes (centers) should be at least 20.32mm (0.8inch) 
* Vertial spacing between jack socket holes (center) and the upper/lower edge of the front-panel should be at least 14mm. Otherwise the socket body will hit the eurorack rails and your panel will not fit.

# Design process

To design front-panels, I use the front-panel design software that [can be
found here](https://www.schaeffer-ag.de/en/front-panel-designer). To add text
and other engravings to the panel, I use inkscape.

I first design the front layer of the panel and when I'm done, I save a copy
and do 'edit' -> 'size of the front panel' to scale it down to 111.6mm to create
 the support panel. I replace the jack sockets with square holes.

I export both the front and support panels from the front-panel designer as DXF
(without reference points) and import them into inkscape. I have to change the
line colors to blue (#0000FF) and I can now add lines and text according to the
specifications of the laser cutter (I use snijlab.nl).
