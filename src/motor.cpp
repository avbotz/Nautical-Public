#include "streaming.h"
#include "pid.hpp"
#include "motor.hpp"
#include "m5/io_m5.h"
#include "util.hpp"


Motors::Motors()
{
	for (int i = 0; i < DOF; i++)
		this->controllers[i].init(GAINS[i][0], GAINS[i][1], GAINS[i][2]);
	for (int i = 0; i < NUM_MOTORS; i++)
		this->thrust[i] = 0.0f;
    for (int i = 0; i < DOF; i++)
        this->forces[i] = 0.0f;
    for (int i = 0; i < DOF; i++)
		this->pid[i] = 0.0f;
    this->p = 0.0f;
}

void Motors::power()
{
	m5_power(VERT_FL, thrust[0]);
	m5_power(VERT_FR, thrust[1]);
	m5_power(VERT_BL, thrust[2]);
	m5_power(VERT_BR, thrust[3]);
	m5_power(SURGE_FL, thrust[4]);
	m5_power(SURGE_FR, thrust[5]);
	m5_power(SURGE_BL, thrust[6]);
	m5_power(SURGE_BR, thrust[7]);
	m5_power_offer_resume();
}

void Motors::pause()
{
	io_m5_trans_stop();
}

uint32_t Motors::run(float *dstate, uint32_t t)
{
	// Calculate time difference since last iteration.
	uint32_t temp = micros();
	float dt = (temp - t)/(float)(1000000);

    // Calculate PID values.
    pid[F] = controllers[F].calculate(dstate[F], dt, 0.50);
    pid[H] = controllers[H].calculate(dstate[H], dt, 0.50);
	pid[V] = controllers[V].calculate(dstate[V], dt, 0.00);
    for (int i = BODY_DOF; i < GYRO_DOF; i++)
		pid[i] = controllers[i].calculate(dstate[i], dt, 0.0);

	// Default body motors to 0.
	for (int i = 0; i < NUM_MOTORS; i++)
		thrust[i] = 0.0f;
	if (p > 0.01f)
	{
		thrust[0] += 0.15f;
		thrust[1] -= 0.15f;
		thrust[2] -= 0.15f;
        thrust[3] += 0.15f;
	}

	// Compute final thrust given to each motor based on orientation matrix.
	for (int i = 0; i < NUM_MOTORS; i++)
		for (int j = 0; j < DOF; j++) 
			thrust[i] += p * pid[j] * ORIENTATION[i][j];
	if (!SIM) power();
	
    // Compute forces from motors.
    /*
    float MOUNT_ANGLE = sin(45.0);
    for (int i = 0; i < DOF; i++)
        forces[i] = 0.0f;
    forces[F] = MOUNT_ANGLE*(thrust[4]-thrust[5]-thrust[6]+thrust[7]);
    forces[H] = MOUNT_ANGLE*(thrust[4]+thrust[5]+thrust[6]+thrust[7]);
    forces[V] = thrust[0]-thrust[1]-thrust[2]+thrust[3];
    */

    return temp;
}
