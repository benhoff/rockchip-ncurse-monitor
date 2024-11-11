#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <unistd.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <ncurses.h>
#include <sys/types.h>
#include <dirent.h>
#include <cstring>
#include <regex>
#include <chrono>
#include <thread>
#include <algorithm>
#include <cctype>

class SystemMonitor {
public:
    SystemMonitor();
    void update();
    void display();

private:
    int nrCPUs;
    int nrNPUs;
    int nrRGAs;
    std::vector<std::vector<long>> currStats;
    std::vector<std::vector<long>> prevStats;
    std::vector<double> cpuLoad;
    std::vector<double> cpuFreq;
    double ramUsagePercent;
    double totalRAMGB;
    double swapUsagePercent;
    double totalSwapGB;
    double cpuTempC;
    double cpuTempF;
    std::vector<int> npuLoad;
    double npuFreq;
    std::vector<int> rgaLoad;
    std::vector<double> rgaFreq;
    int gpuLoad;
    double gpuFreq;
    int fanSpeedPercent;
    std::string gpuLoadPath;
    std::string npuLoadPath;
    std::string fanLoadPath;

    // Member functions
    int getNumberOfCores();
    std::vector<long> readCPUStats(int cpuNumber);
    double calculateCPULoad(const std::vector<long>& prevStats, const std::vector<long>& currStats);
    double readCPUFrequency(int cpuNumber);
    void readCPULoad();
    void readCPUFreq();
    void readRAM();
    void readTemp();
    void readNpuLoad();
    void readNpuFreq();
    void readRgaLoad();
    void readRgaFreq();
    void readGpuLoad();
    void readFanSpeed();
    void findGPULoadPath();
    void findNPULoadPath();
    void findPwmFanDevice();
    void drawProgressBar(int row, int col, int width, double percent, const std::string& label);
};

SystemMonitor::SystemMonitor() {
    nrCPUs = getNumberOfCores();
    currStats.resize(nrCPUs);
    prevStats.resize(nrCPUs);
    cpuLoad.resize(nrCPUs);
    cpuFreq.resize(nrCPUs);

    for (int i = 0; i < nrCPUs; ++i) {
        prevStats[i] = readCPUStats(i);
    }

    npuLoadPath = "";
    gpuLoadPath = "";
    fanLoadPath = "";

    findNPULoadPath();
    findGPULoadPath();
    findPwmFanDevice();

    nrNPUs = 0;
    nrRGAs = 0;
    npuFreq = 0.0;
    gpuLoad = 0;
    gpuFreq = 0.0;
    fanSpeedPercent = 0;
}

int SystemMonitor::getNumberOfCores() {
    return sysconf(_SC_NPROCESSORS_ONLN);
}

std::vector<long> SystemMonitor::readCPUStats(int cpuNumber) {
    std::ifstream file("/proc/stat");
    std::string line;
    std::vector<long> stats;

    if (file.is_open()) {
        while (getline(file, line)) {
            std::istringstream iss(line);
            std::string cpuLabel;
            iss >> cpuLabel;
            if (cpuLabel == "cpu" + std::to_string(cpuNumber)) {
                long value;
                while (iss >> value) {
                    stats.push_back(value);
                }
                break;
            }
        }
        file.close();
    }

    return stats;
}

double SystemMonitor::calculateCPULoad(const std::vector<long>& prevStats, const std::vector<long>& currStats) {
    long prevIdle = prevStats[3] + prevStats[4];
    long currIdle = currStats[3] + currStats[4];

    long prevNonIdle = prevStats[0] + prevStats[1] + prevStats[2] + prevStats[5] + prevStats[6] + prevStats[7];
    long currNonIdle = currStats[0] + currStats[1] + currStats[2] + currStats[5] + currStats[6] + currStats[7];

    long prevTotal = prevIdle + prevNonIdle;
    long currTotal = currIdle + currNonIdle;

    long totalDelta = currTotal - prevTotal;
    long idleDelta = currIdle - prevIdle;

    return (double)(totalDelta - idleDelta) / totalDelta * 100.0;
}

