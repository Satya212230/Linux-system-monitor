// SPDX-License-Identifier: MIT
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <thread>
#include <chrono>
#include <algorithm>
#include <csignal>
#include <ncurses.h>
#include <dirent.h>
#include <unistd.h>

using namespace std;
using namespace std::chrono;

struct Process {
    int pid;
    string name;
    double cpu;
    double mem;
    unsigned long long utime, stime;

    unsigned long long total_time() const { return utime + stime; }
};

map<int, unsigned long long> prev_proc_time;
unsigned long long prev_total_jiffies = 0;

unsigned long long get_total_jiffies() {
    ifstream file("/proc/stat");
    string cpu;
    unsigned long long user, nice, system, idle, iowait, irq, softirq, steal;
    file >> cpu >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;
    return user + nice + system + idle + iowait + irq + softirq + steal;
}

vector<Process> gather_processes() {
    vector<Process> procs;
    DIR *dir = opendir("/proc");
    if (!dir) return procs;

    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        if (!isdigit(ent->d_name[0])) continue;
        int pid = atoi(ent->d_name);
        string stat_path = "/proc/" + string(ent->d_name) + "/stat";
        ifstream stat(stat_path);
        if (!stat) continue;
        Process p;
        p.pid = pid;
        string tmp;
        stat >> tmp >> p.name >> tmp >> tmp >> tmp >> tmp >> tmp >> tmp >> tmp >> tmp >> tmp >> tmp >> tmp
             >> p.utime >> p.stime;
        p.name = p.name.substr(1, p.name.size() - 2);

        // memory usage
        string status_path = "/proc/" + string(ent->d_name) + "/status";
        ifstream status(status_path);
        string line;
        while (getline(status, line)) {
            if (line.find("VmRSS:") != string::npos) {
                stringstream ss(line);
                string key;
                double val;
                string unit;
                ss >> key >> val >> unit;
                p.mem = val / 1024.0; // MB
                break;
            }
        }
        procs.push_back(p);
    }
    closedir(dir);
    return procs;
}

void draw_header(WINDOW *win) {
    wattron(win, A_BOLD);
    mvwprintw(win, 0, 0, "PID\tCPU%%\tMEM(MB)\tNAME");
    wattroff(win, A_BOLD);
    wrefresh(win);
}

void draw_stats(WINDOW *win, unsigned long long total_jiffies) {
    mvwprintw(win, 0, 0, "Total CPU Jiffies: %llu", total_jiffies);
    wrefresh(win);
}

#define REFRESH_SEC 3

int main() {
    initscr(); cbreak(); noecho(); keypad(stdscr, TRUE); curs_set(FALSE);
    if (!has_colors()) start_color();
    nodelay(stdscr, TRUE); // non-blocking getch
    int rows, cols; getmaxyx(stdscr, rows, cols);

    int header_h = 1;
    int stats_h = 1;
    int list_h = rows - header_h - stats_h;

    WINDOW *header = newwin(header_h, cols, 0, 0);
    WINDOW *listw = newwin(list_h, cols, header_h, 0);
    WINDOW *statw = newwin(stats_h, cols, header_h + list_h, 0);

    draw_header(header);
    unsigned long long total_jiffies = get_total_jiffies();
    prev_total_jiffies = total_jiffies;

    auto procs = gather_processes();
    for (auto &p : procs) prev_proc_time[p.pid] = p.total_time();

    int selected = 0;

    while (true) {
        std::this_thread::sleep_for(milliseconds(200));
        int ch = getch();
        if (ch == 'q') break;
        if (ch == KEY_UP && selected > 0) selected--;
        else if (ch == KEY_DOWN) selected++;

        auto new_procs = gather_processes();
        unsigned long long new_total = get_total_jiffies();
        unsigned long long total_diff = new_total - prev_total_jiffies;

        for (auto &p : new_procs) {
            unsigned long long prev = prev_proc_time[p.pid];
            unsigned long long cur = p.total_time();
            unsigned long long diff = (cur > prev) ? (cur - prev) : 0;
            p.cpu = (total_diff > 0) ? (100.0 * diff / total_diff) : 0;
            prev_proc_time[p.pid] = cur;
        }

        sort(new_procs.begin(), new_procs.end(), [](auto &a, auto &b) { return a.cpu > b.cpu; });

        werase(listw);
        for (int i = 0; i < (int)new_procs.size() && i < list_h - 1; ++i) {
            auto &p = new_procs[i];
            if (i == selected) wattron(listw, A_REVERSE);
            mvwprintw(listw, i, 0, "%d\t%.2f\t%.1f\t%s", p.pid, p.cpu, p.mem, p.name.c_str());
            if (i == selected) wattroff(listw, A_REVERSE);
        }
        wrefresh(listw);
        draw_stats(statw, new_total);
        prev_total_jiffies = new_total;
    }

    endwin();
    return 0;
}
