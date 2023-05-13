//Sukhdeep Singh
//sukhdeep.singh144@myhunter.cuny.edu

#ifndef SIMOS_H
#define SIMOS_H

#include <vector>
#include <queue>
#include <unordered_map>
#include <string>

struct FileReadRequest
{
    int PID{0};
    std::string fileName{""};
};

struct MemoryItem
{
    unsigned long long itemAddress;
    unsigned long long itemSize;
    int PID; // PID of the process using this chunk of memory
};

using MemoryUsage = std::vector<MemoryItem>;

class SimOS
{
public:
    SimOS(int numberOfDisks, unsigned long long amountOfRAM);
    bool NewProcess(int priority, unsigned long long size);
    bool SimFork();
    void SimExit();
    void SimWait();
    void DiskReadRequest(int diskNumber, std::string fileName);
    void DiskJobCompleted(int diskNumber);
    int GetCPU();
    std::vector<int> GetReadyQueue();
    MemoryUsage GetMemory();
    FileReadRequest GetDisk(int diskNumber);
    std::queue<FileReadRequest> GetDiskQueue(int diskNumber);
private:
    struct Process
    {
        int PID{0};
        int priority{0};
        unsigned long long size{0};
        unsigned long long memoryAddress{0};
        bool isZombie{false};
        int parentPID{0};
        std::vector<int> childrenPIDs;
        bool isWaiting{false};
    };

    struct DiskRequest
    {
        int PID{0};
        std::string fileName{""};
    };

    unsigned long long amountOfRAM_;
    MemoryUsage memoryUsage_;
    std::unordered_map<int, Process> processes_;
    int nextPID_{1};
    int currentPID_{0};
    std::queue<int> readyQueue_;
    std::queue<int> diskQueue_[10];
    std::vector<DiskRequest> diskRequests_[10];

    bool allocateMemory(unsigned long long size, int PID);
    void deallocateMemory(int PID);
    void killProcess(int PID);
    void killDescendants(int PID);
    int findProcess(int PID);
    void schedule();
};

#endif
