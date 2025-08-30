# APU

## Overview
APU is responsible for producing sound

Typically digital signals are produced by taking a sample at every x.



- 4 channels
  - 1,2 square channels, pulse width modulated waves
    
  - 3, wave, 
  - 4
`NRx0`: channel specific,
`NRx1`: length timer
`NRx2`: volume and envelope
`NRx3`: period
`NRx4`: trigger and length time enable bits

where `x` is channel number
- Registers
  `NR52`: Audio Master control
  `NR51`: Sound Panning
  `NR50`: Master Volume and VIN pinning

`enabled`
`sampleRate`: 
`cycleCounter`: tracks number of cycles
