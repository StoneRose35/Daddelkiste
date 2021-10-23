import evdev
import evdev.ecodes as e
import RPi.GPIO as GPIO

GC_GPIOPIN_AX_L = 13
GC_GPIOPIN_AX_R = 15
GC_GPIOPIN_AX_U = 16
GC_GPIOPIN_AX_D = 18

GC_GPIOPIN_SELECT = 29
GC_GPIOPIN_START = 31
GC_GPIOPIN_BTN_X_NORTH = 33
GC_GPIOPIN_BTN_Y_WEST = 37
GC_GPIOPIN_BTN_B_SOUTH = 36
GC_GPIOPIN_BTN_A_EAST = 32
GC_GPIOPIN_BTN_TL = 22
GC_GPIOPIN_BTN_TR = 24

pinmapping = [(GC_GPIOPIN_AX_L, e.EV_ABS, e.ABS_HAT0X, -1, 0),
              (GC_GPIOPIN_AX_R, e.EV_ABS, e.ABS_HAT0X, 1, 0),
              (GC_GPIOPIN_AX_U, e.EV_ABS, e.ABS_HAT0Y, -1, 0),
              (GC_GPIOPIN_AX_D, e.EV_ABS, e.ABS_HAT0Y, 1, 0),
              (GC_GPIOPIN_SELECT, e.EV_KEY, e.BTN_SELECT, 1, 0),
              (GC_GPIOPIN_START, e.EV_KEY, e.BTN_START, 1, 0),
              (GC_GPIOPIN_BTN_X_NORTH, e.EV_KEY, e.BTN_NORTH, 1, 0),
              (GC_GPIOPIN_BTN_Y_WEST, e.EV_KEY, e.BTN_WEST, 1, 0),
              (GC_GPIOPIN_BTN_B_SOUTH, e.EV_KEY, e.BTN_SOUTH, 1, 0),
              (GC_GPIOPIN_BTN_A_EAST, e.EV_KEY, e.BTN_EAST, 1, 0),
              (GC_GPIOPIN_BTN_TL, e.EV_KEY, e.BTN_TL, 1, 0),
              (GC_GPIOPIN_BTN_TR, e.EV_KEY, e.BTN_TR, 1, 0)
              ]


class Joystick:

    def __init__(self):
        self.capabilities = {e.EV_ABS: [(e.ABS_HAT0X,
                                         evdev.AbsInfo(value=0, min=-1, max=1, fuzz=0, flat=0, resolution=0)),
                                        (e.ABS_HAT0Y,
                                         evdev.AbsInfo(value=0, min=-1, max=1, fuzz=0, flat=0, resolution=0))],
                             e.EV_KEY: [e.BTN_WEST, e.BTN_SOUTH, e.BTN_EAST, e.BTN_NORTH, e.BTN_TL, e.BTN_TR,
                                        e.BTN_SELECT,
                                        e.BTN_START]}
        self.virtdevice = evdev.UInput(self.capabilities, "DaddelJoystick", vendor=9999, product=8888)

    def handle_gpio_event(self, mappingel):
        # mappingel: channel,etype,code,val_low,val_high
        if GPIO.input(mappingel[0]):
            self.virtdevice.write(mappingel[1], mappingel[2], mappingel[4])
            self.virtdevice.syn()
        else:
            self.virtdevice.write(mappingel[1], mappingel[2], mappingel[3])
            self.virtdevice.syn()

    def joystick_callback(self, channel):
        mappingel = filter(lambda x: x[0] == channel, pinmapping).__next__()
        self.handle_gpio_event(mappingel)

    def init_gpio(self, pin_nr):
        GPIO.setmode(GPIO.BOARD)
        GPIO.setup(pin_nr, GPIO.IN, pull_up_down=GPIO.PUD_UP)
        GPIO.add_event_detect(pin_nr, GPIO.BOTH, callback=self.joystick_callback, bouncetime=30)

    def init_joystick(self):
        for el in pinmapping:
            self.init_gpio(el[0])