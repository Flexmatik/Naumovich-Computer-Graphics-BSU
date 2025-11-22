#include <gtkmm.h>
#include <cairomm/cairomm.h>
#include <vector>
#include <deque>
#include <cmath>
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <chrono>
#include <string>

struct PixelTask {
    int x;
    int y;
    double r, g, b; 
    double alpha;   
};

class RasterApp : public Gtk::Window {
public:
    RasterApp();
    virtual ~RasterApp();

protected:
    // Сигналы
    void on_draw_clicked();
    void on_clear_clicked();
    void on_resize_clicked();
    void on_draw_real_clicked();
    void on_scale_changed();
    
    bool on_drawing_area_draw(const Cairo::RefPtr<Cairo::Context>& cr);
    bool on_timeout_step();

    // Хелперы
    void clear_canvas_data();
    void set_pixel_instant(int x, int y, double r, double g, double b, double a);
    void queue_pixel_task(int x, int y, double r, double g, double b, double a);

    // АЛГОРИТМЫ
    void algo_step_line(int x1, int y1, int x2, int y2, double r, double g, double b);
    void algo_dda(int x1, int y1, int x2, int y2, double r, double g, double b);
    void algo_bresenham_line(int x1, int y1, int x2, int y2, double r, double g, double b);
    void algo_bresenham_circle(int cx, int cy, int radius, double r, double g, double b);
    
    // Бонусы
    void algo_antialiased(int x1, int y1, int x2, int y2); // Ву
    void algo_bezier_quadratic(int x1, int y1, int cx, int cy, int x2, int y2); // Безье
    void algo_castle_pitteway(int x1, int y1, int x2, int y2); // Кастла-Питвея (NEW)

    void push_circle_points(int cx, int cy, int x, int y, double r, double g, double b);

    // GUI элементы
    Gtk::Box m_vbox;
    Gtk::DrawingArea m_drawing_area;
    Gtk::Grid m_grid;

    Gtk::Entry m_x1, m_y1, m_x2, m_y2;
    Gtk::Entry m_cx, m_cy, m_radius;
    Gtk::Entry m_w_entry, m_h_entry;
    
    Gtk::SpinButton m_scale_spin;
    Gtk::SpinButton m_delay_spin;

    // Чекбоксы
    Gtk::CheckButton m_chk_seq, m_chk_dda, m_chk_bres, m_chk_circle;
    Gtk::CheckButton m_chk_aa, m_chk_bezier, m_chk_castle; // Бонусные

    Gtk::Button m_btn_draw, m_btn_clear, m_btn_real, m_btn_resize;

    // Состояние
    int m_canvas_width = 50;
    int m_canvas_height = 50;
    int m_pixel_scale = 9;
    bool m_is_real_line_active = false;

    std::vector<unsigned char> m_pixel_data;
    std::deque<PixelTask> m_tasks;
    sigc::connection m_timeout_conn;
};

