/**
 * @file interrupts.hpp
 * @author Nawal Musameh, Shahd Elsaman
 * @brief Extended full version for OS scheduling practice
 */

#ifndef INTERRUPTS_HPP_
#define INTERRUPTS_HPP_

#include<iostream>
#include<fstream>
#include<string>
#include<vector>
#include<tuple>
#include<random>
#include<utility>
#include<sstream>
#include<iomanip>
#include<algorithm>

// PROCESS STATES
enum states {
    NEW,
    READY,
    RUNNING,
    WAITING,
    TERMINATED,
    NOT_ASSIGNED
};

inline std::ostream& operator<<(std::ostream& os, const enum states& s) {
    std::string state_names[] = {
        "NEW",
        "READY",
        "RUNNING",
        "WAITING",
        "TERMINATED",
        "NOT_ASSIGNED"
    };
    return (os << state_names[s]);
}

// ================================
// MEMORY PARTITIONS
// ================================
struct memory_partition{
    unsigned int    partition_number;
    unsigned int    size;
    int             occupied;
} memory_paritions[] = {
    {1, 40, -1},
    {2, 25, -1},
    {3, 15, -1},
    {4, 10, -1},
    {5, 8, -1},
    {6, 2, -1}
};

// PCB (EXTENDED VERSION)
struct PCB{
    int             PID;
    unsigned int    size;
    unsigned int    arrival_time;
    int             start_time;
    unsigned int    processing_time;
    unsigned int    remaining_time;
    int             partition_number;
    enum states     state;
    unsigned int    io_freq;
    unsigned int    io_duration;

    // --------- ADDED FOR FULL SIMULATOR ---------
    unsigned int    remaining_io_time;   // for WAITING state
    int             priority;            // for priority scheduling

    // --------- METRICS TRACKING ---------
    unsigned int    completion_time;
    unsigned int    total_wait_time;
    unsigned int    last_ready_time;
};

// STRING HELPERS
inline std::vector<std::string> split_delim(std::string input, std::string delim) {
    std::vector<std::string> tokens;
    std::size_t pos = 0;
    std::string token;

    while ((pos = input.find(delim)) != std::string::npos) {
        token = input.substr(0, pos);
        tokens.push_back(token);
        input.erase(0, pos + delim.length());
    }
    tokens.push_back(input);
    return tokens;
}

// PCB PRINTING
inline std::string print_PCB(std::vector<PCB> _PCB) {
    const int tableWidth = 83;
    std::stringstream buffer;

    buffer << "+" << std::setfill('-') << std::setw(tableWidth) << "+\n";

    buffer << "|"
           << std::setw(4) << "PID" << " |"
           << std::setw(11) << "Partition" << " |"
           << std::setw(5) << "Size" << " |"
           << std::setw(13) << "Arrival Time" << " |"
           << std::setw(11) << "Start Time" << " |"
           << std::setw(14) << "Remaining" << " |"
           << std::setw(11) << "State" << " |\n";

    buffer << "+" << std::setfill('-') << std::setw(tableWidth) << "+\n";

    for (const auto& program : _PCB) {
        buffer << "|"
               << std::setw(4) << program.PID << " |"
               << std::setw(11) << program.partition_number << " |"
               << std::setw(5) << program.size << " |"
               << std::setw(13) << program.arrival_time << " |"
               << std::setw(11) << program.start_time << " |"
               << std::setw(14) << program.remaining_time << " |"
               << std::setw(11) << program.state << " |\n";
    }

    buffer << "+" << std::setfill('-') << std::setw(tableWidth) << "+\n";
    return buffer.str();
}

inline std::string print_PCB(PCB _PCB) {
    return print_PCB(std::vector<PCB>{_PCB});
}

// EXECUTION LOG PRINTING
inline std::string print_exec_header() {
    const int tableWidth = 49;
    std::stringstream buffer;

    buffer << "+" << std::setfill('-') << std::setw(tableWidth) << "+\n";
    buffer << "|"
           << std::setw(18) << "Time of Transition" << " |"
           << std::setw(3) << "PID" << " |"
           << std::setw(10) << "Old State" << " |"
           << std::setw(10) << "New State" << " |\n";
    buffer << "+" << std::setfill('-') << std::setw(tableWidth) << "+\n";

    return buffer.str();
}

