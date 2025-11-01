#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>
#include <sstream>
#include <cmath>

using namespace cv;
using namespace std;

int g_b = 0;
int g_g = 0;
int g_r = 255;

int g_h = 0;
int g_s = 255;
int g_v = 255;

int g_h_display = 0;
int g_s_display = 100;
int g_v_display = 100;

int g_c = 0;
int g_m = 100;
int g_y = 100;
int g_k = 0;

const string g_window_controls = "Controls";
const string g_window_display = "Display";
const string g_window_input = "Manual Input";

Mat g_color_display(180, 220, CV_8UC3);
Mat g_hsv_palette(180, 256, CV_8UC3);
Mat g_display_window;
Mat g_display_with_text;

bool g_is_updating = false;

enum InputModel { NONE, RGB_INPUT, HSV_INPUT, CMYK_INPUT };
InputModel g_input_model = NONE;
string g_input_prompt = "";
string g_input_buffer = "";

void update_all_trackbars();
void update_palette();
void update_display();
void on_bgr_trackbar(int, void*);
void on_hsv_trackbar(int, void*);
void on_cmyk_trackbar(int, void*);
void parse_and_update();
void handle_text_input(int key);

void bgr_to_hsv() {
    Mat bgr_color(1, 1, CV_8UC3, Scalar(g_b, g_g, g_r));
    Mat hsv_color;
    cvtColor(bgr_color, hsv_color, COLOR_BGR2HSV);
    Vec3b hsv = hsv_color.at<Vec3b>(0, 0);
    g_h = hsv[0];
    g_s = hsv[1];
    g_v = hsv[2];
    g_h_display = g_h * 2;
    g_s_display = static_cast<int>(std::round(g_s / 2.55));
    g_v_display = static_cast<int>(std::round(g_v / 2.55));
}

void hsv_to_bgr() {
    Mat hsv_color(1, 1, CV_8UC3, Scalar(g_h, g_s, g_v));
    Mat bgr_color;
    cvtColor(hsv_color, bgr_color, COLOR_HSV2BGR);
    Vec3b bgr = bgr_color.at<Vec3b>(0, 0);
    g_b = bgr[0];
    g_g = bgr[1];
    g_r = bgr[2];
}

void bgr_to_cmyk() {
    double r_norm = g_r / 255.0;
    double g_norm = g_g / 255.0;
    double b_norm = g_b / 255.0;
    double k_norm = 1.0 - max({r_norm, g_norm, b_norm});
    if (abs(k_norm - 1.0) < 1e-6) {
        g_c = 0;
        g_m = 0;
        g_y = 0;
    } else {
        g_c = static_cast<int>(std::round(((1.0 - r_norm - k_norm) / (1.0 - k_norm)) * 100));
        g_m = static_cast<int>(std::round(((1.0 - g_norm - k_norm) / (1.0 - k_norm)) * 100));
        g_y = static_cast<int>(std::round(((1.0 - b_norm - k_norm) / (1.0 - k_norm)) * 100));
    }
    g_k = static_cast<int>(std::round(k_norm * 100));
}

void cmyk_to_bgr() {
    double c_norm = g_c / 100.0;
    double m_norm = g_m / 100.0;
    double y_norm = g_y / 100.0;
    double k_norm = g_k / 100.0;
    g_r = static_cast<int>(std::round(255.0 * (1.0 - c_norm) * (1.0 - k_norm)));
    g_g = static_cast<int>(std::round(255.0 * (1.0 - m_norm) * (1.0 - k_norm)));
    g_b = static_cast<int>(std::round(255.0 * (1.0 - y_norm) * (1.0 - k_norm)));
}

void update_palette() {
    Mat hsv_palette_hsv(180, 256, CV_8UC3);
    for (int h = 0; h < 180; h++) {
        for (int s = 0; s < 256; s++) {
            hsv_palette_hsv.at<Vec3b>(h, s) = Vec3b(h, s, g_v);
        }
    }
    cvtColor(hsv_palette_hsv, g_hsv_palette, COLOR_HSV2BGR);
}

