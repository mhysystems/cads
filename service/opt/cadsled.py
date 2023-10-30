#!/usr/bin/env python3
import Jetson.GPIO as GPIO
import argparse
import sys
import socket

cnt = 5

def wireguard(up):
    GPIO.output(21, GPIO.HIGH if up else GPIO.LOW)

def up():
    global cnt
    cnt = 5
    wireguard(True)

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

GPIO.setwarnings(False)
GPIO.setmode(GPIO.BCM)
GPIO.setup(21, GPIO.OUT)

if args.wireguard is not None:
    wireguard(args.wireguard == 0)
else:
    while True:
        try:
            sysd = socket.socket(fileno=sys.stdin.fileno())

            while True:
                (conn,address) = sysd.accept()
                i = conn.recv(8).decode('utf-8').strip()
                up() if '0' == i else down()
                sys.stdout.flush()
        except Exception as e:
            print(e) 
        

GPIO.cleanup(21)