inline std::string print_exec_status(unsigned int current_time, int PID,
                                     states old_state, states new_state) {
    std::stringstream buffer;
    buffer << "|"
           << std::setw(18) << current_time << " |"
           << std::setw(3)  << PID << " |"
           << std::setw(10) << old_state << " |"
           << std::setw(10) << new_state << " |\n";
    return buffer.str();
}

inline std::string print_exec_footer() {
    const int tableWidth = 49;
    std::stringstream buffer;
    buffer << "+" << std::setfill('-') << std::setw(tableWidth) << "+\n";
    return buffer.str();
}

//Writes a string to a file
// FILE OUTPUT
inline void write_output(std::string execution, const char* filename) {
    std::ofstream output_file(filename);
    if (output_file.is_open()) {
        output_file << execution;
        output_file.close();
    } else {
        std::cerr << "Error opening file!\n";
    }
}
//--------------------------------------------FUNCTIONS FOR THE "OS"-------------------------------------

//Assign memory partition to program
// MEMORY MANAGEMENT
inline bool assign_memory(PCB &program) {
    for(int i = 5; i >= 0; i--) {
        if(program.size <= memory_paritions[i].size &&
           memory_paritions[i].occupied == -1) {
            memory_paritions[i].occupied = program.PID;
            program.partition_number = memory_paritions[i].partition_number;
            return true;
        }
    }
    return false;
}

//Free a memory partition
inline bool free_memory(PCB &program){
    for(int i = 5; i >= 0; i--) {
        if(program.PID == memory_paritions[i].occupied) {
            memory_paritions[i].occupied = -1;
            program.partition_number = -1;
            return true;
        }
    }
    return false;
}

//Convert a list of strings into a PCB
// PROCESS CREATION (EXTENDED)
inline PCB add_process(std::vector<std::string> tokens) {
    PCB process;

    process.PID              = std::stoi(tokens[0]);
    process.size             = std::stoi(tokens[1]);
    process.arrival_time     = std::stoi(tokens[2]);
    process.processing_time = std::stoi(tokens[3]);
    process.remaining_time  = std::stoi(tokens[3]);
    process.io_freq          = std::stoi(tokens[4]);
    process.io_duration      = std::stoi(tokens[5]);

    // Optional 7th field for priority (EP / EP+RR)
    process.priority         = (tokens.size() > 6) ? std::stoi(tokens[6]) : 0;

    process.start_time        = -1;
    process.partition_number = -1;
    process.state             = NOT_ASSIGNED;
    process.remaining_io_time = 0;

    // Metrics init
    process.completion_time   = 0;
    process.total_wait_time   = 0;
    process.last_ready_time   = 0;

    return process;
}

// QUEUE / CPU HELPERS
inline void sync_queue(std::vector<PCB> &process_queue, PCB _process) {
    for(auto &process : process_queue)
        if(process.PID == _process.PID) process = _process;
}

//Returns true if all processes in the queue have terminated
inline bool all_process_terminated(std::vector<PCB> processes) {
    for(auto process : processes)
        if(process.state != TERMINATED) return false;
    return true;
}

//Terminates a given process
inline void terminate_process(PCB &running, std::vector<PCB> &job_queue) {
    running.remaining_time = 0;
    running.state = TERMINATED;
    free_memory(running);
    sync_queue(job_queue, running);
}

inline void idle_CPU(PCB &running) {
    running.start_time = 0;
    running.processing_time = 0;
    running.remaining_time = 0;
    running.arrival_time = 0;
    running.io_duration = 0;
    running.io_freq = 0;
    running.partition_number = 0;
    running.size = 0;
    running.state = NOT_ASSIGNED;
    running.PID = -1;
}

inline std::string memory_status(unsigned int current_time,
                                 const std::vector<PCB> &job_list) {
    unsigned int used = 0;

    for (const auto &p : job_list) {
        if (p.state != TERMINATED && p.partition_number != -1)
            used += p.size;
    }

    std::stringstream ss;
    ss << "Time " << current_time
       << " | Used Memory: " << used << " KB\n";

    return ss.str();
}

// INTERRUPT / CONTEXT SWITCH OVERHEAD
inline void simulate_interrupt_overhead(unsigned int &current_time) {
    const unsigned int ISR_OVERHEAD = 5;   // mock ISR time
    current_time += ISR_OVERHEAD;
}
#endif