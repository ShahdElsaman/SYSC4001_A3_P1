/**
 * @file interrupts.cpp
 * @author Shahd Elsaman, Nawal Musameh
 * @brief template main.cpp file for Assignment 3 Part 1 of SYSC4001
 * 
 */

#include "interrupts_101360700_101268283.hpp"

void EP_scheduler(std::vector<PCB> &ready_queue) {
    std::sort(
        ready_queue.begin(),
        ready_queue.end(),
        [](const PCB &first, const PCB &second){
            return (first.priority < second.priority);
        }
    );
}

std::tuple<std::string /* add std::string for bonus mark */ > run_simulation(std::vector<PCB> list_processes) {

    std::vector<PCB> ready_queue;   //The ready queue of processes
    std::vector<PCB> wait_queue;    //The wait queue of processes
    std::vector<PCB> job_list;      //List of all processes
    std::vector<PCB> input_processes = list_processes;

    unsigned int current_time = 0;
    PCB running;
    idle_CPU(running);              //Initialize running = idle

    std::string execution_status;
    execution_status = print_exec_header();    //Header row

    //Main simulation loop
    while(!input_processes.empty() ||
          !all_process_terminated(job_list) ||
          !ready_queue.empty() ||
          !wait_queue.empty() ||
          running.PID != -1)
    {
        ////////////////////////////////////////////////////////////////////
        for(auto it = input_processes.begin(); it != input_processes.end();) {

            if(it->arrival_time == current_time) {
                if(assign_memory(*it)) {
                    it->state = READY;
                    it->remaining_time = it->processing_time;

                    ready_queue.push_back(*it);
                    job_list.push_back(*it);

                    execution_status += print_exec_status(
                        current_time, it->PID, NEW, READY
                    );

                    it = input_processes.erase(it);
                }
                else {
                    ++it;
                }
            }
            else {
                ++it;
            }
        }
        ////////////////////////////////////////////////////////////////////
        for(auto it = wait_queue.begin(); it != wait_queue.end();) {

            if(it->remaining_io_time <= current_time){
                states old_state = it->state;
                it->state = READY;

                ready_queue.push_back(*it);
                sync_queue(job_list, *it);

                execution_status += print_exec_status(
                    current_time, it->PID, old_state, READY
                );

                it = wait_queue.erase(it);
            }
            else {
                ++it;
            }
        }

        ////////////////////////////////////////////////////////////////////
        if(running.PID == -1 && !ready_queue.empty())
        {
            EP_scheduler(ready_queue);

            running = ready_queue.front();
            ready_queue.erase(ready_queue.begin());

            states old_state = running.state;
            running.state = RUNNING;

            execution_status += print_exec_status(current_time, running.PID, old_state, RUNNING);

            sync_queue(job_list, running);
        }

        ////////////////////////////////////////////////////////////////////
        if(running.PID != -1)
        {
            running.remaining_time--; 

            unsigned int time_executed =
                running.processing_time - running.remaining_time;

            if(running.remaining_time > 0 &&
               running.io_freq > 0 &&
               (time_executed % running.io_freq == 0))
            {
                states old_state = running.state;
                running.state = WAITING;
                running.remaining_io_time =
                    current_time + 1 + running.io_duration;

                wait_queue.push_back(running);
                sync_queue(job_list, running);

                execution_status += print_exec_status(current_time + 1, running.PID, old_state, WAITING);

                idle_CPU(running);
            }

            //TERMINATION
            else if(running.remaining_time == 0)
            {
                states old_state = running.state;
                running.state = TERMINATED;

                execution_status += print_exec_status(current_time + 1, running.PID, old_state, TERMINATED);

                terminate_process(running, job_list);
                idle_CPU(running);
            }
            else {
                sync_queue(job_list, running);
            }
        }

        current_time++; 
    }

    execution_status += print_exec_footer();
    return std::make_tuple(execution_status);
}


int main(int argc, char** argv) {

    if(argc != 2) {
        std::cout << "ERROR!\nExpected 1 argument.\n";
        return -1;
    }

    std::ifstream input_file(argv[1]);
    if (!input_file.is_open()) {
        std::cerr << "Error opening file\n";
        return -1;
    }

    std::string line;
    std::vector<PCB> list_process;

    while(std::getline(input_file, line)) {
        auto tokens = split_delim(line, ", ");
        auto new_process = add_process(tokens);
        list_process.push_back(new_process);
    }
    input_file.close();

    auto [exec] = run_simulation(list_processes);
    write_output(exec, "execution.txt");

    return 0;
}