RasterApp::RasterApp() : m_vbox(Gtk::ORIENTATION_VERTICAL, 6) {
    set_title("Лаб 3: Растровые алгоритмы (+Кастла-Питвея)");
    set_default_size(950, 750);
    add(m_vbox);

    m_drawing_area.set_size_request(600, 500);
    m_drawing_area.signal_draw().connect(sigc::mem_fun(*this, &RasterApp::on_drawing_area_draw));
    m_vbox.pack_start(m_drawing_area, true, true, 0);

    m_grid.set_column_spacing(10);
    m_grid.set_row_spacing(5);
    m_grid.set_border_width(10);
    m_vbox.pack_start(m_grid, false, false, 0);

    int row = 0;
    // Line Inputs
    m_grid.attach(*Gtk::manage(new Gtk::Label("X1:")), 0, row, 1, 1);
    m_x1.set_text("2"); m_grid.attach(m_x1, 1, row, 1, 1);
    m_grid.attach(*Gtk::manage(new Gtk::Label("Y1:")), 2, row, 1, 1);
    m_y1.set_text("45"); m_grid.attach(m_y1, 3, row, 1, 1);
    row++;

    m_grid.attach(*Gtk::manage(new Gtk::Label("X2:")), 0, row, 1, 1);
    m_x2.set_text("48"); m_grid.attach(m_x2, 1, row, 1, 1);
    m_grid.attach(*Gtk::manage(new Gtk::Label("Y2:")), 2, row, 1, 1);
    m_y2.set_text("30"); m_grid.attach(m_y2, 3, row, 1, 1);
    row++;

    // Circle/Curve Inputs
    m_grid.attach(*Gtk::manage(new Gtk::Label("Center/Control X:")), 0, row, 1, 1);
    m_cx.set_text("25"); m_grid.attach(m_cx, 1, row, 1, 1);
    m_grid.attach(*Gtk::manage(new Gtk::Label("Center/Control Y:")), 2, row, 1, 1);
    m_cy.set_text("10"); m_grid.attach(m_cy, 3, row, 1, 1);
    row++;
    
    m_grid.attach(*Gtk::manage(new Gtk::Label("Radius:")), 0, row, 1, 1);
    m_radius.set_text("15"); m_grid.attach(m_radius, 1, row, 1, 1);
    row++;

    // Canvas Settings
    m_grid.attach(*Gtk::manage(new Gtk::Label("W:")), 0, row, 1, 1);
    m_w_entry.set_text(std::to_string(m_canvas_width)); m_grid.attach(m_w_entry, 1, row, 1, 1);
    m_grid.attach(*Gtk::manage(new Gtk::Label("H:")), 2, row, 1, 1);
    m_h_entry.set_text(std::to_string(m_canvas_height)); m_grid.attach(m_h_entry, 3, row, 1, 1);
    
    m_btn_resize.set_label("Set Size");
    m_btn_resize.signal_clicked().connect(sigc::mem_fun(*this, &RasterApp::on_resize_clicked));
    m_grid.attach(m_btn_resize, 4, row, 1, 1);
    row++;

    // Controls
    m_grid.attach(*Gtk::manage(new Gtk::Label("Scale:")), 0, row, 1, 1);
    m_scale_spin.set_range(2, 100); m_scale_spin.set_increments(1, 5); m_scale_spin.set_value(m_pixel_scale);
    m_scale_spin.signal_value_changed().connect(sigc::mem_fun(*this, &RasterApp::on_scale_changed));
    m_grid.attach(m_scale_spin, 1, row, 1, 1);

    m_grid.attach(*Gtk::manage(new Gtk::Label("Delay(s):")), 2, row, 1, 1);
    m_delay_spin.set_digits(3); m_delay_spin.set_range(0.000, 0.5); m_delay_spin.set_increments(0.001, 0.01); m_delay_spin.set_value(0.005);
    m_grid.attach(m_delay_spin, 3, row, 1, 1);

    // Checkboxes
    int col = 5;
    int r_algo = 0;
    m_chk_seq.set_label("Пошаговый (Red)"); m_grid.attach(m_chk_seq, col, r_algo++, 2, 1);
    m_chk_dda.set_label("ЦДА (Green)"); m_grid.attach(m_chk_dda, col, r_algo++, 2, 1);
    m_chk_bres.set_label("Брезенхем (Blue)"); m_grid.attach(m_chk_bres, col, r_algo++, 2, 1);
    m_chk_circle.set_label("Окр. Брезенхема (Purple)"); m_grid.attach(m_chk_circle, col, r_algo++, 2, 1);
    
    // Бонусы
    m_chk_aa.set_label("Сглаживание By (Black)"); m_grid.attach(m_chk_aa, col, r_algo++, 2, 1);
    m_chk_bezier.set_label("Безье (Orange)"); m_grid.attach(m_chk_bezier, col, r_algo++, 2, 1);
    m_chk_castle.set_label("Кастла-Питвея (Cyan)"); m_grid.attach(m_chk_castle, col, r_algo++, 2, 1);

    row = std::max(row + 1, r_algo + 1);
    
    m_btn_draw.set_label("Нарисовать + Замер времени");
    m_btn_draw.signal_clicked().connect(sigc::mem_fun(*this, &RasterApp::on_draw_clicked));
    m_grid.attach(m_btn_draw, 0, row, 3, 1);

    m_btn_clear.set_label("Очистить");
    m_btn_clear.signal_clicked().connect(sigc::mem_fun(*this, &RasterApp::on_clear_clicked));
    m_grid.attach(m_btn_clear, 3, row, 2, 1);

    m_btn_real.set_label("Векторная линия");
    m_btn_real.signal_clicked().connect(sigc::mem_fun(*this, &RasterApp::on_draw_real_clicked));
    m_grid.attach(m_btn_real, 5, row, 2, 1);

    clear_canvas_data();
    show_all_children();
}

RasterApp::~RasterApp() { if (m_timeout_conn) m_timeout_conn.disconnect(); }

