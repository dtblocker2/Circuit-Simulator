#ifndef OSCILLOSCOPE_H
#define OSCILLOSCOPE_H

#include <vector>
#include <string>
#include <map>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <algorithm>

struct TimePoint {
    double time;
    std::vector<double> nodeVoltages;
};

class Oscilloscope {
    std::vector<TimePoint> buffer;

    void drawChar(std::vector<int>& img, int w, int h, int x0, int y0, char c, int r, int g, int b) const {
        static const unsigned char font[14][5] = {
            {0x3E,0x51,0x49,0x45,0x3E}, // 0
            {0x00,0x42,0x7F,0x40,0x00}, // 1
            {0x42,0x61,0x51,0x49,0x46}, // 2
            {0x21,0x41,0x45,0x4B,0x31}, // 3
            {0x18,0x14,0x12,0x7F,0x10}, // 4
            {0x27,0x45,0x45,0x45,0x39}, // 5
            {0x3C,0x4A,0x49,0x49,0x30}, // 6
            {0x01,0x71,0x09,0x05,0x03}, // 7
            {0x36,0x49,0x49,0x49,0x36}, // 8
            {0x06,0x49,0x49,0x29,0x1E}, // 9
            {0x00,0x00,0x06,0x06,0x00}, // .
            {0x08,0x08,0x08,0x08,0x08}, // -
            {0x00,0x00,0x00,0x00,0x00}, // ' '
            {0x03,0x03,0x00,0x03,0x03}  // :
        };
        int idx = 12;
        if (c >= '0' && c <= '9') idx = c - '0';
        else if (c == '.') idx = 10;
        else if (c == '-') idx = 11;
        else if (c == ':') idx = 13;

        for (int col = 0; col < 5; col++) {
            unsigned char colBits = font[idx][col];
            for (int row = 0; row < 7; row++) {
                if ((colBits >> row) & 1) {
                    int px = x0 + col;
                    int py = y0 + row;
                    if (px >= 0 && px < w && py >= 0 && py < h) {
                        int pIdx = (py * w + px) * 3;
                        img[pIdx] = r; img[pIdx+1] = g; img[pIdx+2] = b;
                    }
                }
            }
        }
    }

    void drawText(std::vector<int>& img, int w, int h, int x0, int y0, const std::string& text) const {
        for (size_t i = 0; i < text.length(); i++) {
            drawChar(img, w, h, x0 + i * 6, y0, text[i], 200, 200, 200);
        }
    }

public:
    void record(double t, const std::vector<double>& v) { buffer.push_back({t, v}); }
    void clear() { buffer.clear(); }

    void generatePPM(const std::string& filename, int targetNode, int imgW = 800, int imgH = 600) const {
        if (buffer.empty()) return;
        std::ofstream ofs(filename);
        if (!ofs) throw std::runtime_error("Cannot write image file.");

        std::vector<double> y_vals;
        for (const auto& tp : buffer) {
            y_vals.push_back((targetNode <= tp.nodeVoltages.size()) ? tp.nodeVoltages[targetNode-1] : 0.0);
        }
        double min_y = *std::min_element(y_vals.begin(), y_vals.end());
        double max_y = *std::max_element(y_vals.begin(), y_vals.end());
        if (std::abs(max_y - min_y) < 1e-9) { max_y += 1.0; min_y -= 1.0; }

        int padL = 60, padR = 20, padT = 20, padB = 40;
        int plotW = imgW - padL - padR;
        int plotH = imgH - padT - padB;

        std::vector<int> img(imgW * imgH * 3, 15); // Dark background

        // Grid lines & Axis numbers
        for (int i = 0; i <= 4; i++) {
            int y = padT + plotH - (i * plotH) / 4;
            double val = min_y + (i * (max_y - min_y)) / 4.0;
            for (int x = padL; x < padL + plotW; x++) {
                int idx = (y * imgW + x) * 3;
                img[idx] = 40; img[idx+1] = 40; img[idx+2] = 40;
            }
            std::stringstream ss; ss << std::fixed << std::setprecision(2) << val;
            drawText(img, imgW, imgH, 5, y - 3, ss.str());
        }
        for (int i = 0; i <= 5; i++) {
            int x = padL + (i * plotW) / 5;
            double timeVal = buffer.front().time + (i * (buffer.back().time - buffer.front().time)) / 5.0;
            for (int y = padT; y < padT + plotH; y++) {
                int idx = (y * imgW + x) * 3;
                img[idx] = 40; img[idx+1] = 40; img[idx+2] = 40;
            }
            std::stringstream ss; ss << std::fixed << std::setprecision(3) << timeVal;
            drawText(img, imgW, imgH, x - 12, padT + plotH + 10, ss.str());
        }

        // Waveform Trace
        int num_points = y_vals.size();
        for (int x = 0; x < plotW; x++) {
            int idx = (x * num_points) / plotW;
            double norm = (y_vals[std::min(idx, num_points-1)] - min_y) / (max_y - min_y);
            int y = padT + plotH - 1 - static_cast<int>(norm * (plotH - 1));
            y = std::clamp(y, padT, padT + plotH - 1);
            
            int pIdx = (y * imgW + (padL + x)) * 3;
            img[pIdx] = 0; img[pIdx+1] = 255; img[pIdx+2] = 0; // Green Phosphor
        }

        ofs << "P3\n" << imgW << " " << imgH << "\n255\n";
        for (int i = 0; i < imgW * imgH * 3; i += 3) {
            ofs << img[i] << " " << img[i+1] << " " << img[i+2] << " ";
            if ((i/3) % imgW == imgW - 1) ofs << "\n";
        }

        // EMIT METADATA FOR GUI CURSOR
        std::cout << "[OSC_META] t_start=" << buffer.front().time << " t_end=" << buffer.back().time
                  << " y_min=" << min_y << " y_max=" << max_y 
                  << " padL=" << padL << " padR=" << padR << " padT=" << padT << " padB=" << padB
                  << " imgW=" << imgW << " imgH=" << imgH << "\n";
        std::cout << "[OSC] Picture generated successfully: " << filename << "\n";
    }

    void exportJSON(int targetNode) const {
        if (buffer.empty()) {
            std::cout << "[OSC_DATA] {\"node\":" << targetNode << ",\"times\":[],\"voltages\":[]}\n";
            return;
        }

        std::vector<double> y_vals;
        for (const auto& tp : buffer) {
            y_vals.push_back((targetNode > 0 && targetNode <= static_cast<int>(tp.nodeVoltages.size()))
                ? tp.nodeVoltages[targetNode - 1] : 0.0);
        }
        double min_y = *std::min_element(y_vals.begin(), y_vals.end());
        double max_y = *std::max_element(y_vals.begin(), y_vals.end());
        if (std::abs(max_y - min_y) < 1e-9) { max_y += 1.0; min_y -= 1.0; }

        std::cout << "[OSC_DATA] {\"node\":" << targetNode
                  << ",\"t_start\":" << buffer.front().time
                  << ",\"t_end\":" << buffer.back().time
                  << ",\"y_min\":" << min_y
                  << ",\"y_max\":" << max_y
                  << ",\"times\":[";
        for (size_t i = 0; i < buffer.size(); i++) {
            if (i > 0) std::cout << ",";
            std::cout << buffer[i].time;
        }
        std::cout << "],\"voltages\":[";
        for (size_t i = 0; i < y_vals.size(); i++) {
            if (i > 0) std::cout << ",";
            std::cout << y_vals[i];
        }
        std::cout << "]}\n";
    }
};

#endif // OSCILLOSCOPE_H