double SystemMonitor::readCPUFrequency(int cpuNumber) {
    std::string path = "/sys/devices/system/cpu/cpu" + std::to_string(cpuNumber) + "/cpufreq/scaling_cur_freq";
    std::ifstream file(path);
    long frequency = 0;

    if (file.is_open()) {
        file >> frequency;
        file.close();
    }

    return static_cast<double>(frequency) / 1000000.0;
}

void SystemMonitor::readCPULoad() {
    for (int i = 0; i < nrCPUs; ++i) {
        currStats[i] = readCPUStats(i);
        cpuLoad[i] = calculateCPULoad(prevStats[i], currStats[i]);
        prevStats[i] = currStats[i];
    }
}

void SystemMonitor::readCPUFreq() {
    for (int i = 0; i < nrCPUs; ++i) {
        cpuFreq[i] = readCPUFrequency(i);
    }
}

void SystemMonitor::readRAM() {
    std::ifstream file("/proc/meminfo");
    std::unordered_map<std::string, long> memInfo;
    std::string key;
    long value;
    std::string unit;

    if (file.is_open()) {
        while (file >> key >> value >> unit) {
            key.pop_back(); // Remove trailing ':'
            memInfo[key] = value;
        }
        file.close();
    }

    long totalRAM = memInfo["MemTotal"];            // Total RAM in kB
    long availableRAM = memInfo["MemAvailable"];    // Available RAM in kB
    long usedRAM = totalRAM - availableRAM;         // Used RAM in kB
    long totalSwap = memInfo["SwapTotal"];          // Total Swap in kB
    long freeSwap = memInfo["SwapFree"];            // Free Swap in kB
    long usedSwap = totalSwap - freeSwap;           // Used Swap in kB

    ramUsagePercent = 100.0 * usedRAM / totalRAM;
    totalRAMGB = totalRAM / (1024.0 * 1024.0);
    swapUsagePercent = (totalSwap > 0) ? 100.0 * usedSwap / totalSwap : 0.0;
    totalSwapGB = totalSwap / (1024.0 * 1024.0);
}

void SystemMonitor::readTemp() {
    std::string cpuTempPath = "/sys/class/thermal/thermal_zone0/temp";
    std::ifstream file(cpuTempPath);
    double temp = 0.0;

    if (file.is_open()) {
        file >> temp;
        file.close();
    }

    cpuTempC = temp / 1000.0;
    cpuTempF = (cpuTempC * 9.0 / 5.0) + 32;
}

void SystemMonitor::readNpuLoad() {
    npuLoad.clear();
    std::ifstream file("/sys/kernel/debug/rknpu/load");
    if (file.is_open()) {
        std::string line;
        getline(file, line);
        file.close();

        std::regex core_regex(R"(Core\d+:\s+(\d+)%?)");
        std::regex total_regex(R"((\d+)%?)");
        std::smatch match;

        std::string::const_iterator searchStart(line.cbegin());
        while (regex_search(searchStart, line.cend(), match, core_regex)) {
            npuLoad.push_back(std::stoi(match[1]));
            searchStart = match.suffix().first;
        }

        if (npuLoad.empty() && regex_search(line, match, total_regex)) {
            npuLoad.push_back(std::stoi(match[1]));
        }

        nrNPUs = npuLoad.size();
    }
}

void SystemMonitor::readNpuFreq() {
    std::ifstream file(npuLoadPath);
    long frequency = 0;

    if (file.is_open()) {
        file >> frequency;
        file.close();
    }

    npuFreq = static_cast<double>(frequency) / 1000000000.0;
}

