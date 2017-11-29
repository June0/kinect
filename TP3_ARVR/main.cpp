#include "vrpn_Tracker.h"
#include <Windows.h>
#include <iostream>
using namespace std;

//Global variables
float left_hand_x = 0;
float left_hand_y = 0;
float left_hand_z = 1;//Fix to avoid a "Clap" message on start
float right_hand_x = 0;
float right_hand_y = 0;
float right_hand_z = 0;
float left_elbow_y = 0;
float right_elbow_y = 0;


//Tracker
void VRPN_CALLBACK handle_tracker(void* userData, const vrpn_TRACKERCB t)
{
	switch(t.sensor)
	{
		case 8: 
			left_hand_x = t.pos[0];
			left_hand_y = t.pos[1];
			left_hand_z = t.pos[2];
			break;
		case 6:
			left_elbow_y = t.pos[1];
			break;
		case 14:
			right_hand_x = t.pos[0];
			right_hand_y = t.pos[1];
			right_hand_z = t.pos[2];
			break;
		case 12:
			right_elbow_y = t.pos[1];
			break;
		default: break;
	}
}

//Function to detect state changes
bool debounce(bool cond, bool *state) {
	bool return_state = cond && !*state;
	if (cond) {
		*state = true;
	}
	else {
		*state = false;
	}
	return return_state;
}

//Emulate mouse click
void click(bool pressed)
{
	POINT p;
	GetCursorPos(&p);
	printf("%d %d\n", p.x, p.y);

	INPUT input;
	input.type = INPUT_MOUSE;
	input.mi.dx = p.x;
	input.mi.dy = p.y;
	input.mi.dwFlags = (MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE | MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP);
	input.mi.mouseData = 0;
	input.mi.dwExtraInfo = NULL;
	input.mi.time = 0;

	
	if (pressed) {
		input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
	}
	else {
		input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
	}
	SendInput(1, &input, sizeof(INPUT));	
}


int main(int argc, char* argv[])
{
	char *addr = "Tracker0@172.16.169.2:3883";

	float arm_threshold = 0.1;
	float clap_threshold = 0.1;
	bool clap = false;
	bool left_hand_up = false;
	bool right_hand_up = false;
	bool left_hand_down = false;
	bool right_hand_down = false;

	bool paint_is_open = false;

	vrpn_Tracker_Remote *tracker = new vrpn_Tracker_Remote(addr);
	tracker->register_change_handler(NULL, handle_tracker);

	while (1) {
		//Handlers
		tracker->mainloop();

		//Clap
		if (debounce(abs(left_hand_x - right_hand_x) < clap_threshold && abs(left_hand_y - right_hand_y) < clap_threshold && abs(left_hand_z - right_hand_z) < clap_threshold, &clap)) {
			printf("Clap\n");
			if (paint_is_open) {
				system("taskkill /F /IM mspaint.exe");
				paint_is_open = false;
			}
			else {
				system("start mspaint");
				paint_is_open = true;
			}
		}

		//Arms
		if (debounce(left_hand_y - left_elbow_y > arm_threshold, &left_hand_up)) {
			printf("Left arm UP\n");
		}
		if (debounce(left_hand_y - left_elbow_y < - arm_threshold, &left_hand_down)) {
			printf("Left arm DOWN\n");
		}
		if (debounce(right_hand_y - right_elbow_y > arm_threshold, &right_hand_up)) {
			printf("Right arm UP\n");
		}
		if (debounce(right_hand_y - right_elbow_y < - arm_threshold, &right_hand_down)) {
			printf("Right arm DOWN\n");
		}

		//Paint
		if (paint_is_open) {
			//printf("x:%f y:%f z:%f\n", right_hand_x, right_hand_y, right_hand_z);
			SetCursorPos((right_hand_x + 1) * 500, (-right_hand_y + 1) * 500);
			if (left_hand_up) {
				click(true);
			}
			else {
				click(false);
			}
		}

		Sleep(16);
	}
}