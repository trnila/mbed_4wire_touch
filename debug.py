import argparse

from serial import Serial
from turtle import Turtle, setworldcoordinates

MAX = 65535
RESOLUTION_X = 180
RESOLUTION_Y = 230


def setup_turtle():
    t = Turtle()
    setworldcoordinates(-1, -1, RESOLUTION_X, RESOLUTION_Y)
    t.speed(0)
    return t


class Touch:
    def __init__(self):
        self.s = Serial('/dev/ttyACM0', 115200)

    def get_touch(self):
        while True:
            try:
                # parse line to dict and check that all keys are available, otherwise try another line
                def pair(p):
                    return p[0], int(p[1])

                result = dict([pair(i.split('=', 2)) for i in self.s.readline().decode('utf-8').strip().split(' ')])
                if all([k in result for k in ['X', 'Y', 'RX', 'RY', 'pressure', 'pressed']]):
                    return result
            except Exception as e:
                print(e)


class Calibrator:
    def run(self, touch):
        minX = MAX
        maxX = 0
        minY = MAX
        maxY = 0

        while True:
            evt = touch.get_touch()
            X = evt['RX']
            Y = evt['RY']

            if X < minX:
                minX = X
            if X > maxX:
                maxX = X

            if Y < minY:
                minY = Y
            if Y > maxY:
                maxY = Y

            print("int minX = {}, maxX = {};".format(minX, maxX))
            print("int minY = {}, maxY = {};".format(minY, maxY))
            print("pressure {}".format(evt['pressure']))
            print("")


class Render:
    def run(self, touch):
        t = setup_turtle()

        minX = 15078
        maxX = 36855
        minY = 18613
        maxY = 51230

        def m(val, fa, fb, res):
            if val > fb:
                val = fb

            val -= fa

            if val < 0:
                val = 0

            return val / (fb - fa) * res

        while True:
            evt = touch.get_touch()

            x = RESOLUTION_X - m(evt['RX'], minX, maxX, RESOLUTION_X)
            y = RESOLUTION_Y - m(evt['RY'], minY, maxY, RESOLUTION_Y)

            t.pendown()
            t.goto(x, y)
            print(evt)


class DirectRender:
    def run(self, touch):
        t = setup_turtle()
        t.pendown()

        while True:
            evt = touch.get_touch()
            if evt['pressed']:
                t.goto(evt['X'], evt['Y'])
                print(evt)


modes = {
    'calibrate': Calibrator,
    'render': Render,
    'direct_render': DirectRender
}

parser = argparse.ArgumentParser()
parser.add_argument('mode', choices=modes.keys())

args = parser.parse_args()


mode = modes[args.mode]()
touch = Touch()
mode.run(touch)
