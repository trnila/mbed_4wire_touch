#include <mbed.h>
#include <mbed_events.h>
#include "Touch.h"
#include "TouchPanel.h"
#include "config.h"
#include "commands.h"
#include "demo.h"

#define us2dc(t, us) ((double) (us) / ((t)*1000))

Configuration conf;

Serial pc(USBTX, USBRX);
Touch touch(MBED_CONF_APP_PIN_XP, MBED_CONF_APP_PIN_XM, MBED_CONF_APP_PIN_YP, MBED_CONF_APP_PIN_YM);
TouchPanel panel(touch);
PwmOut servoX(MBED_CONF_APP_PIN_SERVOX), servoY(MBED_CONF_APP_PIN_SERVOY);
InterruptIn centerBtn(MBED_CONF_APP_PIN_BTN_CENTER);
InterruptIn rectBtn(MBED_CONF_APP_PIN_BTN_DEMO);


const int pointCount = 10;
int lastPoint = 0;
Vectorf lastPoints[pointCount];

double lastX = 0, lastY = 0;
Vectorf dir;
int i = 0;

Vectorf positions[] = {
		Vectorf(1, 1),
		Vectorf(-1, 1),
		Vectorf(-1, -1),
		Vectorf(1, -1)
};
int currentPosition = 0;

#if INPUT_METHOD == INPUT_METHOD_TOUCH
	TouchPanel &input = panel;
#elif INPUT_METHOD == INPUT_METHOD_ACCELEROMETER
	#include "Accelerometer.h"
	AccelerometerInput input;
#else
	#error "wrong input method"
#endif

double cap(double val) {
	return std::min(std::max(val, SHIFT_MIN_US), SHIFT_MAX_US);
}

void control() {
	double x, y, z = 1;

	if(conf.state == STATE_BALANCE) {
		input.getPos(x, y);
	} else if(conf.state == STATE_DEMO && !conf.positions.empty()) {
		Vectorf v = conf.positions.current();

		x = v.x;
		y = v.y;

		if(i % 50 == 0) {
			conf.positions.next();
		}
	} else {
		return;
	}

	double zx = -x * MX / z;
	double zy = -y * MY / z;
	//double zx = -dir.x * MX / z;
	//double zy = -dir.y * MY / z;

	double angleX = zx / MX;
	double angleY = zy / MY;

	double DX = angleX * 1800 / M_PI;
	double DY = angleY * 1800 / M_PI;

	double USX = us2dc(DUTY_MS, CENTER_X_US + cap(DX));
	double USY = us2dc(DUTY_MS, CENTER_Y_US + cap(DY));

	//if(i % 4 == 0) {
	//	printf("FXOS8700Q ACC: X=%1.4f Y=%1.4f Z=%1.4f DX=%f DY=%f dir(%1.4f, %1.4f)\r\n", x, y, z, USX, USY, dir.x, dir.y);
		servoX.write(USX);
		servoY.write(USY);
	//}
	i++;

	lastX = x;
	lastY = y;
}

void center() {
	servoX.write(us2dc(DUTY_MS, CENTER_X_US));
	servoY.write(us2dc(DUTY_MS, CENTER_Y_US));
	conf.state = STATE_STOP;
}

void rect() {
	conf.state = STATE_DEMO;
	currentPosition = 0;
}


int main() {
	pc.baud(115200);
	pc.printf("Initialized\n");

	CommandsProcessor cmds(pc, conf);
	cmds.start();

	centerBtn.fall(&center);
	rectBtn.fall(&rect);

	servoX.period(DUTY_MS / 1000.0);
	servoY.period(DUTY_MS / 1000.0);

#ifdef FUNCTION_CENTER
	center();
	for(;;);
#endif

	panel.setPressureThreshold(120000);
	panel.calibrateX(8800, 49600, true);
	panel.calibrateY(13000, 42568, true);
	panel.setSwapXY(true);

	center();

#ifdef FUNCTION_CALIBRATE
	double X, Y;
	int RX, RY, pressure;
	bool pressed;
	while (true) {
		pressed = panel.getPosRaw(X, Y, RX, RY, pressure);
		pc.printf("X=0 Y=0 RX=%d RY=%d pressure=%d pressed=%d\r\n", RX, RY, pressure, pressed);
		wait(0.02f);
	}
#endif

	EventQueue queue;

	queue.call_every(MEASUREMENT_PERIOD_MS, control);
	queue.dispatch();
}
