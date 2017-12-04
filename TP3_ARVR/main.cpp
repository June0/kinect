#include "vrpn_Tracker.h"
#include <Windows.h>
#include <iostream>
#include <conio.h>
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
		case 6: //5 6
			left_elbow_y = t.pos[1];
			break;
		case 8: //7 8
			left_hand_x = t.pos[0];
			left_hand_y = t.pos[1];
			left_hand_z = t.pos[2];
			break;
		case 12: //9 12
			right_elbow_y = t.pos[1];
			break;
		case 14: //11 14
			right_hand_x = t.pos[0];
			right_hand_y = t.pos[1];
			right_hand_z = t.pos[2];
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

void GetDesktopResolution(int& hsize, int& vsize)
{
	RECT desktop;
	// Get a handle to the desktop window
	const HWND hDesktop = GetDesktopWindow();
	// Get the size of screen to the variable desktop
	GetWindowRect(hDesktop, &desktop);
	// The top left corner will have coordinates (0,0)
	// and the bottom right corner will have coordinates
	// (horizontal, vertical)
	hsize = desktop.right;
	vsize = desktop.bottom;
}


int main(int argc, char* argv[])
{
	char *addr = "Tracker0@172.16.169.10:3883";

	float arm_threshold = 0.1;
	float clap_threshold = 0.1;
	float h_amplitude = 1.6; //kinect  horizontal amplitude
	float v_amplitude = 1.2; //kinect  vertical amplitude
	float h_offset = 0.8;
	float v_offset = 0.65;
	bool clap = false;
	bool left_hand_up = false;
	bool right_hand_up = false;
	bool left_hand_down = false;
	bool right_hand_down = false;

	bool paint_is_open = false;

	vrpn_Tracker_Remote *tracker = new vrpn_Tracker_Remote(addr);
	tracker->register_change_handler(NULL, handle_tracker);

	RECT desktop;
	// Get a handle to the desktop window
	const HWND hDesktop = GetDesktopWindow();
	// Get the size of screen to the variable desktop
	GetWindowRect(hDesktop, &desktop);
	int hsize = desktop.right;
	int vsize = desktop.bottom;

	while (1) {
		if (GetKeyState(VK_SPACE) & 0x8000)
		{
			return 0; //emergency stop
		}
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
				click(false);
			}
		}

		//Arms
		if (debounce(left_hand_y - left_elbow_y > arm_threshold, &left_hand_up)) {
			printf("Left arm UP\n");
		}
		if (debounce(left_hand_y - left_elbow_y < -arm_threshold, &left_hand_down)) {
			printf("Left arm DOWN\n");
		}
		if (debounce(right_hand_y - right_elbow_y > arm_threshold, &right_hand_up)) {
			printf("Right arm UP\n");
		}
		if (debounce(right_hand_y - right_elbow_y < -arm_threshold, &right_hand_down)) {
			printf("Right arm DOWN\n");
		}

		//Paint
		if (paint_is_open) {
			SetCursorPos((right_hand_x + h_offset) * hsize / h_amplitude, (-right_hand_y + v_offset) * vsize / v_amplitude);
			printf("%f %f\n", right_hand_x, right_hand_y);
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