void RasterApp::clear_canvas_data() {
    size_t size = m_canvas_width * m_canvas_height * 4;
    m_pixel_data.assign(size, 255);
}

void RasterApp::on_clear_clicked() {
    if (m_timeout_conn) m_timeout_conn.disconnect();
    m_tasks.clear();
    clear_canvas_data();
    m_drawing_area.queue_draw();
}

void RasterApp::on_resize_clicked() {
    try {
        m_canvas_width = std::stoi(m_w_entry.get_text());
        m_canvas_height = std::stoi(m_h_entry.get_text());
        on_clear_clicked();
    } catch (...) {}
}

void RasterApp::on_scale_changed() {
    m_pixel_scale = (int)m_scale_spin.get_value();
    m_drawing_area.queue_draw();
}

void RasterApp::on_draw_real_clicked() {
    m_is_real_line_active = !m_is_real_line_active;
    m_drawing_area.queue_draw();
}

void RasterApp::set_pixel_instant(int x, int y, double r_new, double g_new, double b_new, double alpha_val) {
    if (x < 0 || x >= m_canvas_width || y < 0 || y >= m_canvas_height) return;
    int index = (y * m_canvas_width + x) * 4;
    unsigned char b_curr = m_pixel_data[index + 0];
    unsigned char g_curr = m_pixel_data[index + 1];
    unsigned char r_curr = m_pixel_data[index + 2];
    unsigned char a_curr = m_pixel_data[index + 3];

    double a_norm = std::clamp(alpha_val, 0.0, 1.0);
    bool is_white = (a_curr == 255 && b_curr == 255 && g_curr == 255 && r_curr == 255);
    unsigned char r_out, g_out, b_out, a_out;

    if (is_white) {
        b_out = (unsigned char)(b_new * 255 * a_norm + b_curr * (1.0 - a_norm));
        g_out = (unsigned char)(g_new * 255 * a_norm + g_curr * (1.0 - a_norm));
        r_out = (unsigned char)(r_new * 255 * a_norm + r_curr * (1.0 - a_norm));
        a_out = 255;
    } else {
        b_out = std::min(255, (int)(b_curr + b_new * 255 * 0.5)); // Simple blend
        g_out = std::min(255, (int)(g_curr + g_new * 255 * 0.5));
        r_out = std::min(255, (int)(r_curr + r_new * 255 * 0.5));
        a_out = 255;
    }
    m_pixel_data[index] = b_out; m_pixel_data[index+1] = g_out;
    m_pixel_data[index+2] = r_out; m_pixel_data[index+3] = a_out;
}

void RasterApp::queue_pixel_task(int x, int y, double r, double g, double b, double a) {
    m_tasks.push_back({x, y, r, g, b, a});
}

bool RasterApp::on_timeout_step() {
    if (m_tasks.empty()) { m_timeout_conn.disconnect(); return false; }
    
    int batch = (m_delay_spin.get_value() < 0.002) ? 20 : 1;
    
    for(int i=0; i<batch && !m_tasks.empty(); ++i) {
        PixelTask t = m_tasks.front();
        m_tasks.pop_front();
        set_pixel_instant(t.x, t.y, t.r, t.g, t.b, t.alpha);
    }
    m_drawing_area.queue_draw();
    return true;
}

