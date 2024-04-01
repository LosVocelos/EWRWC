from time import sleep


class Motors:

    def __init__(self, spi):
        self.spi = spi
        self._speed = [0, 0]
        self._angle = 0

        self.servo_base = 501
        self.servo_step = 1995 / float(180)  # 1995 is the maximum range of the servo with some safeguards added
        self.period = (1000000 / 100)  # Perion of PWM in us
        self.msg = [0x01, 0b00000110, 0x10, 0x00, 0x00, 0x00, 0x00]

    def __del__(self):
        self.spi.xfer([0xFF])
        print("killing")

    def enable(self,state=True):
        self.spi.xfer([state, 0x09])

    @property
    def speed(self):
        return self._speed

    @property
    def angle(self):
        return self._angle

    @speed.setter
    def speed(self, newSpeed):
        self._speed = newSpeed
        if newSpeed[0] < 0:
            self.msg[1] += 0b00000100
        if newSpeed[1] < 0:
            self.msg[1] -= 0b00000001

        h1, h2 = max(min(abs(self._speed[1]), 65535), 0).to_bytes(2, "big")
        self.msg[3] = h1
        self.msg[4] = h2

        h1, h2 = max(min(abs(self._speed[0]), 65535), 0).to_bytes(2, "big")
        self.msg[5] = h1
        self.msg[6] = h2

        print(self.msg)

    @angle.setter
    def angle(self, newAngle):
        self._angle = newAngle
        if (self._angle < 0):
            self._angle = 0
        elif (self._angle > 180):
            self._angle = 180

        pulseWidth = self._angle * self.servo_step + self.servo_base # Angle * Degree represented in microseconds + Pulse for 0 degrees
        duty = int((pulseWidth / float(self.period)) * 0xFFFF)
        h1,h2 = duty.to_bytes(2, "big")
        self.spi.xfer([0x20, h1, h2])