void SystemMonitor::readRgaLoad() {
    rgaLoad.clear();
    std::ifstream file("/sys/kernel/debug/rkrga/load");
    if (file.is_open()) {
        std::string line;
        while (getline(file, line)) {
            if (line.find("load =") != std::string::npos) {
                if (line.find("== load ==") == std::string::npos) {
                    size_t pos = line.find("=");
                    std::string loadStr = line.substr(pos + 1);
                    loadStr = loadStr.substr(0, loadStr.find("%"));
                    rgaLoad.push_back(std::stoi(loadStr));
                }
            }
        }
        file.close();
    }
}

void SystemMonitor::readRgaFreq() {
    rgaFreq.clear();
    std::ifstream file("/sys/kernel/debug/clk/clk_summary");
    if (file.is_open()) {
        std::string content((std::istreambuf_iterator<char>(file)),
                            (std::istreambuf_iterator<char>()));
        file.close();

        std::regex regex_aclk(R"(aclk_rga\d*[_\d]*\s+\d+\s+\d+\s+\d+\s+(\d+))");
        std::smatch match;
        std::string::const_iterator searchStart(content.cbegin());

        while (regex_search(searchStart, content.cend(), match, regex_aclk)) {
            int freq = std::stoi(match[1]);
            rgaFreq.push_back(static_cast<double>(freq) / 1000000000.0);
            searchStart = match.suffix().first;
        }

        nrRGAs = rgaFreq.size();
    }
}

void SystemMonitor::readGpuLoad() {
    std::ifstream file(gpuLoadPath);
    if (file.is_open()) {
        std::string line;
        getline(file, line);
        file.close();

        std::regex regex_gpu(R"((\d+)@(\d+))");
        std::smatch match;

        if (std::regex_search(line, match, regex_gpu)) {
            gpuLoad = std::stoi(match[1]);
            gpuFreq = std::stod(match[2]) / 1000000000.0;
        }
    }
}

void SystemMonitor::readFanSpeed() {
    std::ifstream file(fanLoadPath);
    if (file.is_open()) {
        int pwm = 0;
        file >> pwm;
        file.close();
        // Calculate the fan speed percentage
        fanSpeedPercent = ((pwm + 1) / 256.0) * 100.0;
    }
}

void SystemMonitor::findNPULoadPath() {
    DIR* dir = opendir("/sys/class/devfreq/");
    if (dir) {
        struct dirent* ent;
        while ((ent = readdir(dir)) != nullptr) {
            if (strstr(ent->d_name, ".npu")) {
                npuLoadPath = std::string("/sys/class/devfreq/") + ent->d_name + "/cur_freq";
                break;
            }
        }
        closedir(dir);
    }
}

void SystemMonitor::findGPULoadPath() {
    DIR* dir = opendir("/sys/class/devfreq/");
    if (dir) {
        struct dirent* ent;
        while ((ent = readdir(dir)) != nullptr) {
            if (strstr(ent->d_name, ".gpu")) {
                gpuLoadPath = std::string("/sys/class/devfreq/") + ent->d_name + "/load";
                break;
            }
        }
        closedir(dir);
    }
}

