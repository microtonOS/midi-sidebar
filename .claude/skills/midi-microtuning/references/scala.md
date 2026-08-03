# Scala
 
Scala is one of the most popular tuning file formats.
The tuning is split up into a scale file (`.scl`) and one or several keyboard mapping files ( `.kbm`).
Apart from the [Huygens–Fokker Foundation](https://www.huygens-fokker.org/scala/), the Surge Synthesizer Team has a useful [guide](https://surge-synthesizer.github.io/tuning-guide/) and a library ([GitHub](https://github.com/surge-synthesizer/tuning-library)) for handling `.scl`and `.kbm` files.
 
## [`.scl`](https://www.huygens-fokker.org/scala/scl_format.html)
- The files are human readable ASCII or 8-bit character text-files. 1)
- The file type is `.scl` .
- There is one scale per file.
- Lines beginning with an exclamation mark are regarded as comments and are to be ignored.
- The first (non comment) line contains a short description of the scale, but long lines are possible and should not give a read error. The description is only one line. If there is no description, there should be an empty line.
- The second line contains the number of notes. This number indicates the number of lines with pitch values that follow. In principle there is no upper limit to this, but it is allowed to reject files exceeding a certain size. The lower limit is 0, which is possible since degree 0 of 1/1 is implicit. Spaces before or after the number are allowed.
- After that come the pitch values, each on a separate line, either as a ratio or as a value in cents. If the value contains a period, it is a cents value, otherwise a ratio. Ratios are written with a slash, and only one. Integer values with no period or slash should be regarded as such, for example "2" should be taken as "2/1". Numerators and denominators should be supported to at least 2^31 - 1 = 2147483647. Anything after a valid pitch value should be ignored. Space or horizontal tab characters are allowed and should be ignored. Negative ratios are meaningless and should give a read error. For a description of cents, go here.
- The first note of 1/1 or 0.0 cents is implicit and not in the files.
- Files for which Scala gives Error in file format are incorrectly formatted. They should give a read error and be rejected.
So these lines are all valid pitch lines:
 
```
81/64
408.0
408.
5
-5.0
10/20
100.0 cents
 100.0 C#
 5/4   E\
```
 
Here is an example of a valid file:
 
```
! meanquar.scl
!
1/4-comma meantone scale. Pietro Aaron's temperament (1523)
 12
!
 76.04900
 193.15686
 310.26471
 5/4
 503.42157
 579.47057
 696.57843
 25/16
 889.73529
 1006.84314
 1082.89214
 2/1
```
 
A simple advise: put the filename on the first line behind an exclamation mark. Then someone receiving the file and reading it knows a name under which to save it.
 
## [`.kbm`](https://www.huygens-fokker.org/scala/help.htm#mappings)
 
Keyboard mappings determine the allocation of scale degrees to keys on a MIDI keyboard, or MIDI note numbers in general. They are automatically used when you tune a synthesizer with the SEND command, retune a MIDI file with the EXAMPLE/MIDI command or create a sequence file with the EXAMPLE/CREATE command. They can also be created by using an external text editor, and the file type should be `.kbm`. They are activated by the command LOAD/MAPPING. An example template file is `example.kbm`. It contains various parameters on the first few lines and then the mapping defined by scale degrees for consecutive keys. For instance if it is 0, 1, 2, 3, etc., then it will be an ordinary linear mapping. 0, 2, 3 would mean degree 0 will be on the first key (which is the given middle note), degree 2 will be on the second key, degree 3 on the third, etc. Scale degrees may be assigned to more than one key or to no key at all. This is useful for octave-based scales with less than 12 notes so gaps in the key row can be filled with duplicate notes. With scales containing notes that are alternatives for each other, unused alternatives can be left unmapped. When not all scale degrees need to be mapped, the size of the map can be smaller than the size of the scale. Otherwise it would of course need to be at least the same size as the scale, or zero for a linear mapping.
With nonstandard keyboards, mappings can be made that do not repeat, so all keys can be assigned individually, see example file `128.kbm`. If a certain key is not to be tuned, an "x" must be placed instead of a number. If this is done with the frequency reference note it will be considered an error. The user should be aware that what happens when such mapping is in an instrument or other software is not defined, for example it could change the reference frequency or produce an error.
There is no restriction to the degree numbers in the mapping or to the scale degree to consider as formal octave, they can be any number, also negative, also lie outside the scale range. It means pitches are always calculated based on octave extension. See Scales. If you want a mapping for a double octave range, which is the case if the mapping is different in the next octave for example, then make the scale degree to consider as formal octave parameter twice the size of the scale. If an instrument cannot produce the frequency mapped to a key, it can unmap the key or do whatever to avoid unwanted side effects.
See also SHOW MAPPING, CLEAR/MAPPING, DIRECTORY/MAPPING, KEY/MAPPING and SELECT/MAPPING. This is an example mapping:
 
```
! Template for a keyboard mapping
!
! Size of map. The pattern repeats every so many keys:
12
! First MIDI note number to retune:
0
! Last MIDI note number to retune:
127
! Middle note where the first entry of the mapping is mapped to:
60
! Reference note for which frequency is given:
69
! Frequency to tune the above note to (floating point e.g. 440.0):
440.0
! Scale degree to consider as formal octave (determines difference in pitch
! between adjacent mapping patterns):
12
! Mapping.
! The numbers represent scale degrees mapped to keys. The first entry is for
! the given middle note, the next for subsequent higher keys.
! For an unmapped key, put in an "x". At the end, unmapped keys may be left out.
0
1
2
3
4
5
6
7
8
9
10
11
```
 
In Scala version 2.0 and higher, mappings can also be created and edited using the Edit:Edit mapping menu. The middle MIDI note (see SET MIDDLE) and the reference pitch (see SET MAP_FREQ) can also be set in the Edit:Options dialog in the MIDI tab. Furthermore the mapping is also used for real-time MIDI relaying (Tools:Microtuning MIDI Relay). There there are two ways to use them. Normally, with a single mapping to be used for one input channel or all (Omni). Or, with a multichannel mapping which consists of a set of single mapping files with the same name followed by an underscore and a MIDI channel number from 1 .. 16. For example, if `map_1.kbm`, `map_2.kbm` and `map_3.kbm` are present in the same directory as the filename given (either one of those) then they function independently for MIDI input channels 1, 2 and 3 and messages from other channels will not be mapped, therefore ignored. This is useful if you stack multiple keyboards or have a microtonal keyboard which uses multiple channels to overcome the 128 note number limitation.
 
Note that as of 31 July 2026 the Surge Synthesizer Team's library does not implement multichannel support for .kbm files.