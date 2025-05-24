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
    logfile << std::ctime(&now_c) << ": " << msg << std::endl;
}

const int num_jobs = 8;
const int heartbeat_tag = 99;
const int update_tag = 88;

void assign_jobs_round_robin(const std::vector<int> &processors, std::ofstream &logfile, std::vector<std::string> &assignments)
{
    logfile << "--- Round Robin Job Assignment ---\n";
    for (int i = 0; i < num_jobs; i++)
    {
        int assigned_processor = processors[i % processors.size()];
        logfile << "Job " << i + 1 << " assigned to processor " << assigned_processor << std::endl;
        assignments.push_back("Job " + std::to_string(i + 1) + " -> P" + std::to_string(assigned_processor));
    }
}

void assign_jobs_least_connection(const std::vector<int> &processors, std::vector<double> &loads, std::ofstream &logfile, std::vector<std::string> &assignments)
{
    logfile << "--- Least Connection Job Assignment ---\n";
    for (int i = 0; i < num_jobs; i++)
    {
        int chosen = -1;
        double min_load = INT_MAX;
        for (size_t j = 0; j < processors.size(); j++)
        {
            if (loads[j] < min_load)
            {
                min_load = loads[j];
                chosen = processors[j];
            }
        }
        logfile << "Job " << i + 1 << " assigned to processor " << chosen << " (Least Load: " << min_load << ")\n";
        loads[chosen - 1] += 1.0;
        assignments.push_back("Job " + std::to_string(i + 1) + " -> P" + std::to_string(chosen));
    }
}

void write_json_status(const std::vector<int> &active_processors, const std::vector<std::string> &assignments)
{
    json j;
    j["active_processors"] = active_processors;
    j["job_assignments"] = assignments;

    std::ofstream file("../server/system-state.json");
    file << j.dump(4);
    file.close();
}

void master_process(int num_processors)
{
    std::ofstream logfile("../server/logs/mpi.log", std::ios::app);
    if (!logfile.is_open())
        return;

    logfile << "Master started\n";

    MPI_Status status;
    std::vector<int> processors;
    std::vector<int> alive(num_processors, 0);

    double start = MPI_Wtime();
    double timeout = 5.0;

    logfile << "Master: Listening for heartbeats...\n";

    while (MPI_Wtime() - start < timeout)
    {
        for (int i = 1; i <= num_processors; i++)
        {
            int flag = 0, heartbeat;
            MPI_Iprobe(i, heartbeat_tag, MPI_COMM_WORLD, &flag, &status);
            if (flag)
            {
                MPI_Recv(&heartbeat, 1, MPI_INT, i, heartbeat_tag, MPI_COMM_WORLD, &status);
                logfile << "Heartbeat from processor " << i << "\n";
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

    std::vector<double> loads(num_processors, 0.0);
    std::vector<std::string> assignments;

    assign_jobs_round_robin(processors, logfile, assignments);
    assign_jobs_least_connection(processors, loads, logfile, assignments);

    write_json_status(processors, assignments);
    logfile << "Master done.\n";
    logfile.close();
}

void slave_process(int rank)
{
    std::ofstream logfile("../server/logs/mpi.log", std::ios::app);
    if (!logfile.is_open())
        return;

    int heartbeat = 1;
    MPI_Send(&heartbeat, 1, MPI_INT, 0, heartbeat_tag, MPI_COMM_WORLD);
    logfile << "Processor " << rank << ": Sent heartbeat.\n";

    logfile.close();
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
