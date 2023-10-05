#!/usr/bin/env python3
import Jetson.GPIO as GPIO
import argparse
import signal


def wireguard(up):
    GPIO.setwarnings(False)
    GPIO.setmode(GPIO.BCM)
    GPIO.setup(21, GPIO.OUT)
    GPIO.output(21, GPIO.HIGH if up else GPIO.LOW)
    if not up:
        GPIO.cleanup(21)

def sigusr1(signum,frame):
    wireguard(True)

def sigusr2(signum,frame):
    wireguard(False)


parser = argparse.ArgumentParser(description='Control Cads LEDs')
parser.add_argument("--wireguard", type=int, help="Wireguard Connection") 

args = parser.parse_args()
signal.signal(signal.SIGUSR1, sigusr1)
signal.signal(signal.SIGUSR2, sigusr2)

if args.wireguard is not None:
    wireguard(args.wireguard == 0)
else:
    while True:
        signal.pause()

