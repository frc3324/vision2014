#include <cstdio>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <raspicam/raspicam_cv.h>
#include <raspicam/raspicam.h>
#include <raspicam/raspicamtypes.h>
#include "gpio.h"
#include <unistd.h>

using namespace std;
using namespace cv;

int width, height;
Mat hsv, thresh, image, gray;

const int INPUT_PIN = 17;
const int OUTPUT_PIN = 27;

unsigned char _hsl_lo[3] = {0, 0, 200};
unsigned char _hsl_hi[3] = {255, 255, 255};

vector<unsigned char> hsl_lo(_hsl_lo, _hsl_lo + 3);
vector<unsigned char> hsl_hi(_hsl_hi, _hsl_hi + 3);
//int track_lo[3] = {0, 0, 200};
//int track_hi[3] = {255, 255, 255};
int track_lo[3] = {30, 140, 140};
int track_hi[3] = {140, 255, 255};

int shutter_speed = 50; //microseconds
bool use_gui = false;
bool always_save = false;

struct pt {
	int x, y;
	pt(int _x, int _y) {
		x = _x;
		y = _y;
	}

	pt() {
		x = 0;
		y = 0;
	}
};

//int img[height][width][3];
//bool threshed[height][width];
uchar *threshed;

bool vis[1000][1000];

uchar val(pt p) {
	return thresh.data[p.y * width + p.x];
}

bool edge(pt p1, pt p2) {
	return val(p1) == val(p2) && !vis[p2.x][p2.y];
}

pt q[1000 * 1000];
pt *head = &q[0], *tail = &q[0];

void push(pt c) {
	*tail = c;
	tail++;
}

void pop() {
	head++;
}

pt front() {
	return *head;
}

bool empty() {
	return head == tail;
}

void reset() {
	head = tail = &q[0];
}

struct comp {
	int npixels;
	pt min, max;
	pt sum;
	comp() {
		npixels = 0;
		min = pt(1 << 30, 1 << 30);
		max = pt(-1, -1);
		sum = pt(0, 0);
	}

	comp(pt p) {
		npixels = 1;
		min = p;
		max = p;
		sum = p;
	}

};

pt min_pt(pt p, pt q) {
	return pt(min(p.x, q.x), min(p.y, q.y));
}

pt max_pt(pt p, pt q) {
	return pt(max(p.x, q.x), max(p.y, q.y));
}

void mergec(comp *c, comp d) {
	c->npixels += d.npixels;
	c->min = min_pt(c->min, d.min);
	c->max = max_pt(c->max, d.max);
	c->sum = pt(c->sum.x + d.sum.x, c->sum.y + d.sum.y);
}

comp bfs(pt start) {
	reset();
	push(start);
	
	comp ret;

	int n = 0;
	while (!empty()) {
		pt cur = front();
		int x = cur.x, y = cur.y;
		pop();
		
		if (vis[x][y]) continue;
		vis[x][y] = true;
		mergec(&ret, comp(cur));
		//printf("%d\n", threshed);
		if (x >= 1 && edge(cur, pt(x - 1, y))) push(pt(x - 1, y));
		if (y >= 1 && edge(cur, pt(x, y - 1))) push(pt(x, y - 1));
		if (x < width - 1 && edge(cur, pt(x + 1, y))) push(pt(x + 1, y));
		if (y < height - 1 && edge(cur, pt(x, y + 1))) push(pt(x, y + 1));
	}
	return ret;
}

int max(int x, int y) {
	return (x < y) ? y : x;
}

double compdist_sq(comp c, comp d) {
	pt c_cent = pt((double)c.sum.x / (double)c.npixels, (double)c.sum.y / (double)c.npixels);

	pt d_cent = pt((double)d.sum.x / (double)d.npixels, (double)d.sum.y / (double)d.npixels);
	pt diff = pt(c_cent.x - d_cent.x, c_cent.y - d_cent.y);
	return diff.x * diff.x + diff.y * diff.y;
}

int num_components() {
	memset(vis, 0, sizeof(vis));
	vector<comp> hori, vert;
	//int ret = 0;
	for (int x = 0; x < width; x++) {
		for (int y = 0; y < height; y++) {
			if (!vis[x][y] && val(pt(x, y))) {
				//ret++;
				//printf("%d %d\n", x, y);
				//if (bfs(pt(x, y)) >= 0) ret++;
				comp c = bfs(pt(x, y));
				//printf("%d\n", c.npixels);
				if (c.npixels > 200) {
					double aspect = (double)(c.max.y - c.min.y) / (double)(c.max.x - c.min.x);
					if (aspect < 1) {
						rectangle(image, Point(c.min.x, c.min.y), Point(c.max.x, c.max.y), Scalar(255, 0, 0));
						//printf("horizontal: %d\n", c.npixels);*/
						hori.push_back(c);
					} else if (aspect > 1) {
						rectangle(image, Point(c.min.x, c.min.y), Point(c.max.x, c.max.y), Scalar(0, 255, 0));
						//printf("vertical: %d\n", c.npixels);*/
						vert.push_back(c);
					}
				}
			}
		}
	}
	vector<int> mate(hori.size());
	int mated = 0;
	for (int i = 0; i < hori.size(); i++) {
		int m = 0;
		bool got = false;
		for (int j = 1; j < vert.size(); j++) {
			double curdist = compdist_sq(hori[i], vert[j]);
			double mdist = compdist_sq(hori[i], vert[m]);
			if (curdist < mdist) {
				m = j;
				got = true;
			}
		}
		if (got) mated++;
		comp c2 = hori[i];
		if (vert.size() > 0) mergec(&c2, vert[m]);

		rectangle(image, Point(c2.min.x, c2.min.y), Point(c2.max.x, c2.max.y), Scalar(0, 0, 255));
	}

	//return ret;
	//return 0;
	return mated;
}

