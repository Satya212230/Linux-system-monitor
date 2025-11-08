#include <ncurses.h>
#include <unistd.h>
#include <fstream>
#include <string>
#include <sstream>
#include <iomanip>

float getCPUUsage() {
    static long prevIdle = 0, prevTotal = 0;
    std::ifstream file("/proc/stat");
    std::string line;
    std::getline(file, line);
    std::istringstream ss(line);
    std::string cpu;
    long user, nice, system, idle, iowait, irq, softirq, steal;
    ss >> cpu >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;

    long idleTime = idle + iowait;
    long totalTime = user + nice + system + idle + iowait + irq + softirq + steal;

    long diffIdle = idleTime - prevIdle;
    long diffTotal = totalTime - prevTotal;
    float cpuUsage = (1.0 - (float)diffIdle / diffTotal) * 100.0;

    prevIdle = idleTime;
    prevTotal = totalTime;

    return cpuUsage;
}

float getMemoryUsage() {
    std::ifstream file("/proc/meminfo");
    std::string key;
    long totalMem, freeMem, availableMem;
    while (file >> key) {
        if (key == "MemTotal:") file >> totalMem;
        else if (key == "MemAvailable:") file >> availableMem;
    }
    return (1 - (float)availableMem / totalMem) * 100.0;
}

void drawBar(int y, int x, float percent, int width, int colorPair) {
    int filled = (int)(percent / 100.0 * width);
    attron(COLOR_PAIR(colorPair));
    for (int i = 0; i < width; i++) {
        mvprintw(y, x + i, (i < filled) ? "█" : " ");
    }
    attroff(COLOR_PAIR(colorPair));
}

int main() {
    initscr();
    start_color();
    use_default_colors();
    noecho();
    curs_set(FALSE);
    nodelay(stdscr, TRUE);

    // Define colors
    init_pair(1, COLOR_GREEN, -1);
    init_pair(2, COLOR_YELLOW, -1);
    init_pair(3, COLOR_RED, -1);
    init_pair(4, COLOR_CYAN, -1);

    while (true) {
        clear();
        box(stdscr, 0, 0);
        mvprintw(1, 2, "🔥 LINUX SYSTEM MONITOR 🔥");
        mvprintw(2, 2, "Press 'q' to quit");

        float cpu = getCPUUsage();
        float mem = getMemoryUsage();

        int color = (cpu < 50) ? 1 : (cpu < 80 ? 2 : 3);
        mvprintw(4, 2, "CPU Usage: %.2f%%", cpu);
        drawBar(5, 2, cpu, 50, color);

        color = (mem < 50) ? 1 : (mem < 80 ? 2 : 3);
        mvprintw(7, 2, "Memory Usage: %.2f%%", mem);
        drawBar(8, 2, mem, 50, color);

        refresh();
        usleep(500000); // 0.5s refresh rate

        int ch = getch();
        if (ch == 'q') break;
    }

    endwin();
    return 0;
}
