
FAN_SPEEDS = [{"temperature":55.0, "fan_speed": 255}, {"temperature": 60.0, "fan_speed": 255}]
DELTA_T = 2.5
fan_idx_old = 0

BAT_CONVERSION = 0.011695888188


def calculate_battery_voltage(raw_voltage):
    return float(raw_voltage) * BAT_CONVERSION

class FanSpeedCalculator:

    def __init__(self):
        self.fan_idx_old = 0

    def compute_fan_speed(self, cpu_temp):
        fan_idx = 0
        for f in FAN_SPEEDS:
            if cpu_temp > f["temperature"]:
                fan_idx += 1
        if fan_idx > self.fan_idx_old:
            outval = FAN_SPEEDS[fan_idx -1]["fan_speed"]
        else:
            fan_idx = len(FAN_SPEEDS)
            for f in reversed(FAN_SPEEDS):
                if cpu_temp < f["temperature"] - DELTA_T:
                    fan_idx -= 1
            if 0 < fan_idx < self.fan_idx_old:
                outval = FAN_SPEEDS[fan_idx - 1]["fan_speed"]
            elif fan_idx == 0:
                outval = 0
            elif self.fan_idx_old > 0:
                outval = FAN_SPEEDS[self.fan_idx_old - 1]["fan_speed"]
            else:
                outval = 0
        self.fan_idx_old = fan_idx
        return outval
