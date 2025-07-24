#pragma once
#include <chrono>
#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>

class Timer {
public:
    Timer(const std::string& label = "")
            : m_Label(label), m_Start(std::chrono::high_resolution_clock::now()), m_Stopped(false) {}

    ~Timer() {
        stop();
    }

    void stop() {
        if (m_Stopped) return;

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = end - m_Start;
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();

        std::ostringstream output;
        output << "[Timer] " << m_Label << " took ";

        if (ms >= 60000) {
            // Format as minutes
            double minutes = ms / 60000.0;
            output << std::fixed << std::setprecision(2) << minutes << " min";
        } else if (ms >= 1000) {
            // Format as seconds
            double seconds = ms / 1000.0;
            output << std::fixed << std::setprecision(2) << seconds << " s";
        } else if (ms > 0) {
            // Milliseconds
            output << ms << " ms";
        } else if (us > 0) {
            // Microseconds
            output << us << " µs";
        } else {
            // Nanoseconds
            output << ns << " ns";
        }

        std::cout << output.str() << std::endl;

        m_Stopped = true;
    }

private:
    std::string m_Label;
    std::chrono::high_resolution_clock::time_point m_Start;
    bool m_Stopped;
};