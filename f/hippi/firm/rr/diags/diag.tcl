# 
# Procedures to run host-based SRAM tests multiple times. These
# are built into the rrdbg which tests through PIO reads/writes.
# rrdbg does not do any register config as part of the "sram"
# test, so if you want to change any configs, you should do
# so explicitly using the "set <reg> <value>" rrdbg command
# before running the sram test.
#
proc s10 {} { foreach k { 1 2 3 4 5 6 7 8 9 10 } { sram } }
proc s100 {} { foreach l { 1 2 3 4 5 6 7 8 9 10 } { s10 } }
proc s1k {} { foreach m { 1 2 3 4 5 6 7 8 9 10 } { s100 } }


################  RR-based walking_1 test #############################
#
# Tcl script to put RR functions on hold, before we download diag fw.
# 
# set config regs to sane values to halt RR activity, then 
# downloads walking_1 diag.

proc w1 { delayline } {
# reset
set hc 8

# clear PERR bits, enable parchk, enable eeprom access
set lc 0xd402

# set DelayLineReg value
set 0xdc $delayline

# put receive in reset state
set hrs 0xff800002

# put transmit in reset state
set hts 0x2

# disable DMA assist
set 0x15c 0

# reset write DMA
set wstat 1

# reset read DMA
set rstat 1

# CPU pri low to match operational fw conditions
set 0xcc  0

d c

dload walk1.fw

exec sleep 3

d c

d regs
}

##### do the above multiple times
proc w10 {delay} { foreach i { 1 2 3 4 5 6 7 8 9 10 } { w1 $delay } }
proc w100 {delay} { foreach j { 1 2 3 4 5 6 7 8 9 10 } { w10 $delay } }

############################## DEQA ################################
## Data EQuals Address test, deqa.fw is a RR-based diag which writes
## to each word of SRAM (except low-mem where its instructions lie)
## the address of the word and reads it back.

proc deqa { delayline } {
# reset
set hc 8

# clear PERR bits, enable parchk, enable eeprom access
set lc 0xd402

# set DelayLineReg value
set 0xdc $delayline

# put receive in reset state
set hrs 0xff800002

# put transmit in reset state
set hts 0x2

# disable DMA assist
set 0x15c 0

# reset write DMA
set wstat 1

# reset read DMA
set rstat 1

# CPU pri low to match operational fw conditions
set 0xcc  0

d c

dload deqa.fw

exec sleep 3

d c

d regs
}

proc deqa10 {delay} { foreach i { 1 2 3 4 5 6 7 8 9 10 } { deqa $delay } }
proc deqa100 {delay} { foreach j { 1 2 3 4 5 6 7 8 9 10 } { deqa $delay } }

############################## DEQNA ################################
## Data EQuals Not Address test, deqna.fw is a RR-based diag which writes
## to each word of SRAM (except low-mem where its instructions lie)
## the ~address of the word and reads it back.

proc deqna { delayline } {
# reset
set hc 8

# clear PERR bits, enable parchk, enable eeprom access
set lc 0xd402

# set DelayLineReg value
set 0xdc $delayline

# put receive in reset state
set hrs 0xff800002

# put transmit in reset state
set hts 0x2

# disable DMA assist
set 0x15c 0

# reset write DMA
set wstat 1

# reset read DMA
set rstat 1

# CPU pri low to match operational fw conditions
set 0xcc  0

d c

dload deqna.fw

exec sleep 3

d c

d regs
}

proc deqna10 {delay} { foreach i { 1 2 3 4 5 6 7 8 9 10 } { deqna $delay } }
proc deqna100 {delay} { foreach j { 1 2 3 4 5 6 7 8 9 10 } { deqna $delay } }


