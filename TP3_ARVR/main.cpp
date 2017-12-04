#include "vrpn_Tracker.h"
#include <Windows.h>
#include <iostream>
#include <conio.h>
#include <SFML/Graphics.hpp>
#include <queue>
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

struct Pos {
	float x;
	float y;
};

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
	int cursorX = 0;
	int cursorY = 0;

	deque<Pos> queue_right_hand;


	vrpn_Tracker_Remote *tracker = new vrpn_Tracker_Remote(addr);
	tracker->register_change_handler(NULL, handle_tracker);

	RECT desktop;
	// Get a handle to the desktop window
	const HWND hDesktop = GetDesktopWindow();
	// Get the size of screen to the variable desktop
	GetWindowRect(hDesktop, &desktop);
	int hsize = desktop.right;
	int vsize = desktop.bottom;

	sf::RenderWindow window(sf::VideoMode(200, 200), "SFML works!");
	sf::CircleShape shape(100.f);
	shape.setFillColor(sf::Color::Green);

	while (window.isOpen()) {
		//SFML closing
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();
		}

		//Emergency stop
		if (GetKeyState(VK_SPACE) & 0x8000)
		{
			return 0;
		}

		//Handlers
		tracker->mainloop();

		Pos pos_right_hand;
		pos_right_hand.x = right_hand_x;
		pos_right_hand.y = right_hand_y;

		queue_right_hand.push_front(pos_right_hand);
		if (queue_right_hand.size() > 10) {
			queue_right_hand.pop_back();
		}

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
			shape.setFillColor(sf::Color::Red);
		}
		if (debounce(right_hand_y - right_elbow_y < -arm_threshold, &right_hand_down)) {
			printf("Right arm DOWN\n");
			shape.setFillColor(sf::Color::Blue);
		}

		//Paint
		if (paint_is_open) {

			Pos sum;
			sum.x = 0;
			sum.y = 0;
			for (int i = 0; i < queue_right_hand.size(); i++) {
				sum.x += queue_right_hand[i].x;
				sum.y += queue_right_hand[i].y;
			}
			//printf("%f, %f", sum.x, sum.y);
			Pos mean_right_hand;
			mean_right_hand.x = sum.x / queue_right_hand.size();
			mean_right_hand.y = sum.y / queue_right_hand.size();
			
			cursorX = (mean_right_hand.x + h_offset) * hsize / h_amplitude;
			cursorY = (-mean_right_hand.y + v_offset) * vsize / v_amplitude;
			SetCursorPos(cursorX, cursorY);
			printf("%f %f\n", right_hand_x, right_hand_y);
			if (left_hand_up) {
				click(true);
			}
			else {
				click(false);
			}
		}

		window.clear();
		window.draw(shape);
		window.display();

		Sleep(16);
	}
}