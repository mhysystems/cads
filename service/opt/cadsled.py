#!/usr/bin/env python3
import Jetson.GPIO as GPIO
import argparse
import sys
import socket

cnt = 5

def wireguard(up):
    GPIO.setwarnings(False)
    GPIO.setmode(GPIO.BCM)
    GPIO.setup(21, GPIO.OUT)
    GPIO.output(21, GPIO.HIGH if up else GPIO.LOW)
    if not up:
        GPIO.cleanup(21)

def up():
    global cnt
    cnt = 5
    wireguard(True)
#    GPIO.output(21, GPIO.HIGH )

def down():
    global cnt
    if cnt < 0:
        print("cannot ping 10.9.1.1")
        wireguard(False)
    else:
        cnt = cnt - 1
    
    sys.stdout.flush()
#    GPIO.output(21, GPIO.LOW)

parser = argparse.ArgumentParser(description='Control Cads LEDs')
parser.add_argument("--wireguard", type=int, help="Wireguard Connection")


args = parser.parse_args()

if args.wireguard is not None:
    wireguard(args.wireguard == 0)
else:

    sysd = socket.socket(fileno=sys.stdin.fileno())

    while True:
        (conn,address) = sysd.accept()
        i = conn.recv(8).decode('utf-8').strip()
        up() if '0' == i else down()

        

