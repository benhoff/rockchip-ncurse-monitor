# System Monitor

A terminal-based system monitor application that displays various system statistics using ncurses. This application shows CPU usage, RAM usage, Swap usage, CPU temperature, NPU load, RGA load, GPU load, and fan speed in real-time with progress bars.

Based on: https://github.com/Qengineering/rtop-Ubuntu


## Dependencies

C++ Compiler: A C++ compiler that supports C++11 or later (e.g., g++, clang++).

ncurses Library: The ncurses library is required for terminal handling.

## Installation
Install Dependencies
On Ubuntu/Debian:

`sudo apt-get update`

`sudo apt-get install build-essential libncurses5-dev libncursesw5-dev`

On Fedora:

`sudo dnf install ncurses-devel`

On Arch Linux:

`sudo pacman -S ncurses`

## Compile the Program

Download the Source Code

Save the system_monitor.cpp file to your local machine.

Compile Using g++

Open a terminal and navigate to the directory containing system_monitor.cpp. Compile the program with the following command:
`g++ -o system_monitor system_monitor.cpp -lncurses -std=c++11`


## Run the Program

Execute the compiled binary:

`./system_monitor`

To get NPU and RGA statistics:
`sudo ./system_monitor`

The application will start displaying system statistics in real-time.
Press q at any time to exit the program.

## Notes

Permissions:

Some system files (e.g., /sys/kernel/debug/) may require root permissions to read.

If you encounter permission errors, run the program with elevated privileges:

        `sudo ./system_monitor`

Warning: Running programs as root can be risky. Ensure you trust the source code before executing it with sudo.

System Compatibility:
        Designed for Linux systems with standard /proc and /sys interfaces.
        Availability of hardware components like NPU, RGA, or GPU may vary.
        The program handles missing components gracefully by skipping their display.

Error Handling:
        The program includes basic error handling for file operations.
        If certain system files are not accessible, corresponding sections will not display data.

Terminal Size:
        For optimal display, run the program in a terminal window with sufficient width (at least 80 characters).

## Customization

Adjusting Display:
        Modify the drawProgressBar function in system_monitor.cpp to change the appearance of progress bars.
        Adjust the width parameter to change the width of the bars.

Adding Features:
        Extend the application by adding new system metrics.
        Use existing functions as templates for reading and displaying additional data.

Improving Performance:

Adjust the update interval by changing the sleep duration in the main loop:

`std::this_thread::sleep_for(std::chrono::seconds(1)); // Change '1' to desired number of seconds`