void RasterApp::on_draw_clicked() {
    on_clear_clicked();
    try {
        int x1 = std::stoi(m_x1.get_text()), y1 = std::stoi(m_y1.get_text());
        int x2 = std::stoi(m_x2.get_text()), y2 = std::stoi(m_y2.get_text());
        int cx = std::stoi(m_cx.get_text()), cy = std::stoi(m_cy.get_text());
        int r  = std::stoi(m_radius.get_text());

        std::cout << "--- Результаты замеров (мкс) ---" << std::endl;

        if (m_chk_seq.get_active()) {
            auto start = std::chrono::high_resolution_clock::now();
            algo_step_line(x1, y1, x2, y2, 1.0, 0, 0);
            auto end = std::chrono::high_resolution_clock::now();
            std::cout << "Step-by-Step: " << std::chrono::duration_cast<std::chrono::microseconds>(end-start).count() << " us" << std::endl;
        }
        if (m_chk_dda.get_active()) {
            auto start = std::chrono::high_resolution_clock::now();
            algo_dda(x1, y1, x2, y2, 0, 0.8, 0);
            auto end = std::chrono::high_resolution_clock::now();
            std::cout << "DDA: " << std::chrono::duration_cast<std::chrono::microseconds>(end-start).count() << " us" << std::endl;
        }
        if (m_chk_bres.get_active()) {
            auto start = std::chrono::high_resolution_clock::now();
            algo_bresenham_line(x1, y1, x2, y2, 0, 0, 1.0);
            auto end = std::chrono::high_resolution_clock::now();
            std::cout << "Bresenham Line: " << std::chrono::duration_cast<std::chrono::microseconds>(end-start).count() << " us" << std::endl;
        }
        if (m_chk_circle.get_active()) {
            auto start = std::chrono::high_resolution_clock::now();
            algo_bresenham_circle(cx, cy, r, 0.6, 0, 0.6);
            auto end = std::chrono::high_resolution_clock::now();
            std::cout << "Bresenham Circle: " << std::chrono::duration_cast<std::chrono::microseconds>(end-start).count() << " us" << std::endl;
        }
        if (m_chk_aa.get_active()) {
            auto start = std::chrono::high_resolution_clock::now();
            algo_antialiased(x1, y1, x2, y2);
            auto end = std::chrono::high_resolution_clock::now();
            std::cout << "Wu (Antialiased): " << std::chrono::duration_cast<std::chrono::microseconds>(end-start).count() << " us" << std::endl;
        }
        if (m_chk_bezier.get_active()) {
            auto start = std::chrono::high_resolution_clock::now();
            algo_bezier_quadratic(x1, y1, cx, cy, x2, y2);
            auto end = std::chrono::high_resolution_clock::now();
            std::cout << "Bezier Quadratic: " << std::chrono::duration_cast<std::chrono::microseconds>(end-start).count() << " us" << std::endl;
        }
        if (m_chk_castle.get_active()) {
            auto start = std::chrono::high_resolution_clock::now();
            algo_castle_pitteway(x1, y1, x2, y2);
            auto end = std::chrono::high_resolution_clock::now();
            std::cout << "Castle-Pitteway: " << std::chrono::duration_cast<std::chrono::microseconds>(end-start).count() << " us" << std::endl;
        }

        if (!m_tasks.empty()) {
            int delay_ms = std::max(1, (int)(m_delay_spin.get_value() * 1000));
            m_timeout_conn = Glib::signal_timeout().connect(sigc::mem_fun(*this, &RasterApp::on_timeout_step), delay_ms);
        }

    } catch (...) { std::cerr << "Input Error" << std::endl; }
}

// Реализация алгоритмов
void RasterApp::algo_step_line(int x1, int y1, int x2, int y2, double r, double g, double b) {
    int dx = x2 - x1;
    int dy = y2 - y1;
    if (dx == 0 && dy == 0) { queue_pixel_task(x1, y1, r, g, b, 1.0); return; }
    
    if (abs(dx) >= abs(dy)) {
        if (x1 > x2) { std::swap(x1, x2); std::swap(y1, y2); }
        double k = (double)(y2 - y1) / (x2 - x1);
        double y = y1;
        for (int x = x1; x <= x2; x++) {
            queue_pixel_task(x, round(y), r, g, b, 1.0);
            y += k;
        }
    } else {
        if (y1 > y2) { std::swap(x1, x2); std::swap(y1, y2); }
        double k = (double)(x2 - x1) / (y2 - y1);
        double x = x1;
        for (int y = y1; y <= y2; y++) {
            queue_pixel_task(round(x), y, r, g, b, 1.0);
            x += k;
        }
    }
}

void RasterApp::algo_dda(int x1, int y1, int x2, int y2, double r, double g, double b) {
    int dx = x2 - x1;
    int dy = y2 - y1;
    int steps = std::max(std::abs(dx), std::abs(dy));
    if (steps == 0) { queue_pixel_task(x1, y1, r, g, b, 1.0); return; }

    double x_inc = (double)dx / steps;
    double y_inc = (double)dy / steps;
    double x = x1;
    double y = y1;

    for (int i = 0; i <= steps; ++i) {
        queue_pixel_task((int)std::round(x), (int)std::round(y), r, g, b, 1.0);
        x += x_inc;
        y += y_inc;
    }
}

void RasterApp::algo_bresenham_line(int x1, int y1, int x2, int y2, double r, double g, double b) {
    int dx = std::abs(x2 - x1);
    int dy = std::abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

    int x = x1;
    int y = y1;

    while (true) {
        queue_pixel_task(x, y, r, g, b, 1.0);
        if (x == x2 && y == y2) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x += sx; }
        if (e2 < dx) { err += dx; y += sy; }
    }
}

