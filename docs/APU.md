# APU

## Overview
The APU is responsible for producing sound. Essentially sound is waves that our bodies are able to interpret and process.
In the real world sound exists as analog signals, in the digital world we take samples at every some interval and use that to construct a digital sound wave. The APU has 4 channels which it samples and mixes to produce output sound.

### Frame Sequencer
The frame sequencer is responsible for creating a clock for the APU. The gameboy master clock runs at 4,193,304 Hz, the APU runs at a clock rate of 512Hz. The master clock is divided by 8192 to achieve this. Essentially we tick our apu every time we tick are cpu, and count to 8192 in the apu. At increments of 8192 ticks we perform some action.

On each apu clock cycle we update our `triggerLength`, `triggerEnvelope`, and `triggerSweep` variables accordingly


| Cycle | Length Counter | Volume Envelope | Sweep |
|-------|----------------|-----------------|-------|
| 0     | True           | -               | -     |
| 1     | -              | -               | -     |
| 2     | True           | -               | True  |
| 3     | -              | -               | -     |
| 4     | True           | -               | -     |
| 5     | -              | -               | -     |
| 6     | True           | -               | True  |
| 7     | -              | True            | -     |


This table indicates when to activate our different audio functions

### Length Timer (Length Timer Action)
The length timer function controls how long a sound will play by enabling and disabling the channel. The function will return true until the channel `lengthTimer` value is less than or equal to 0 at which point it will disable the channel.

### Volume Envelope (Envelope Action)
The envelope action is used to control the waves amplitude. This creates fade in, fade, out, and sustained sound. We read in how fast and which direction to increment or decrement from our `nrx2` register.

APU Registers
- `nr52`: Audio Master control
- `nr51`: Sound Panning
- `nr50`: Master Volume and VIN pinning

## Channels
There are two `square channels`, one `wave channel` and one `noise channel`. Each of the channels share some similar propertries.

Channel Registers
Each channel has 8 bit registers known as `nrx0`,`nrx1`,`nrx2`,`nrx3`,`nrx4` where x is the channel number.
`nrx0`: channel specific register
`nrx1`: length timer
`nrx2`: volume and envelope
`nrx3`: period
`nrx4`: trigger and length timer enable 


Sound generation components
| Channel | Sweep | Frequency | Wave Form	 | Length Timer |Volume|
|----------|----------|----------|----------|----------|----------|
| 1 | Sweep  | Period Counter	  | Duty  | Length Timer	  | Envelope |
| 2  |   | Period Counter	| Duty  | Length Timer	  | Envelope  |
| 3  |    | Period Counter	  | Wave  | Length Timer	 | Envelope  |
| 4  |    | Period Counter	 | LFSR  |Length Timer	 | Envelope |

Each wave has the following generation component, these components determine the shape of the wave.

- Detail sampling rate
Common
  - reset chanel if 7 bit is reset
  - check if we need to trigger duty action
  - trigger duty action if we need to
  - trigger frame sequecer action
  - update whether channel is enabled or not
  - perform envelope action if enabled
  - get sample
  
length counter, when a length counter is reached, channel is disabled, returns sample of 0 until its reenabled

General Flow
1) Check in register `nrx4` if trigger bit is set or not
| Bit 7 | Bit 6 | Bit 5 | Bit 4 | Bit 3 | Bit 2 | Bit 1 | Bit 0 |
|-------|-------|-------|-------|-------|-------|-------|-------|
| Trigger | Length enable | - | - | - | Period (high 3 bits) | | | 
2) If trigger bit is enabled, reset
  - clears trigger bit
  - sets channel to enabled
  - sets envelope volume
  - setup timer length
  - enable envelope
3) Check if timer is trigger by performing timer action
  - subtracks frequency timer until its 0, sets new value and returns true
4) If timer is triggered perform duty action
  - moves to next duty in the table
5) perform frameSequencerAction, basically this syncs with CPU clock, creats a 512Hz clock. cpu runs at 4194304 / 8192 = 512
6) check if its time to trigger based on frameSequencer updates
7)

### Channel 1 and 2
- Rectangular wave with a varying duty cycle
  - duty cycle is the time period wave is active
  - value will be either 1 or 0
  - duty cycle determines how long it will be on or off (so how long it will be at 1 or 0)
  - TODO: list out duty cycles
  - frequency is 0 - 2407