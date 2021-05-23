import unittest
import daddelkisteCommon

class commTest(unittest.TestCase):

    def test_fanspeed(self):
        fan_speed_calculator = daddelkisteCommon.FanSpeedCalculator()
        v1 = fan_speed_calculator.compute_fan_speed(38.5)
        self.assertEqual(v1, 0)
        v2 = fan_speed_calculator.compute_fan_speed(55.3)
        self.assertEqual(v2, 160)
        v3 = fan_speed_calculator.compute_fan_speed(53.5)
        self.assertEqual(v3, 160)
        v4 = fan_speed_calculator.compute_fan_speed(51.4)
        self.assertEqual(v4, 0)
        v5 = fan_speed_calculator.compute_fan_speed(61.2)
        self.assertEqual(v5,255)
        v6 = fan_speed_calculator.compute_fan_speed(59.2)
        self.assertEqual(v6, 255)
        v7 = fan_speed_calculator.compute_fan_speed(56.7)
        self.assertEqual(v7, 160)


if __name__ == '__main__':
    unittest.main()