void RasterApp::push_circle_points(int cx, int cy, int x, int y, double r, double g, double b) {
    int pts[8][2] = {
        {cx+x, cy+y}, {cx-x, cy+y}, {cx+x, cy-y}, {cx-x, cy-y},
        {cx+y, cy+x}, {cx-y, cy+x}, {cx+y, cy-x}, {cx-y, cy-x}
    };
    for(auto& p : pts) queue_pixel_task(p[0], p[1], r, g, b, 1.0);
}

void RasterApp::algo_bresenham_circle(int cx, int cy, int radius, double r, double g, double b) {
    int x = 0;
    int y = radius;
    int d = 3 - 2 * radius;
    push_circle_points(cx, cy, x, y, r, g, b);
    while (x < y) {
        if (d < 0) d = d + 4 * x + 6;
        else { d = d + 4 * (x - y) + 10; y--; }
        x++;
        push_circle_points(cx, cy, x, y, r, g, b);
    }
}

// Алгоритм Кастла-Питвея (Лингвистический/Евклидов)
void RasterApp::algo_castle_pitteway(int x1, int y1, int x2, int y2) {
    int dx = x2 - x1;
    int dy = y2 - y1;

    int sx = (dx >= 0) ? 1 : -1;
    int sy = (dy >= 0) ? 1 : -1;

    int a = std::abs(dx);
    int b = std::abs(dy);
    
    bool steep = b > a;
    if (steep) std::swap(a, b);

    std::string m1 = "s"; 
    std::string m2 = "d";

    int x_alg = a - b;
    int y_alg = b;
    
    std::string pattern;
    
    if (y_alg == 0) {
        pattern = "s"; 
    } else if (x_alg == 0) {
        pattern = "d";
    } else {
        while (x_alg != y_alg) {
            if (x_alg > y_alg) {
                x_alg = x_alg - y_alg;
                m2 = m1 + m2;
            } else {
                y_alg = y_alg - x_alg;
                m1 = m2 + m1;
            }
        }
        pattern = m2 + m1;
    }

    int cur_x = x1;
    int cur_y = y1;

    queue_pixel_task(cur_x, cur_y, 0.0, 1.0, 1.0, 1.0);
    
    int repeats = (x_alg == 0) ? a : x_alg; // Если x_alg стал 0 (диагональ), то повторяем 'a' раз 'd'
    if (y_alg == 0) repeats = a; // Для прямых
    
    std::string full_path = "";
    for(int i=0; i < repeats; ++i) full_path += pattern;

    // Проходим по сгенерированной строке
    for (char move : full_path) {
        if (steep) {
            if (move == 's') {
                cur_y += sy; 
            } else {
                cur_y += sy;
                cur_x += sx;
            }
        } else {
            if (move == 's') {
                cur_x += sx;
            } else {
                cur_x += sx;
                cur_y += sy;
            }
        }
        queue_pixel_task(cur_x, cur_y, 0.0, 1.0, 1.0, 1.0);
    }
}

// Сглаживание (Алгоритм Ву)
void RasterApp::algo_antialiased(int x1, int y1, int x2, int y2) {
    auto ipart = [](double x) { return (int)std::floor(x); };
    auto round_ = [](double x) { return (int)std::round(x); };
    auto fpart = [](double x) { return x - std::floor(x); };
    auto rfpart = [&](double x) { return 1.0 - fpart(x); };

    bool steep = std::abs(y2 - y1) > std::abs(x2 - x1);
    if (steep) { std::swap(x1, y1); std::swap(x2, y2); }
    if (x1 > x2) { std::swap(x1, x2); std::swap(y1, y2); }

    int dx = x2 - x1;
    int dy = y2 - y1;
    double gradient = (dx == 0) ? 1.0 : (double)dy / dx;

    int xend = round_(x1);
    double yend = y1 + gradient * (xend - x1);
    double xgap = rfpart(x1 + 0.5);
    int xpxl1 = xend;
    int ypxl1 = ipart(yend);
    
    if (steep) {
        queue_pixel_task(ypxl1, xpxl1, 0,0,0, rfpart(yend) * xgap);
        queue_pixel_task(ypxl1+1, xpxl1, 0,0,0, fpart(yend) * xgap);
    } else {
        queue_pixel_task(xpxl1, ypxl1, 0,0,0, rfpart(yend) * xgap);
        queue_pixel_task(xpxl1, ypxl1+1, 0,0,0, fpart(yend) * xgap);
    }
    double intery = yend + gradient;

    xend = round_(x2);
    yend = y2 + gradient * (xend - x2);
    xgap = fpart(x2 + 0.5);
    int xpxl2 = xend;
    int ypxl2 = ipart(yend);
    
    for (int x = xpxl1 + 1; x < xpxl2; x++) {
        if (steep) {
            queue_pixel_task(ipart(intery), x, 0,0,0, rfpart(intery));
            queue_pixel_task(ipart(intery)+1, x, 0,0,0, fpart(intery));
        } else {
            queue_pixel_task(x, ipart(intery), 0,0,0, rfpart(intery));
            queue_pixel_task(x, ipart(intery)+1, 0,0,0, fpart(intery));
        }
        intery += gradient;
    }
}

