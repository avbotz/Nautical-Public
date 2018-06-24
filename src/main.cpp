#include <Arduino.h>
#include "ahrs/ahrs.h"
#include "streaming.h"
#include "config.h"
#include "kalman.hpp"
#include "motor.hpp"
#include "util.hpp"
#include "pid.hpp"
#include "io.hpp"


void run()
{
	// Start IO between Nautical and hardware.
	io();

	// Initial motor strength at 0.
	float p = 0.0f;
	float mtr[NUM_MOTORS] = { 0.0f };
	Serial << _FLOAT(mtr[0], 6) << ' ' << _FLOAT(mtr[1], 6) << ' ' << 
		_FLOAT(mtr[2], 6) << ' ' << _FLOAT(mtr[3], 6) << ' ' <<
		_FLOAT(mtr[4], 6) << ' ' << _FLOAT(mtr[5], 6) << ' ' << 
		_FLOAT(mtr[6], 6) << ' ' << _FLOAT(mtr[7], 6) << '\n';

	// Current state represents location, while desired state holds destination.
	float current[DOF] = { 0.0f };
	float desired[DOF] = { 0.0f };

	// Kalman filter initial state and error covariance.
	float state[N] = {
		0.000, 0.000, 0.000, 0.000, 0.000, 0.000
	};
	float covar[N*N] = {	
		1.000, 0.000, 0.000, 0.000, 0.000, 0.000,
		0.000, 1.000, 0.000, 0.000, 0.000, 0.000,
		0.000, 0.000, 1.000, 0.000, 0.000, 0.000,
		0.000, 0.000, 0.000, 1.000, 0.000, 0.000,
		0.000, 0.000, 0.000, 0.000, 1.000, 0.000,
		0.000, 0.000, 0.000, 0.000, 0.000, 1.000,
	};

	// Compute initial bias from the accelerometer.
	for (int i = 0; i < 100; i++)
	{
		delay(1);
		ahrs_att_update();
	}
	for (int i = 0; i < 100; i++)
	{
		delay(1);
		ahrs_att_update();
		afbias += ahrs_accel((enum accel_axis)(SURGE));
		ahbias += ahrs_accel((enum accel_axis)(SWAY));
	}
	afbias /= 100.0;
	ahbias /= 100.0;

	// Initial time, helps compute time difference.
	uint32_t ktime = micros();
	uint32_t mtime = micros();

	// Start PID controllers for each DOF.
	PID controllers[DOF];
	for (int i = 0; i < DOF; i++)
		controllers[i].init(GAINS[i][0], GAINS[i][1], GAINS[i][2]);

	while (true)
	{
		// Important, otherwise data is sent too fast.
		delay(10);
		ahrs_att_update();

		// Check for user input.
		if (Serial.available() > 0)
		{
			char c = Serial.read();

			// Return accelerometer data. 
			if (c == 'b')
				Serial << _FLOAT(afbias, 6) << ' ' << _FLOAT(ahbias, 6) << '\n';

			// Return Kalman filter state and covariance.
			else if (c == 'k')
			{
				Serial << _FLOAT(state[0], 6) << ' ' << _FLOAT(state[1], 6) << ' ' << 
					_FLOAT(state[2], 6) << ' ' << _FLOAT(state[3], 6) << ' ' <<
					_FLOAT(state[4], 6) << ' ' << _FLOAT(state[5], 6) << '\n';
			}

			// Return current state.
			else if (c == 'c')
			{
				Serial << _FLOAT(current[0], 6) << ' ' << _FLOAT(current[1], 6) << ' ' << 
					_FLOAT(current[2], 6) << ' ' << _FLOAT(current[3], 6) << ' ' <<
					_FLOAT(current[4], 6) << ' ' << _FLOAT(current[5], 6) << '\n';
			}

			// Return desired state.
			else if (c == 'd')
			{
				Serial << _FLOAT(desired[0], 6) << ' ' << _FLOAT(desired[1], 6) << ' ' << 
					_FLOAT(desired[2], 6) << ' ' << _FLOAT(desired[3], 6) << ' ' <<
					_FLOAT(desired[4], 6) << ' ' << _FLOAT(desired[5], 6) << '\n';
			}

			// Return motor settings.
			else if (c == 'm')
			{
				Serial << _FLOAT(mtr[0], 6) << ' ' << _FLOAT(mtr[1], 6) << ' ' << 
					_FLOAT(mtr[2], 6) << ' ' << _FLOAT(mtr[3], 6) << ' ' <<
					_FLOAT(mtr[4], 6) << ' ' << _FLOAT(mtr[5], 6) << ' ' << 
					_FLOAT(mtr[6], 6) << ' ' << _FLOAT(mtr[7], 6) << '\n';
			}

			// Receive new power setting.
			else if (c == 'p')
				p = Serial.parseFloat();

			// Receive new desired state.
			else if (c == 's')
				for (int i = 0; i < DOF; i++)
					desired[i] = Serial.parseFloat();
		}

		// Kalman filter removes noise from measurements and estimates the new
		// state (linear).
		ktime = kalman(state, covar, ktime);

		// Compute rest of the DOF.
		current[F] = state[0];
		current[H] = state[2];
		current[V] = (analogRead(NPIN) - 230.0)/65.0;
		current[Y] = ahrs_att((enum att_axis) (YAW));
		current[P] = ahrs_att((enum att_axis) (PITCH));
		current[R] = ahrs_att((enum att_axis) (ROLL));

		// Compute state difference.
		float dstate[DOF] = { 0.0f };
		for (int i = 0; i < BODY_DOF; i++)
			dstate[i] = desired[i] - current[i];

		// Compute PID within motors and set thrust.
		mtime = motors(controllers, dstate, mtr, p, mtime);
	}
}

void setup()
{
	Serial.begin(9600);
	run();
}
void loop() {}
