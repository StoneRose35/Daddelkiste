#!/usr/bin/env python3
import time
import datetime
import serial
import re
import subprocess
import threading
import os
import RPi.GPIO as GPIO
import daddelkisteCommon

SERIAL_DEVICE = "/dev/ttyACM0"
GPIO_OFFSWITCH_PIN = 11
BAT_CONVERSION = 0.011695888188

fan_speed_calculator = daddelkisteCommon.FanSpeedCalculator()

arduino = serial.Serial(SERIAL_DEVICE)
battery_voltage = 12.
vol_changing = False
next_vol = -1
serial_writer_state = 0 #0: off, 1: winding down, 2: running
serial_reader_state = 0 #0: off, 1: winding down, 2: running


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
    # print(rawdata.decode("utf-8"))
    return rawdata.decode("utf-8")


def set_volume(vol: int):
    global vol_changing
    subprocess.call(
        ["sudo", "-u", '#1000', "XDG_RUNTIME_DIR=/run/user/1000", "pactl", "set-sink-volume", "@DEFAULT_SINK@",
         "{:d}%".format(int((vol * 100.) / 1024.))])
    vol_changing = False


def set_volume_async(vol: int):
    global vol_changing
    vol_changing = True
    tvol = threading.Thread(target=set_volume, args=(vol,))
    tvol.start()


def get_cpu_temp():
    resp = subprocess.Popen(["vcgencmd", "measure_temp"], stdout=subprocess.PIPE)
    tempstring = str(resp.stdout.read().decode("utf-8"))
    m = re.search("temp=([0-9.]+)'C", tempstring)
    if m is not None:
        return float(m.group(1))
    return None


def serial_listener():
    last_vol = 0
    global battery_voltage, vol_changing, next_vol, serial_reader_state
    while serial_reader_state > 0:
        keyVal = convert_value(read_serial_values())
        if keyVal is not None:
            if "VOL" in keyVal:
                next_vol = keyVal["VOL"]
                if vol_changing is False:
                    set_volume_async(next_vol)
                    arduino.write("D1V: {:.1f}%\n".format(float(next_vol) / 10.24).encode("utf-8"))
                if int(keyVal["VOL"]) > 0 and last_vol == 0:
                    arduino.write("A1".encode("utf-8"))
                elif int(keyVal["VOL"]) == 0 and last_vol > 0:
                    arduino.write("A0".encode("utf-8"))
                last_vol = int(keyVal["VOL"])
            elif "BAT" in keyVal:
                battery_voltage = daddelkisteCommon.calculate_battery_voltage(keyVal["BAT"])
                if 0.0 < battery_voltage < 5.0:
                    turn_off(0)
            elif "BUT" in keyVal:
                with open("/opt/retropie/startup.config", "wt") as f:
                    if keyVal["BUT"] == 1: #short
                        f.write("EMULATIONSTATION")
                        arduino.write("D0Game Mode\n".encode("utf-8"))
                    elif keyVal["BUT"] == 2: #long
                        f.write("DESKTOP")
                        arduino.write("D0Desktop Mode\n".encode("utf-8"))
            elif "ERR" in keyVal:
                arduino.write("D1I2C ERR:{}\n".format(keyVal["ERR"]).encode("utf-8"))
        if serial_reader_state == 1:
            serial_reader_state = 0


def serial_writer():
    cpu_temp_old = 0.0
    fan_idx_old = 0
    global battery_voltage
    global serial_writer_state
    while serial_writer_state > 0:
        cpu_temp = get_cpu_temp()
        if cpu_temp is not None and abs(cpu_temp_old - cpu_temp) > 0.1:
            tempv_array = list("D2T: {:.1f}'C B: {:.1f}V\n".format(cpu_temp, battery_voltage).encode("utf-8"))
            deg_idx = tempv_array.index(39)
            tempv_array[deg_idx] = 223
            arduino.write(bytes(tempv_array))
            fan_val = fan_speed_calculator.compute_fan_speed(cpu_temp)
            arduino.write("F{}\n".format(fan_val).encode("utf-8"))
        cpu_temp_old = cpu_temp
        current_time = datetime.datetime.now().strftime("%d-%m-%Y %H:%M:%S")
        arduino.write("D3{}\n".format(current_time).encode("utf-8"))
        arduino.write("S\n".encode("utf-8"))
        if serial_writer_state == 1:
            serial_writer_state = 0
        time.sleep(0.5)


def turn_off(channel):
    global serial_reader_state, serial_writer_state
    arduino.write("D0Daddelkiste stopping\n".encode("utf-8"))
    serial_writer_state = 1
    serial_reader_state = 1
    while serial_writer_state > 0 or serial_reader_state > 0:
        pass
    subprocess.call(["sudo", "shutdown", "-h", "now"], shell=False)


def init_gpio():
    GPIO.setmode(GPIO.BOARD)
    GPIO.setup(GPIO_OFFSWITCH_PIN, GPIO.IN, pull_up_down=GPIO.PUD_UP)
    GPIO.add_event_detect(GPIO_OFFSWITCH_PIN, GPIO.FALLING, callback=turn_off, bouncetime=10000)


if __name__ == "__main__":
    init_gpio()
    arduino.write("I\n".encode("utf-8"))
    time.sleep(0.01)
    arduino.write("D0Init I2C Comm\n".encode("utf-8"))
    arduino.write("Q\n".encode("utf-8"))

    serial_reader_state = 2
    t1 = threading.Thread(target=serial_listener)
    t1.start()
    time.sleep(0.01)
    # get button push length, start retropie on a short push and normal desktop environment on long push
    arduino.write("B\n".encode("utf-8"))
    time.sleep(0.01)
    # get initial volume reading
    arduino.write("V\n".encode("utf-8"))
    time.sleep(0.01)
    serial_writer_state = 2
    serial_writer()
    