void update_display() {
    g_color_display.setTo(Scalar(g_b, g_g, g_r));
    double luminance = (0.299 * g_r + 0.587 * g_g + 0.114 * g_b);
    Scalar text_color = (luminance > 128) ? Scalar(0, 0, 0) : Scalar(255, 255, 255);
    string rgb_text = "R: " + to_string(g_r) + " G: " + to_string(g_g) + " B: " + to_string(g_b);
    string hsv_text = "H: " + to_string(g_h_display) + " S: " + to_string(g_s_display) + " V: " + to_string(g_v_display);
    string cmyk_text = "C: " + to_string(g_c) + " M: " + to_string(g_m) + " Y: " + to_string(g_y) + " K: " + to_string(g_k);
    int font = FONT_HERSHEY_SIMPLEX;
    double font_scale = 0.5;
    int thickness = 1;
    putText(g_color_display, "RGB:", Point(10, 30), font, font_scale, text_color, thickness);
    putText(g_color_display, rgb_text, Point(20, 50), font, font_scale, text_color, thickness);
    putText(g_color_display, "HSV:", Point(10, 80), font, font_scale, text_color, thickness);
    putText(g_color_display, hsv_text, Point(20, 100), font, font_scale, text_color, thickness);
    putText(g_color_display, "CMYK:", Point(10, 130), font, font_scale, text_color, thickness);
    putText(g_color_display, cmyk_text, Point(20, 150), font, font_scale, text_color, thickness);
    hconcat(g_color_display, g_hsv_palette, g_display_window);
    g_display_window.copyTo(g_display_with_text);
    string hint = "Click palette for H/S. Use 'r', 'h', 'c' for manual input.";
    putText(g_display_with_text, hint, Point(10, g_display_window.rows - 10), FONT_HERSHEY_SIMPLEX, 0.45, Scalar(255, 255, 255), 1);
    imshow(g_window_display, g_display_with_text);
}

void update_all_trackbars() {
    setTrackbarPos("Blue (B)", g_window_controls, g_b);
    setTrackbarPos("Green (G)", g_window_controls, g_g);
    setTrackbarPos("Red (R)", g_window_controls, g_r);
    setTrackbarPos("Hue (H)", g_window_controls, g_h_display);
    setTrackbarPos("Saturation (S)", g_window_controls, g_s_display);
    setTrackbarPos("Value (V)", g_window_controls, g_v_display);
    setTrackbarPos("Cyan (C)", g_window_controls, g_c);
    setTrackbarPos("Magenta (M)", g_window_controls, g_m);
    setTrackbarPos("Yellow (Y)", g_window_controls, g_y);
    setTrackbarPos("Black (K)", g_window_controls, g_k);
}

void on_bgr_trackbar(int, void*) {
    if (g_is_updating) return;
    g_is_updating = true;
    bgr_to_hsv();
    bgr_to_cmyk();
    update_all_trackbars();
    update_palette();
    update_display();
    g_is_updating = false;
}

void on_hsv_trackbar(int, void*) {
    if (g_is_updating) return;
    g_is_updating = true;
    g_h = g_h_display / 2;
    g_s = static_cast<int>(std::round(g_s_display * 2.55));
    g_v = static_cast<int>(std::round(g_v_display * 2.55));
    hsv_to_bgr();
    bgr_to_cmyk();
    update_all_trackbars();
    update_palette();
    update_display();
    g_is_updating = false;
}

void on_cmyk_trackbar(int, void*) {
    if (g_is_updating) return;
    g_is_updating = true;
    cmyk_to_bgr();
    bgr_to_hsv();
    update_all_trackbars();
    update_palette();
    update_display();
    g_is_updating = false;
}

void on_palette_mouse(int event, int x, int y, int flags, void* userdata) {
    if (event == EVENT_LBUTTONDOWN) {
        if (g_is_updating) return;
        int palette_x_start = 220;
        int palette_width = 256;
        int palette_height = 180;
        if (x >= palette_x_start && x < (palette_x_start + palette_width) && y >= 0 && y < palette_height) {
            g_is_updating = true;
            g_h = y;
            g_s = x - palette_x_start;
            g_h_display = g_h * 2;
            g_s_display = static_cast<int>(std::round(g_s / 2.55));
            hsv_to_bgr();
            bgr_to_cmyk();
            update_all_trackbars();
            update_display();
            g_is_updating = false;
        }
    }
}

void start_input(InputModel model, const string& prompt) {
    g_input_model = model;
    g_input_prompt = prompt;
    g_input_buffer = "";
    namedWindow(g_window_input, WINDOW_AUTOSIZE);
    handle_text_input(-1);
}

