#include <mpi.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <climits>
#include <unistd.h>
#include <chrono>
#include <ctime>
#include <sstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
void log_message(std::ofstream &logfile, const std::string &msg)
{
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::string time_str = std::string(std::ctime(&now_c));
    time_str.erase(time_str.length() - 1); // remove newline
    logfile << "[" << time_str << "] " << msg << std::endl;
    logfile.flush();
}

const int num_jobs = 8;
const int heartbeat_tag = 99;

void assign_jobs_round_robin(const std::vector<int> &processors, std::ofstream &logfile, std::vector<std::string> &assignments)
{
    logfile << "--- Round Robin Job Assignment ---" << std::endl;
    for (int i = 0; i < num_jobs; i++)
    {
        int assigned_processor = processors[i % processors.size()];
        logfile << "Job " << i + 1 << " assigned to processor " << assigned_processor << " using round-robin" << std::endl;
        assignments.push_back("Job " + std::to_string(i + 1) + " -> P" + std::to_string(assigned_processor));
    }
    logfile.flush();
}

void assign_jobs_least_connection(const std::vector<int> &processors, std::vector<double> &loads, std::ofstream &logfile, std::vector<std::string> &assignments)
{
    logfile << "--- Least Connection Job Assignment ---" << std::endl;
    for (int i = 0; i < num_jobs; i++)
    {
        int chosen_index = -1;
        double min_load = INT_MAX;
        for (size_t j = 0; j < processors.size(); j++)
        {
            if (loads[j] < min_load)
            {
                min_load = loads[j];
                chosen_index = j;
            }
        }
        int chosen_processor = processors[chosen_index];
        logfile << "Job " << i + 1 << " assigned to processor " << chosen_processor
                << " using least-connections (Least Load: " << min_load << ")" << std::endl;

        loads[chosen_index] += 1.0;
        assignments.push_back("Job " + std::to_string(i + 1) + " -> P" + std::to_string(chosen_processor));
    }
    logfile.flush();
}
void write_json_status(const std::vector<int> &active_processors, const std::vector<std::string> &assignments, const std::string &strategy)
{
    json j;
    j["active_processors"] = active_processors;
    j["job_assignments"] = assignments;
    j["strategy"] = strategy;

    std::ofstream file("../server/system-state.json", std::ios::trunc);
    if (!file.is_open())
    {
        std::cerr << "Failed to open system-state.json for writing\n";
        return;
    }
    file << j.dump(4);
    file.close();
}

void master_process(int num_processors)
{
    std::ofstream logfile("../server/logs/mpi.log", std::ios::app);
    if (!logfile.is_open())
    {
        std::cerr << "Failed to open log file." << std::endl;
        return;
    }

    log_message(logfile, "Master started");

    MPI_Status status;
    std::vector<int> processors;
    std::vector<int> alive(num_processors, 0);

    double start = MPI_Wtime();
    double timeout = 5.0;

    log_message(logfile, "Master: Listening for heartbeats for " + std::to_string((int)timeout) + " seconds...");

    while (MPI_Wtime() - start < timeout)
    {
        for (int i = 1; i <= num_processors; i++)
        {
            int flag = 0, heartbeat;
            MPI_Iprobe(i, heartbeat_tag, MPI_COMM_WORLD, &flag, &status);
            if (flag)
            {
                MPI_Recv(&heartbeat, 1, MPI_INT, i, heartbeat_tag, MPI_COMM_WORLD, &status);
                log_message(logfile, "Master: Received heartbeat from processor " + std::to_string(i));
                alive[i - 1] = 1;
            }
        }
        sleep(1);
    }

    for (int i = 0; i < num_processors; i++)
    {
        if (alive[i])
            processors.push_back(i + 1);
    }

    if (processors.empty())
    {
        log_message(logfile, "No processors alive. Exiting job assignment.");
        logfile.close();
        return;
    }

    std::vector<std::string> rr_assignments;
    assign_jobs_round_robin(processors, logfile, rr_assignments);
    double total_load_rr = (double)num_jobs;
    log_message(logfile, "Total Load (Round Robin): " + std::to_string(total_load_rr));

    std::vector<double> loads(processors.size(), 0.0);
    std::vector<std::string> lc_assignments;
    assign_jobs_least_connection(processors, loads, logfile, lc_assignments);

    double total_load_lc = 0.0;
    for (double l : loads)
        total_load_lc += l;

    log_message(logfile, "Total Load (Least Connection): " + std::to_string(total_load_lc));

    // Write JSON for both strategies
    json j;
    j["active_processors"] = processors;
    j["round_robin"] = {
        {"job_assignments", rr_assignments},
        {"strategy", "RoundRobin"},
        {"total_load", total_load_rr}};
    j["least_connection"] = {
        {"job_assignments", lc_assignments},
        {"strategy", "LeastConnection"},
        {"total_load", total_load_lc}};

    std::ofstream file("../server/system-state.json", std::ios::trunc);
    if (!file.is_open())
    {
        std::cerr << "Failed to open system-state.json for writing\n";
    }
    else
    {
        file << j.dump(4);
        file.close();
    }

    log_message(logfile, "Master done.");
    logfile.close();
}

void slave_process(int rank)
{

    int heartbeat = 1;
    MPI_Send(&heartbeat, 1, MPI_INT, 0, heartbeat_tag, MPI_COMM_WORLD);
}

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (rank == 0)
    {
        master_process(size - 1);
    }
    else
    {
        slave_process(rank);
    }

    MPI_Finalize();
    return 0;
}