void process(int inp) {
	cvtColor(image, hsv, CV_BGR2HSV);
	for (int i = 0; i < 3; i++) {
		hsl_lo[i] = (unsigned char)track_lo[i];
		hsl_hi[i] = (unsigned char)track_hi[i];
	}
	inRange(hsv, hsl_lo, hsl_hi, thresh);
	width = thresh.cols;
	height = thresh.rows;
	int goals_found = num_components();
	printf("found %d hot goals\n", goals_found);
	if (goals_found >= 1 && true) {
		GPIOWrite(OUTPUT_PIN, 1);
	} else {
		GPIOWrite(OUTPUT_PIN, 0);
	}
}

int main(int argc, char **argv) {
	int option;
	while ( (option = getopt(argc, argv, "gs:")) != -1) {
		switch (option) {
		case 'g':
			use_gui = true;
			break;
		case 's':
			shutter_speed = atoi(optarg);
			break;
		}
	}
	printf("VISION: initalizing camera with shutter %d and gui %s\n", shutter_speed, (use_gui ? "on" : "off"));
	raspicam::RaspiCam_Cv camera;
	{
		raspicam::RaspiCam camera2;
		camera2.setExposure(raspicam::RASPICAM_EXPOSURE_OFF);
		//camera2.setShutterSpeed(shutter_speed);
		//delete camera2;
	}
	camera.set(CV_CAP_PROP_FRAME_WIDTH, 640);
	camera.set(CV_CAP_PROP_FRAME_HEIGHT, 480);
	camera.set(CV_CAP_PROP_FORMAT, CV_8UC3);
	camera.set(CV_CAP_PROP_EXPOSURE, 1);
	
	if (use_gui) {
		cv::namedWindow("image");
		cv::createTrackbar("h_lo", "image", &track_lo[0], 255, NULL, NULL);
		cv::createTrackbar("s_lo", "image", &track_lo[1], 255, NULL, NULL);
		cv::createTrackbar("l_lo", "image", &track_lo[2], 255, NULL, NULL);
		cv::createTrackbar("h_hi", "image", &track_hi[0], 255, NULL, NULL);
		cv::createTrackbar("s_hi", "image", &track_hi[1], 255, NULL, NULL);
		cv::createTrackbar("l_hi", "image", &track_hi[2], 255, NULL, NULL);
	}
	
	printf("VISION: initializing GPIO pins, input %d output %d\n", INPUT_PIN, OUTPUT_PIN);
	GPIOExport(INPUT_PIN);
	GPIOExport(OUTPUT_PIN);
	GPIODirection(INPUT_PIN, IN);
	GPIODirection(OUTPUT_PIN, OUT);
	GPIOWrite(OUTPUT_PIN, 0);
	
	printf("VISION: opening camera\n");
	camera.open();
	int key = 0;
	int i = 0;
	char filepath[128];
	int inp = 0;
	while (key = cv::waitKey(25)) {
		if (key == 27) break;
		camera.grab();
		camera.retrieve(image);
		//printf("got image\n");
		int cur = GPIORead(INPUT_PIN);
		inp = inp || cur;
		//printf("received input of %d from cRio\n", cur);
		process(inp);
		if (use_gui) cv::imshow("image", image);
		if ((inp == 1 || always_save) && i % 5 == 0 && i <= 250) {
			snprintf(filepath, sizeof(filepath), "/home/pi/visionlog/image%d.jpg", i);
			cv::imwrite(filepath, image);
		}
		i++;
	}
	printf("VISION: closing camera\n");
	camera.release();
	printf("VISION: uninitializing GPIO pins\n");
	GPIOWrite(OUTPUT_PIN, 0);
	GPIOUnexport(INPUT_PIN);
	GPIOUnexport(OUTPUT_PIN);

	printf("final threshold values\n");
	printf("hue_lo: %d, sat_lo: %d, lum_lo: %d\n", hsl_lo[0], hsl_lo[1], hsl_lo[2]);
	printf("hue_hi: %d, sat_hi: %d, lum_hi: %d\n", hsl_hi[0], hsl_hi[1], hsl_hi[2]);
	return 0;
}
