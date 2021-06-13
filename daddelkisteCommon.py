
FAN_SPEEDS = [{"temperature": -99.0, "fan_speed": 0},{"temperature":55.0, "fan_speed": 255}, {"temperature": 60.0, "fan_speed": 255}]
DELTA_T = 2.5
fan_idx_old = 0

BAT_CONVERSION = 0.011695888188


def calculate_battery_voltage(raw_voltage):
    return float(raw_voltage) * BAT_CONVERSION

class FanSpeedCalculator:

    def __init__(self):
        self.fan_idx_old = 0

    def compute_fan_speed(self, cpu_temp):
        fan_idx = -1
        for f in FAN_SPEEDS:
            if cpu_temp > f["temperature"]:
                fan_idx += 1
        if fan_idx > self.fan_idx_old:
            outval = FAN_SPEEDS[fan_idx]["fan_speed"]
        else:
            fan_idx = len(FAN_SPEEDS)-1
            for f in reversed(FAN_SPEEDS):
                if cpu_temp < f["temperature"] - DELTA_T:
                    fan_idx -= 1
            if fan_idx < self.fan_idx_old:
                outval = FAN_SPEEDS[fan_idx]["fan_speed"]
            else:
                outval = FAN_SPEEDS[self.fan_idx_old]["fan_speed"]
        self.fan_idx_old = fan_idx
        return outval
