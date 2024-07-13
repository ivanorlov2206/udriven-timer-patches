#!/bin/sh

aplay -D hw:CARD=Loopback,DEV=0 -c 1 -r 8000 -d 4 -f S16_LE --period-size 4410 --buffer-size 16000 sound.wav &
arecord -D hw:CARD=Loopback,DEV=1 -c 1 -r 8000 -d 4 -f S16_LE --period-size 4410 --buffer-size 16000
rmmod snd-aloop