void parse_and_update() {
    stringstream ss(g_input_buffer);
    int v1, v2, v3, v4;
    char comma;
    try {
        if (g_input_model == RGB_INPUT) {
            ss >> v1 >> comma >> v2 >> comma >> v3;
            if (!ss.fail()) {
                g_r = v1; g_g = v2; g_b = v3;
                on_bgr_trackbar(0, 0);
            }
        } else if (g_input_model == HSV_INPUT) {
            ss >> v1 >> comma >> v2 >> comma >> v3;
            if (!ss.fail()) {
                g_h_display = v1;
                g_s_display = v2;
                g_v_display = v3;
                on_hsv_trackbar(0, 0);
            }
        } else if (g_input_model == CMYK_INPUT) {
            ss >> v1 >> comma >> v2 >> comma >> v3 >> comma >> v4;
            if (!ss.fail()) {
                g_c = v1; g_m = v2; g_y = v3; g_k = v4;
                on_cmyk_trackbar(0, 0);
            }
        }
    } catch (const std::exception& e) {
        cout << "Input error: " << e.what() << endl;
    }
    destroyWindow(g_window_input);
    g_input_model = NONE;
}

void handle_text_input(int key) {
    if (g_input_model == NONE) return;
    switch (key) {
        case 13:
            parse_and_update();
            return;
        case 27:
            destroyWindow(g_window_input);
            g_input_model = NONE;
            return;
        case 8:
            if (!g_input_buffer.empty()) {
                g_input_buffer.pop_back();
            }
            break;
        case -1:
            break;
        default:
            if ((key >= '0' && key <= '9') || key == ',') {
                g_input_buffer += (char)key;
            }
            break;
    }
    Mat input_img(100, 500, CV_8UC3, Scalar(20, 20, 20));
    putText(input_img, g_input_prompt, Point(10, 30), FONT_HERSHEY_SIMPLEX, 0.6, Scalar(255, 255, 255), 1);
    putText(input_img, g_input_buffer + "_", Point(10, 70), FONT_HERSHEY_SIMPLEX, 0.8, Scalar(200, 200, 100), 2);
    imshow(g_window_input, input_img);
}

int main() {
    namedWindow(g_window_display, WINDOW_AUTOSIZE);
    namedWindow(g_window_controls, WINDOW_AUTOSIZE);
    createTrackbar("Red (R)", g_window_controls, &g_r, 255, on_bgr_trackbar);
    createTrackbar("Green (G)", g_window_controls, &g_g, 255, on_bgr_trackbar);
    createTrackbar("Blue (B)", g_window_controls, &g_b, 255, on_bgr_trackbar);
    createTrackbar("Hue (H)", g_window_controls, &g_h_display, 360, on_hsv_trackbar);
    createTrackbar("Saturation (S)", g_window_controls, &g_s_display, 100, on_hsv_trackbar);
    createTrackbar("Value (V)", g_window_controls, &g_v_display, 100, on_hsv_trackbar);
    createTrackbar("Cyan (C)", g_window_controls, &g_c, 100, on_cmyk_trackbar);
    createTrackbar("Magenta (M)", g_window_controls, &g_m, 100, on_cmyk_trackbar);
    createTrackbar("Yellow (Y)", g_window_controls, &g_y, 100, on_cmyk_trackbar);
    createTrackbar("Black (K)", g_window_controls, &g_k, 100, on_cmyk_trackbar);
    setMouseCallback(g_window_display, on_palette_mouse);
    on_bgr_trackbar(0, 0);
    cout << "Application started." << endl;
    cout << "Press 'r' for RGB, 'h' for HSV, 'c' for CMYK input." << endl;
    cout << "Press ESC to exit." << endl;
    while (true) {
        int key = waitKey(30);
        if (key == 27) {
            if (g_input_model != NONE) {
                destroyWindow(g_window_input);
                g_input_model = NONE;
            } else {
                break;
            }
        }
        if (g_input_model != NONE) {
            handle_text_input(key);
        } else {
            switch (key) {
                case 'r':
                case 'R':
                    start_input(RGB_INPUT, "Enter R,G,B (0-255), e.g. 255,0,0");
                    break;
                case 'h':
                case 'H':
                    start_input(HSV_INPUT, "Enter H(0-360), S(0-100), V(0-100)");
                    break;
                case 'c':
                case 'C':
                    start_input(CMYK_INPUT, "Enter C,M,Y,K (0-100), e.g. 0,100,100,0");
                    break;
            }
        }
    }
    destroyAllWindows();
    return 0;
}