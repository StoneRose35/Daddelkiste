
FAN_SPEEDS = [{"temperature": 0.0, "fan_speed": 0},{"temperature":58.0, "fan_speed": 255}, {"temperature": 85.0, "fan_speed": 255}]
DELTA_T = 5.0
fan_idx_old = 0

BAT_CONVERSION = 0.011695888188


def calculate_battery_voltage(raw_voltage):
    if raw_voltage > 0:
        return float(raw_voltage) * BAT_CONVERSION
    else:
        return -1.0


class FanSpeedCalculator:

    def __init__(self):
        self.fan_idx_old = 0

    def compute_fan_speed(self, cpu_temp):
        fan_idx = 0
        outval = FAN_SPEEDS[self.fan_idx_old]["fan_speed"]
        if cpu_temp > FAN_SPEEDS[self.fan_idx_old+1]["temperature"]:
            outval = FAN_SPEEDS[self.fan_idx_old+1]["fan_speed"]
            self.fan_idx_old += 1
        elif cpu_temp < FAN_SPEEDS[self.fan_idx_old]["temperature"] - DELTA_T:
            outval = FAN_SPEEDS[self.fan_idx_old-1]["fan_speed"]
            self.fan_idx_old -= 1
        return outval