void SystemMonitor::findPwmFanDevice() {
    const std::string basePath = "/sys/devices/platform/pwm-fan/";
    DIR* dir = opendir((basePath + "hwmon/").c_str());
    if (dir) {
        struct dirent* ent;
        while ((ent = readdir(dir)) != nullptr) {
            if (strncmp(ent->d_name, "hwmon", 5) == 0) {
                std::string hwmonPath = basePath + "hwmon/" + ent->d_name;
                std::string pwmPath = hwmonPath + "/pwm1";
                std::ifstream file(pwmPath);
                if (file.good()) {
                    fanLoadPath = pwmPath;
                    break;
                }
            }
        }
        closedir(dir);
}

void SystemMonitor::update() {
    readCPULoad();
    readCPUFreq();
    readRAM();
    readTemp();

    if (!npuLoadPath.empty()) {
        readNpuLoad();
        readNpuFreq();
    }

    if (!gpuLoadPath.empty()) {
        readGpuLoad();
    }

    if (!fanLoadPath.empty()) {
        readFanSpeed();
    }

    readRgaLoad();
    readRgaFreq();
}

void SystemMonitor::drawProgressBar(int row, int col, int width, double percent, const std::string& label) {
    mvprintw(row, col, "%s", label.c_str());
    int barWidth = width - label.size() - 10; // Adjust for label and percentage display
    int filled = (percent / 100.0) * barWidth;
    mvprintw(row, col + label.size(), " [");
    for (int i = 0; i < barWidth; ++i) {
        if (i < filled)
            addch('=');
        else
            addch(' ');
    }
    addch(']');
    mvprintw(row, col + width - 6, "%5.1f%%", percent);
}

void SystemMonitor::display() {
    int row = 0;

    // Display CPU usage
    for (int i = 0; i < nrCPUs; ++i) {
        std::ostringstream oss;
        oss << "CPU" << i << " (" << std::fixed << std::setprecision(2) << cpuFreq[i] << "GHz):";
        drawProgressBar(row++, 0, 70, cpuLoad[i], oss.str());
    }
    row++;

    // Display RAM usage
    drawProgressBar(row++, 0, 70, ramUsagePercent, "RAM Usage:");
    mvprintw(row++, 0, "Total RAM: %.2f GB", totalRAMGB);
    row++;

    // Display Swap usage
    if (totalSwapGB > 0.0) {
        drawProgressBar(row++, 0, 70, swapUsagePercent, "Swap Usage:");
        mvprintw(row++, 0, "Total Swap: %.2f GB", totalSwapGB);
	row++;
    }

    mvprintw(row++, 0, "CPU Temperature: %.1f °C (%.1f °F)", cpuTempC, cpuTempF);

    // Display NPU load
    if (!npuLoad.empty()) {
        for (size_t i = 0; i < npuLoad.size(); ++i) {
            std::ostringstream oss;
            oss << "NPU" << i << " (" << std::fixed << std::setprecision(2) << npuFreq << "GHz):";
            drawProgressBar(row++, 0, 70, npuLoad[i], oss.str());
        }
	row++;
    }

    // Display RGA load
    if (!rgaLoad.empty()) {
        for (size_t i = 0; i < rgaLoad.size() && i < rgaFreq.size(); ++i) {
            std::ostringstream oss;
            oss << "RGA" << i << " (" << std::fixed << std::setprecision(2) << rgaFreq[i] << "GHz):";
            drawProgressBar(row++, 0, 70, rgaLoad[i], oss.str());
        }
	row++;
    }


    // Display GPU load
    if (gpuFreq > 0.0) {
        std::ostringstream oss;
        oss << "GPU (" << std::fixed << std::setprecision(2) << gpuFreq << "GHz):";
        drawProgressBar(row++, 0, 70, gpuLoad, oss.str());
    }
    row++;

    // Display Fan speed
    if (!fanLoadPath.empty()) {
        std::ostringstream oss;
        oss << "Fan Speed:";
        drawProgressBar(row++, 0, 70, fanSpeedPercent, oss.str());
        row++; // Add a blank line after Fan speed
    }

    mvprintw(row++, 0, "Press 'q' to quit.");
}

int main() {
    // Initialize ncurses
    initscr();            // Start curses mode
    cbreak();             // Line buffering disabled
    noecho();             // Don't echo() while we do getch
    nodelay(stdscr, TRUE); // Non-blocking input
    curs_set(0);          // Hide the cursor

    // Create SystemMonitor object
    SystemMonitor monitor;

    // Main loop
    while (true) {
        monitor.update();

        // Clear the screen
        clear();

        // Draw the updated data
        monitor.display();

        // Refresh the screen
        refresh();

        // Sleep for 1 second
        std::this_thread::sleep_for(std::chrono::seconds(1));

        // Check for 'q' key press to quit
        int ch = getch();
        if (ch == 'q' || ch == 'Q')
            break;
    }

    // End ncurses mode
    endwin();
    return 0;
}

