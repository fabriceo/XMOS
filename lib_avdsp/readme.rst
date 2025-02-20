**LIB_AVDSP**

This library provides a light but flexible solution to create audio DSP processes.
They can be included as raw code in the application software, or a user can upload
a specific program dynamically into the xmos memory.

a library of inline functions gives the possibility to create audio pipeline easily with 64bit accumulators.

the library support the possibility to use dynamically different "dsp programs" and to update the DSP code according to sampling rate.
the generic function load, store, gain, delay, biquads are available and easy to integrate.
a DSP program can use multiple cores, with dynamic allocation (eg depending on sampling rate)

The library is devlopped for an easy integration with XMOS usb audio software (version 6.15.2 or later) but can be used independently.
initializing and modifying DSP parameters like delays or biquads is done using XC interfaces.
triggering the DSP engine at each sample is done via a specific "channed".
The DSP program interract with the usb audio application using an array of samples in shared memory.
a mechanism of virtual switch is implemented to secure array access in sync with channed trigger.

**structure**
*lavdsp.h* is the main header containing the xc interfaces definition
*lavdsp.xc* is the core application managing the task creation/deletion,
 synchronizing the DSP cores together and with the usb audio task via the chanend,
 and handling the requests from the main application send trough the XC interfaces.
*lavdsp_conf.h* is a specific file containing user custom application settings.
*lavdsp_base.h* contains the library or component to be used to create a DSP pipelines.
the *example* folder contains *lavdsp_user.c* which is a typical DSP program showcase. it includes 2 programs, 
one to provide stereo tone control on a stereo channel,
another one to provide crossover and delay capabilities in a multichannel environement.
