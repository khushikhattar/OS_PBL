#include <mpi.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <unistd.h>
#include <climits>

const int num_jobs = 8;
const int heartbeat_tag = 99;

std::string log_file = "../server/logs/mpi.log"; // Windows-accessible path

void log(const std::string &message)
{
    std::ofstream logfile(log_file, std::ios::app);
    logfile << message << std::endl;
    logfile.close();
}

void assign_jobs_round_robin(const std::vector<int> &processors)
{
    log("--- Round Robin Job Assignment ---");
    for (int i = 0; i < num_jobs; i++)
    {
        int assigned = processors[i % processors.size()];
        log("Job " + std::to_string(i + 1) + " assigned to processor " + std::to_string(assigned));
    }
}

void assign_jobs_least_connection(const std::vector<int> &processors, std::vector<double> &loads)
{
    log("--- Least Connection Job Assignment ---");
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
        log("Job " + std::to_string(i + 1) + " assigned to processor " + std::to_string(chosen) + " (Least Load: " + std::to_string(min_load) + ")");
        loads[chosen - 1] += 1.0; // Fix index
    }
}

void master_process(int num_processors)
{
    MPI_Status status;
    std::vector<int> processors(num_processors);
    for (int i = 0; i < num_processors; i++)
        processors[i] = i + 1;

    std::vector<double> loads(num_processors, 0.0);
    double start = MPI_Wtime(), timeout = 5.0;

    log("Master: Listening for heartbeats for 5 seconds...");
    while (MPI_Wtime() - start < timeout)
    {
        for (int i = 1; i <= num_processors; i++)
        {
            int flag = 0, heartbeat;
            MPI_Iprobe(i, heartbeat_tag, MPI_COMM_WORLD, &flag, &status);
            if (flag)
            {
                MPI_Recv(&heartbeat, 1, MPI_INT, i, heartbeat_tag, MPI_COMM_WORLD, &status);
                log("Master: Received heartbeat from processor " + std::to_string(i));
            }
        }
        sleep(1);
    }

    assign_jobs_round_robin(processors);
    assign_jobs_least_connection(processors, loads);

    double total = 0;
    for (double l : loads)
        total += l;
    log("Total Load: " + std::to_string(total));
    log("== Job assignment complete ==\n");
}

void slave_process(int rank)
{
    int heartbeat = 1;
    MPI_Send(&heartbeat, 1, MPI_INT, 0, heartbeat_tag, MPI_COMM_WORLD);
    log("Processor " + std::to_string(rank) + ": Sent heartbeat.");
}

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Clear the log file
    if (rank == 0)
    {
        std::ofstream logfile(log_file);
        logfile << "=== MPI Log Started ===\n";
        logfile.close();
        master_process(size - 1);
    }
    else
    {
        slave_process(rank);
    }

    MPI_Finalize();
    return 0;
}
