#ifndef STATE_HPP
#define STATE_HPP 

struct State 
{
	/*
	 * D | +/- 	
	 * X = forward/backward
	 * Y = right/left 
	 * Z = down/up
	 * Yaw, Pitch, and Roll are obvious.
	 * Acceleration: 
	 * Surge = forward
	 * Sway = right/left
	 * Heave = down/up
	 */
	float x, y, z;
	float yaw, pitch, roll;
	float sway, heave, surge;

	void read();
	void print();
};

#endif 
