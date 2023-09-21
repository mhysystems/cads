import Jetson.GPIO as GPIO
import argparse

def wireguard(up):
    GPIO.setmode(GPIO.BCM)
    GPIO.setup(18, GPIO.OUT)
    GPIO.output(GPIO_PIN, GPIO.HIGH if up else GPIO.LOW)

parser = argparse.ArgumentParser(description='Control Cads LEDs')
parser.add_argument("--wireguard", type=int, help="Wireguard Connection") 

args = parser.parse_args()

if args.wireguard is not None:
    wireguard(args.wireguard == 0)
