#!/usr/bin/env python3
import time
import datetime
import serial
import re
import os
import subprocess
import threading
import RPi.GPIO as GPIO

SERIAL_DEVICE = "/dev/ttyACM0"
GPIO_OFFSWITCH_PIN = 11

arduino = serial.Serial(SERIAL_DEVICE)


def convert_value(val):
    res = None
    m = re.search("([A-Z_]+)\\(([0-9]*)\\)", val)
    if m is not None:
        descr = m.group(1)
        val = int(m.group(2))
        res = {descr: val}
    return res

def read_serial_values():
    rawdata = arduino.readline()
    return rawdata.decode("utf-8")

def set_volume(vol: int):
    subprocess.call(["amixer","set", "Headphone", "{:.1f}%".format((vol*100)/1024)])

def get_cpu_temp():
    resp = subprocess.Popen(["vcgencmd", "measure_temp"], stdout=subprocess.PIPE)
    tempstring = str(resp.stdout.read().decode("utf-8"))
    m = re.search("temp=([0-9.]+)'C", tempstring)
    if m is not None:
        return float(m.group(1))
    return None

def serial_listener():
    while True:
        keyVal = convert_value(read_serial_values())
        if keyVal is not None:
            if "VOL" in keyVal:
                set_volume(keyVal["VOL"])

def serial_writer():
    cpu_temp_old = 0.0
    while True:
        cpu_temp = get_cpu_temp()
        if cpu_temp is not None and abs(cpu_temp_old-cpu_temp) > 0.1:
            tempv_array = list("D2Temp {:.1f}'C\n".format(cpu_temp).encode("utf-8"))
            deg_idx = tempv_array.index(39)
            tempv_array[deg_idx]=223
            arduino.write(bytes(tempv_array))
            if cpu_temp > 60.0:
                arduino.write("F160\n".format(cpu_temp).encode("utf-8"))
            elif cpu_temp > 70.0:
                arduino.write("F255\n".encode("utf-8"))
            else:
                arduino.write("F0".encode("utf-8"))
        cpu_temp_old = cpu_temp
        current_time = datetime.datetime.now().strftime("%d-%m-%Y %H:%M:%S")
        arduino.write("D3{}".format(current_time).encode("utf-8")
        time.sleep(0.5)

def turn_off(channel):
    print("switching off")    
    subprocess.call(["shutdown", "-h", "now"], shell=False)

def init_gpio():
    GPIO.setmode(GPIO.BOARD)
    GPIO.setup(GPIO_OFFSWITCH_PIN, GPIO.IN, pull_up_down=GPIO.PUD_UP)
    GPIO.add_event_detect(GPIO_OFFSWITCH_PIN, GPIO.FALLING, callback=turn_off, bouncetime=10000)


if __name__ == "__main__":
    init_gpio()
    t1 = threading.Thread(target=serial_listener)
    t1.start()
    serial_writer()