// Кривые Безье (Квадратичная)
void RasterApp::algo_bezier_quadratic(int x1, int y1, int cx, int cy, int x2, int y2) {
    int steps = std::max(std::abs(x2-x1), std::abs(y2-y1)) + std::max(std::abs(cx-x1), std::abs(cy-y1));
    if (steps == 0) steps = 1;
    
    for (int i = 0; i <= steps; i++) {
        double t = (double)i / steps;
        double u = 1 - t;
        double x = u*u*x1 + 2*u*t*cx + t*t*x2;
        double y = u*u*y1 + 2*u*t*cy + t*t*y2;
        
        queue_pixel_task(std::round(x), std::round(y), 1.0, 0.5, 0.0, 1.0); 
    }
}

// --- Cairo Draw ---
bool RasterApp::on_drawing_area_draw(const Cairo::RefPtr<Cairo::Context>& cr) {
    Gtk::Allocation allocation = m_drawing_area.get_allocation();
    int w = allocation.get_width();
    int h = allocation.get_height();

    double scale = (double)m_pixel_scale;
    double cw = m_canvas_width * scale;
    double ch = m_canvas_height * scale;
    double ox = (w - cw)/2.0;
    double oy = (h - ch)/2.0;

    cr->save();
    cr->translate(ox, oy);
    cr->scale(scale, scale);
    cr->rectangle(0, 0, m_canvas_width, m_canvas_height);
    cr->clip();

    auto surface = Cairo::ImageSurface::create(&m_pixel_data[0], Cairo::FORMAT_ARGB32, m_canvas_width, m_canvas_height, m_canvas_width*4);
    auto pattern = Cairo::SurfacePattern::create(surface);
    pattern->set_filter(Cairo::FILTER_NEAREST);
    cr->set_source(pattern);
    cr->paint();

    cr->set_source_rgb(0.8, 0.8, 0.8);
    cr->set_line_width(1.0/scale);
    for(int i=0; i<=m_canvas_width; ++i) { cr->move_to(i, 0); cr->line_to(i, m_canvas_height); }
    for(int i=0; i<=m_canvas_height; ++i) { cr->move_to(0, i); cr->line_to(m_canvas_width, i); }
    cr->stroke();
    
    if (m_is_real_line_active) {
        try {
            cr->set_source_rgb(0,0,1); cr->set_line_width(0.2);
            cr->move_to(std::stod(m_x1.get_text())+0.5, std::stod(m_y1.get_text())+0.5);
            cr->line_to(std::stod(m_x2.get_text())+0.5, std::stod(m_y2.get_text())+0.5);
            cr->stroke();
        } catch(...) {}
    }
    cr->restore();

    // Labels
    cr->save();
    cr->translate(ox, oy);
    cr->set_source_rgb(0,0,0);
    cr->set_font_size(std::max(10.0, scale*0.7));
    Cairo::TextExtents te;
    for(int i=0; i<=m_canvas_width; i+=5) {
        std::string t = std::to_string(i);
        cr->get_text_extents(t, te);
        cr->move_to(i*scale - te.width/2, -5);
        cr->show_text(t);
    }
    for(int i=0; i<=m_canvas_height; i+=5) {
        std::string t = std::to_string(i);
        cr->get_text_extents(t, te);
        cr->move_to(-te.width-5, i*scale + te.height/2);
        cr->show_text(t);
    }
    cr->restore();

    return true;
}

int main(int argc, char *argv[]) {
    auto app = Gtk::Application::create(argc, argv, "org.raster.lab3");
    RasterApp win;
    return app->run(win);
}