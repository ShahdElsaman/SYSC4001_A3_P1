/**
 * @file interrupts.cpp
 * @author Nawal Musameh, Shahd Elsaman
 * External Priority + Round Robin (100 ms quantum)
 * 
 */

#include "interrupts_101360700_101268283.hpp"


// SORT READY QUEUE BY PRIORITY
void sort_by_priority(std::vector<PCB> &ready_queue) {
    std::sort(ready_queue.begin(), ready_queue.end(),
              [](const PCB &a, const PCB &b) {
                  return a.priority > b.priority;   // higher value = higher priority
              });
}

// EP + RR SIMULATION
std::tuple<std::string, std::string>
run_simulation(std::vector<PCB> list_processes) {

    std::vector<PCB> ready_queue;
    std::vector<PCB> wait_queue;
    std::vector<PCB> job_list;

    unsigned int current_time     = 0;
    const unsigned int QUANTUM    = 100;
    unsigned int quantum_counter  = 0;

    PCB running;
    idle_CPU(running);

    std::string execution_status = print_exec_header();
    std::string memory_log;

    // main simulation loop
    while (!all_process_terminated(job_list) ||
           job_list.empty() ||            // make sure we start when no jobs yet
           !ready_queue.empty() ||
           !wait_queue.empty() ||
           running.PID != -1) {

        // process arrival
        for (auto &process : list_processes) {
            if (process.arrival_time == current_time) {

                if (assign_memory(process)) {
                    process.state = READY;
                    process.last_ready_time = current_time;

                    ready_queue.push_back(process);
                    job_list.push_back(process);

                    execution_status += print_exec_status(
                        current_time, process.PID, NEW, READY);

                    // log memory status when a process is admitted
                    memory_log += memory_status(current_time, job_list);
                }
            }
        }

        // Wait queue 
        for (auto it = wait_queue.begin(); it != wait_queue.end(); ) {

            it->remaining_io_time--;

            if (it->remaining_io_time == 0) {

                states old_state = it->state;
                it->state = READY;
                it->last_ready_time = current_time;

                ready_queue.push_back(*it);
                sync_queue(job_list, *it);

                execution_status += print_exec_status(
                    current_time, it->PID, old_state, READY);

                it = wait_queue.erase(it);
            }
            else {
                ++it;
            }
        }

        if (running.PID != -1 && !ready_queue.empty()) {

            sort_by_priority(ready_queue);

            if (ready_queue.front().priority > running.priority) {

                running.state = READY;
                running.last_ready_time = current_time;

                ready_queue.push_back(running);
                sync_queue(job_list, running);

                execution_status += print_exec_status(
                    current_time, running.PID, RUNNING, READY);

                idle_CPU(running);
                quantum_counter = 0;

                simulate_interrupt_overhead(current_time);
            }
        }

        // Dispach
        if (running.PID == -1 && !ready_queue.empty()) {

            sort_by_priority(ready_queue);

            PCB next = ready_queue.front();
            ready_queue.erase(ready_queue.begin());

            next.total_wait_time +=
                (current_time - next.last_ready_time);

            next.state = RUNNING;

            if (next.start_time == -1)
                next.start_time = current_time;

            running = next;
            sync_queue(job_list, running);

            quantum_counter = 0;

            execution_status += print_exec_status(
                current_time, running.PID, READY, RUNNING);

            simulate_interrupt_overhead(current_time);
        }

        // CPU execution
        if (running.PID != -1) {

            running.remaining_time--;
            quantum_counter++;

            int cpu_used =
                running.processing_time - running.remaining_time;

            bool did_transition = false;

            // I/O interrupt 
            if (running.io_freq > 0 &&
                cpu_used > 0 &&
                cpu_used % running.io_freq == 0 &&
                running.remaining_time > 0) {

                states old_state = running.state;

                running.state = WAITING;
                running.remaining_io_time = running.io_duration;

                wait_queue.push_back(running);
                sync_queue(job_list, running);

                execution_status += print_exec_status(
                    current_time, running.PID, old_state, WAITING);

                idle_CPU(running);
                quantum_counter = 0;

                simulate_interrupt_overhead(current_time);
                did_transition = true;
            }

            // process termination  
            if (!did_transition &&
                running.PID != -1 &&
                running.remaining_time == 0) {

                running.state = TERMINATED;
                running.completion_time = current_time + 1;

                execution_status += print_exec_status(
                    current_time, running.PID, RUNNING, TERMINATED);

                terminate_process(running, job_list);

                // log memory status when a process frees its partition
                memory_log += memory_status(current_time, job_list);

                idle_CPU(running);
                quantum_counter = 0;

                simulate_interrupt_overhead(current_time);
                did_transition = true;
            }

            // RR 
            if (!did_transition &&
                running.PID != -1 &&
                quantum_counter == QUANTUM) {

                states old_state = running.state;

                running.state = READY;
                running.last_ready_time = current_time;

                ready_queue.push_back(running);
                sync_queue(job_list, running);

                execution_status += print_exec_status(
                    current_time, running.PID, old_state, READY);

                idle_CPU(running);
                quantum_counter = 0;

                simulate_interrupt_overhead(current_time);
            }
        }

        current_time++;
    }

    execution_status += print_exec_footer();

    //metrics calculation
    unsigned int n = job_list.size();
    double total_wait = 0, total_turnaround = 0, total_response = 0;
    unsigned int finish_time = 0;

    for (const auto &p : job_list) {

        unsigned int turnaround =
            p.completion_time - p.arrival_time;

        unsigned int response =
            (p.start_time >= 0)
                ? (p.start_time - p.arrival_time)
                : 0;

        total_wait       += p.total_wait_time;
        total_turnaround += turnaround;
        total_response   += response;

        if (p.completion_time > finish_time)
            finish_time = p.completion_time;
    }

    double avg_wait =
        (n > 0) ? total_wait / n : 0;

    double avg_turnaround =
        (n > 0) ? total_turnaround / n : 0;

    double avg_response =
        (n > 0) ? total_response / n : 0;

    double throughput =
        (finish_time > 0)
            ? static_cast<double>(n) / finish_time
            : 0;

    std::stringstream metrics;
    metrics << "\n=== EP + RR Metrics ===\n";
    metrics << "Throughput: " << throughput << " processes/ms\n";
    metrics << "Average Waiting Time: " << avg_wait << " ms\n";
    metrics << "Average Turnaround Time: " << avg_turnaround << " ms\n";
    metrics << "Average Response Time: " << avg_response << " ms\n";

    execution_status += metrics.str();

    return std::make_tuple(execution_status, memory_log);
}


// MAIN
int main(int argc, char** argv) {

    if (argc != 2) {
        std::cout << "ERROR!\nUsage: ./interrupts_EP_RR input.txt\n";
        return -1;
    }

    std::ifstream input_file(argv[1]);
    if (!input_file.is_open()) {
        std::cerr << "Error opening file.\n";
        return -1;
    }

    std::vector<PCB> list_process;
    std::string line;

    while (std::getline(input_file, line)) {
        auto tokens = split_delim(line, ", ");
        auto p = add_process(tokens);
        list_process.push_back(p);
    }

    input_file.close();

    auto [exec, memlog] = run_simulation(list_process);

    write_output(exec,   "execution_EP_RR.txt");
    write_output(memlog, "memory_EP_RR.txt");

    return 0;