# MTS ESP

Use the official library ([GitHub](https://github.com/ODDSound/MTS-ESP)) for C/C++ and mtsespy ([GitHub](https://github.com/narenratan/mtsespy)) for Python.

## Client

Steps for using the MTS-ESP client API to add microtuning support to a plug-in.
Steps 1 and 2 are required, however it is recommended to include further steps when
integrating:


### 1. REQUIRED:
Register and de-register a plug-in instance as a client with MTS-ESP.
On startup in the plug-in constructor call:

```c++
MTSClient *client = MTS_RegisterClient();
```

Store the returned MTSClient pointer to supply when calling other MTS-ESP client API
functions. On shutdown in the plug-in destructor call:

```c++
MTS_DeregisterClient(client);
```

### 2. REQUIRED
Query retuning when a note-on message is received and adjust tuning accordingly.
When given a note call:

```c++
double freq = MTS_NoteToFrequency(client, midinote, midichannel);
```
OR
```c++
double retune_semitones = MTS_RetuningInSemitones(client, midinote, midichannel);
```
OR
```c++
double retune_ratio = MTS_RetuningAsRatio(client, midinote, midichannel);
```

MIDI channel arguments should use the range [0,15] however if you don’t know the MIDI
channel, use -1 (see step 6 for more on MIDI channels).


### 3. RECOMMENDED
Continuously query retuning whilst a note is held, allowing tuning to change
along the flight of a note. Do this if you can and as often as possible, ideally at the same
time as processing any other pitch modulation sources (envelopes, MIDI controllers, LFOs etc.).


### 4. RECOMMENDED
Provide an option to the user to select whether tuning is queried at note-on
only, as in step 2, or continuously, as in step 3. There are creative and practical
advantages to both, depending on the use case, and offering an option to the user will
provide the most useful MTS-ESP integration. If not offering such an option, continuous
retuning should be preferred over note-on only retuning.


### 5. RECOMMENDED
Query whether a note should be sounded when a note-on message is received.
The Scala .kbm keyboard mapping format allows for MIDI keys to be unmapped i.e. no frequency
is specified for them, and the MTS-ESP library supports this too. You can query whether a note
is unmapped and should be ignored with:

```c++
bool should_ignore_note = MTS_ShouldFilterNote(client, midinote, midichannel);
```

If this returns true, ignore the note-on and don’t play anything. Calling this function is
recommended but optional and a valid value for frequency/retuning will be returned for an
unmapped note. MIDI channel arguments should use the range [0,15] however if you don’t
know the MIDI channel, use -1.


### 6. RECOMMENDED
Always supply a MIDI channel when querying retuning or note filtering. Doing
so allows your plug-in to use multi-channel tuning tables, useful for microtonal MIDI controllers
with more than 128 keys or working with large scales. Even if multi-channel tables are not
in use, a master may still make use of channel-specific note filtering for functions such as
key switches to change tunings. If your plug-in supports MPE and has a switch for enabling MPE
support, it is recommended to NOT supply a MIDI channel if MPE is enabled.


### 7. RECOMMENDED
If you are adding MTS-ESP support to a plug-in that already has some kind
of microtuning support, e.g. loading .scl or .tun files, let the local tuning automatically
override MTS-ESP, or provide an option for MTS-ESP retuning to be explicitly disabled.
This affords a user the option to use a different tuning to the global MTS-ESP table
for a specific plug-in instance.


### 8. OPTIONAL
Add support for MIDI Tuning Standard (or MTS, from the MIDI specification) SysEx messages to your plug-in.
When not connected to an MTS-ESP master plug-in, these can be used
to retune it instead, providing microtuning support even when MTS-ESP is not in use.
When a SysEx message is received, call:

```c++
MTS_ParseMIDIData(client, buffer, len); // if buffer is signed char *
```
OR
```c++
MTS_ParseMIDIDataU(client, buffer, len); // if buffer is unsigned char *
```

These will update a local tuning table which is used when querying retuning as in steps 2
and 3. Check whether a valid MTS SysEx message has been received with:

```c++
bool MTS_SysEx_received = MTS_HasReceivedMTSSysEx(client);
```

### 9. OPTIONAL
If you want to display to the user whether the plug-in is "connected" to an
MTS-ESP master plug-in, call:

```c++
bool has_master = MTS_HasMaster(client);
```

### 10: OPTIONAL
It is possible to query the name of the current scale.
This function is necessarily
supplied for the case where a client is sending MTS SysEx messages, however it can be used
to display the current scale name to the user on your UI too:

```c++
const char *name = MTS_GetScaleName(client);
```

### 11: OPTIONAL
After registering, let the user know if they have an older version of the libMTS dynamic library
installed which may not support some features in this version of the API:

```c++
bool should_update = MTS_Client_ShouldUpdateLibrary(client);
```

The latest version of libMTS will always be backward compatible with clients built with
an older version of the API. Users can update libMTS using the installers at
https://github.com/ODDSound/MTS-ESP/tree/main/libMTS.


### 12: EXTRAS
Helper functions are available which return the MIDI note whose pitch is nearest
a given frequency. The MIDI note returned is guaranteed to be mapped. If you intend to
generate a note-on message using the returned note number, you may already know which MIDI
channel it will be sent on, in which case you must specify this in the call, else the client
library can prescribe a channel for you. This is done so that multi-channel mapping
and note filtering can be respected. See below for further details.

```c++
// Opaque datatype for MTSClient.
typedef struct MTSClient MTSClient;

// Register/deregister as a client. Call from the plug-in constructor and destructor.
extern MTSClient *MTS_RegisterClient();
extern void MTS_DeregisterClient(MTSClient *client);

// Check if the client is currently connected to a master plug-in.
extern bool MTS_HasMaster(MTSClient *client);

// Check if the MTS-ESP dynamic library needs to be updated to use all features in this version of the API.
extern bool MTS_Client_ShouldUpdateLibrary(MTSClient *client);

// Returns true if note should not be played. MIDI channel argument should be included if possible (0-15), else set to -1.
extern bool MTS_ShouldFilterNote(MTSClient *client, char midinote, signed char midichannel);

// Retuning a midi note. Pick the version that makes your life easiest! MIDI channel argument should be included if possible (0-15), else set to -1.
extern double MTS_NoteToFrequency(MTSClient *client, char midinote, signed char midichannel);
extern double MTS_RetuningInSemitones(MTSClient *client, char midinote, signed char midichannel);
extern double MTS_RetuningAsRatio(MTSClient *client, char midinote, signed char midichannel);

// MTS_FrequencyToNote() is a helper function returning the note number whose pitch is closest to the supplied frequency. Two versions are provided:
// The first is for the simplest case: supply a frequency and get a note number back.
// If you intend to use the returned note number to generate a note-on message on a specific, pre-determined MIDI channel, set the midichannel argument to the destination channel (0-15), else set to -1.
// If a MIDI channel is supplied, the corresponding multi-channel tuning table will be queried if in use, else multi-channel tables are ignored.
extern char MTS_FrequencyToNote(MTSClient *client, double freq, signed char midichannel);
// Use the second version if you intend to use the returned note number to generate a note-on message and where you have the possibility to send it on any MIDI channel.
// The midichannel argument is a pointer to a char which will receive the MIDI channel on which the note message should be sent (0-15).
// Multi-channel tuning tables are queried if in use.
extern char MTS_FrequencyToNoteAndChannel(MTSClient *client, double freq, signed char *midichannel);

// Returns the name of the current scale.
extern const char *MTS_GetScaleName(MTSClient *client);

// Returns the period of the current scale, or 2.0 (12 semitones) if not supplied by a master.
extern double MTS_GetPeriodRatio(MTSClient *client);
extern double MTS_GetPeriodSemitones(MTSClient *client);

// Query information about keyboard mapping.
// NOTE: negative values are invalid and these functions will return -1 if the information has not been supplied by a master.
// The return value must therefore be checked it is valid before being used.
extern signed char MTS_GetMapSize(MTSClient *client);
extern signed char MTS_GetMapStartKey(MTSClient *client);
extern signed char MTS_GetRefKey(MTSClient *client);

// Parse incoming MIDI data to update local tuning. All formats of MTS SysEx message accepted.
extern void MTS_ParseMIDIDataU(MTSClient *client, const unsigned char *buffer, int len);
extern void MTS_ParseMIDIData(MTSClient *client, const signed char *buffer, int len);

// Check if the client has received any valid MTS SysEx messages and will use local tuning if not connected to a master plug-in.
extern bool MTS_HasReceivedMTSSysEx(MTSClient *client);
```

## Master


MTS Master interface - for creating MTS Master plugins, one per session, which will control
tuning for all MTS-ESP-compatible plugins in the session.

On startup in the constructor:

```c++
MTS_RegisterMaster();
```


On shutdown in the destructor:

```c++
MTS_DeregisterMaster();
```


To determine whether the user has already instanced a master (don’t instance if this returns false) call:

```c++
bool can_register_master = MTS_CanRegisterMaster();
```

To configure the tunings for the entire session, call:

```c++
double frequencies_in_hz[128]; // Fill this in
MTS_SetNoteTunings(frequencies_in_hz);
OR
MTS_SetNoteTuning(frequency_in_hz, midinote);
```

To tell clients to ignore a note, call:

```c++
MTS_FilterNote(should_ignore, midinote, midichannel);
```

Supply -1 for the midichannel argument if the note should be ignored on all MIDI channels.
Note that it is optional for a client to provide a MIDI channel when querying whether a note
should be filtered. If not provided, a client will filter any notes set via this function
regardless of MIDI channel. Although encouraged, it is also optional for a client to check note
filtering at all, so it is suggested to provide a frequency for all MIDI notes, even unmapped ones.
Ideally unmapped notes should use the frequency of the next lowest mapped note, or next highest
if there is none lower.


To reset the ignored notes, call:

```c++
MTS_ClearNoteFilter();
```


To tell the user how many clients you can see connected, call:

```c++
int num_connected = MTS_GetNumClients();
```


To tell clients the scale name, call:

```c++
MTS_ScaleName(“Scale name”);
```


Some MIDI controllers for microtonal work have more than 128 keys where the same note number
may be mapped to different frequencies across different MIDI channels. To support these, use:

```c++
MTS_SetMultiChannel(is_part_of_the_multichannel_system, midichannel);
```

And then:

```c++
MTS_SetMultiChannelNoteTunings, MTS_SetMultiChannelNoteTuning, MTS_FilterNoteMultiChannel, MTS_ClearNoteFilterMultiChannel
```

Which are as above, but take the MIDI channel as an extra parameter. Multi-channel support will only
work with clients that provide a MIDI channel when querying both note filtering and retuning.
Clients that don't provide a MIDI channel will use the frequencies and note filtering provided using
the non-multi-channel functions, therefore it is advised to always provide a general tuning table in addition
to a multi-channel one.


After registering, you may wish to let the user know if they have an older version of the libMTS dynamic library
installed which may not support some features in this version of the API:

```c++
bool should_update = MTS_Master_ShouldUpdateLibrary();
```

The latest version of libMTS will always be backward compatible with masters built with
an older version of the API. Users can update libMTS using the installers at
https://github.com/ODDSound/MTS-ESP/tree/main/libMTS.


IPC support:

MTS_HasIPC() allows you to check if the process in which the plug-in is running is using IPC for sharing MTS-ESP
tuning data. If a DAW crashes, MTS_DeregisterMaster() may not get called. If this happens when using IPC the
shared memory will persist and MTS_HasMaster() will still return true, even if no other master is instanced.
To allow for this case we have included the MTS_Reinitialize() function which will reset the MTS-ESP library,
including tuning tables, scale name, note filters, client count and master connection status.

IMPORTANT: ONLY if MTS_CanRegisterMaster() returns false and MTS_HasIPC() returns true is it advisable to offer an option to
the user to reinitialize MTS-ESP. Follow reinitialization with a call to MTS_RegisterMaster(). The code for registering
as a master should follow this pattern:

```c++
if (MTS_CanRegisterMaster())
    MTS_RegisterMaster();
else
{
    if (MTS_HasIPC())
    {
        Warn user another master is already connected, but provide an option to reinitialize MTS-ESP in case there was a crash and no master is connected any more;
        if (user clicks to reinitialize MTS-ESP)
        {
            MTS_Reinitialize();
            MTS_RegisterMaster();
        }
    }
    else
        Warn user another master is already connected, do not provide an option to reinitialize MTS-ESP;
}
```


```c++
// Register/deregister as a master. Call from the plugin constructor and destructor.
extern void MTS_RegisterMaster();
extern void MTS_DeregisterMaster();

// Check if a master plugin is already instanced before registering, as only one Master may be registered at any one time.
// Don't call MTS_RegisterMaster() if this returns false.
extern bool MTS_CanRegisterMaster();

// Check if the process in which the master plug-in is running is using IPC for sharing MTS-ESP tuning data.
extern bool MTS_HasIPC();

// Reset everything in the MTS-ESP library, including the master connection status and client count.
// IMPORTANT: This is only intended to be called if IPC is in use and only after the process in which the master
// plug-in is running crashes.
extern void MTS_Reinitialize();

// Check if the MTS-ESP dynamic library needs to be updated to use all features in this version of the API.
extern bool MTS_Master_ShouldUpdateLibrary();

// Returns the number of connected clients.
extern int MTS_GetNumClients();

// Set frequencies for 128 MIDI notes.
extern void MTS_SetNoteTunings(const double *freqs);
extern void MTS_SetNoteTuning(double freq, char midinote);

// Set a scale name, so it can be displayed in clients or included in MTS sysex messages sent by a client.
extern void MTS_SetScaleName(const char *name);

// Set the period ratio of the scale, e.g for a period of an octave supply 2.0.
extern void MTS_SetPeriodRatio(double periodRatio);

// Supply information about how a scale is mapped to the MIDI note range.
//  - Map size: the number of notes between each repetition of the scale pattern.
//  - Map start key: the note on which the map starts. This could technically be any key at which the scale pattern repeats but would typically be the one below but nearest to the ref key.
//  - Ref key: the note for which a frequency has been explicitly defined and from which the frequency of all other notes is established.
// Negative values are invalid. If you do not wish to supply this information, or it is not relevant for the current tuning, set -1 as the argument. 
// This will indicate to client plug-ins that they cannot use the queried values.
// Using these functions is optional and if you don't call them at all clients will receive values of -1 by default.
extern void MTS_SetMapSize(signed char size);
extern void MTS_SetMapStartKey(signed char key);
extern void MTS_SetRefKey(signed char key);

// Instruct clients to filter midi notes e.g. because they are not mapped to any scale steps.
// MIDI channel argument is optional, filtering will apply to all channels if not provided.
// Range for midichannel argument is 0-15, or -1 for all channels.
extern void MTS_FilterNote(bool doFilter, char midinote, signed char midichannel);
extern void MTS_ClearNoteFilter();

//-------------------------------------------------------------------------------------------------------

// Optional set of functions for mutli-channel tuning table.
// Range for midichannel arguments is 0-15.

// Set whether a specific MIDI channel is included in the multi-channel tuning table.
extern void MTS_SetMultiChannel(bool set, signed char midichannel);

// Set frequencies for 128 MIDI notes on a specific MIDI channel.
extern void MTS_SetMultiChannelNoteTunings(const double *freqs, signed char midichannel);
extern void MTS_SetMultiChannelNoteTuning(double freq, char midinote, signed char midichannel);

// Instruct clients to filter midi notes on a specific MIDI channel.
extern void MTS_FilterNoteMultiChannel(bool doFilter, char midinote, signed char midichannel);
extern void MTS_ClearNoteFilterMultiChannel(signed char midichannel);
```

## mtsespy
Python bindings for the [ODDSound MTS-ESP library](https://oddsound.com/devs.php).

### Installation

To install from PyPI:
```console
$ pip install mtsespy
```
or to clone the repo and install from source:
```console
$ git clone --recurse-submodules https://github.com/narenratan/mtsespy.git
$ cd mtsespy
$ python3 -m pip install .
```

Using MTS-ESP requires the libMTS dynamic library which is available in the
ODDSound MTS-ESP repo
[here](https://github.com/ODDSound/MTS-ESP/tree/main/libMTS/). The places to
put it for each OS are given in the MTS-ESP README
[here](https://github.com/oddsound/mts-esp?tab=readme-ov-file#libmts).

### Examples

Set tuning of midi note 69 to frequency 441 Hz
```python
import signal

import mtsespy as mts

with mts.Master():
    mts.set_note_tuning(441.0, 69)
    signal.pause()
```

Pull frequency of midi note 69 on midi channel 0
```python
import mtsespy as mts

with mts.Client() as c:
    f = mts.note_to_frequency(c, 69, 0)
```

The `Master` and `Client` context managers, used above, handle registering
and deregistering the MTS-ESP master and client.

### Wrapper names

The function names in the MTS-ESP C++ library and this Python wrapper
correspond as follows

#### Master functions

|   C++                             |   Python                          |
| --------------------------------- | --------------------------------- |
|   MTS_RegisterMaster              |   register_master                 |
|   MTS_DeregisterMaster            |   deregister_master               |
|   MTS_HasIPC                      |   has_ipc                         |
|   MTS_Reinitialize                |   reinitialize                    |
|   MTS_Master_ShouldUpdateLibrary  |   master_should_update_library    |
|   MTS_GetNumClients               |   get_num_clients                 |
|   MTS_SetNoteTunings              |   set_note_tunings                |
|   MTS_SetNoteTuning               |   set_note_tuning                 |
|   MTS_SetScaleName                |   set_scale_name                  |
|   MTS_SetPeriodRatio              |   set_period_ratio                |
|   MTS_SetMapSize                  |   set_map_size                    |
|   MTS_SetMapStartKey              |   set_map_start_key               |
|   MTS_SetRefKey                   |   set_ref_key                     |
|   MTS_FilterNote                  |   filter_note                     |
|   MTS_ClearNoteFilter             |   clear_note_filter               |
|   MTS_SetMultiChannel             |   set_multi_channel               |
|   MTS_SetMultiChannelNoteTunings  |   set_multi_channel_note_tunings  |
|   MTS_SetMultiChannelNoteTuning   |   set_multi_channel_note_tuning   |
|   MTS_FilterNoteMultiChannel      |   filter_note_multi_channel       |
|   MTS_ClearNoteFilterMultiChannel |   clear_note_filter_multi_channel |

### Client functions

|   C++                             |   Python                          |
| --------------------------------- | --------------------------------- |
|   MTS_RegisterClient              |   register_client                 |
|   MTS_DeregisterClient            |   deregister_client               |
|   MTS_HasMaster                   |   has_master                      |
|   MTS_Client_ShouldUpdateLibrary  |   client_should_update_library    |
|   MTS_ShouldFilterNote            |   should_filter_note              |
|   MTS_NoteToFrequency             |   note_to_frequency               |
|   MTS_RetuningInSemitones         |   retuning_in_semitones           |
|   MTS_RetuningAsRatio             |   retuning_as_ratio               |
|   MTS_FrequencyToNote             |   frequency_to_note               |
|   MTS_FrequencyToNoteAndChannel   |   frequency_to_note_and_channel   |
|   MTS_GetScaleName                |   get_scale_name                  |
|   MTS_GetPeriodRatio              |   get_period_ratio                |
|   MTS_GetPeriodSemitones          |   get_period_semitones            |
|   MTS_GetMapSize                  |   get_map_size                    |
|   MTS_GetMapStartKey              |   get_map_start_key               |
|   MTS_GetRefKey                   |   get_ref_key                     |
|   MTS_ParseMIDIDataU              |   -                               |
|   MTS_ParseMIDIData               |   parse_midi_data                 |
|   MTS_HasReceivedMTSSysEx         |   has_received_mts_sysex          |