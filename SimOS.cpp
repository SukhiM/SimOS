//Sukhdeep Singh
//sukhdeep.singh144@myhunter.cuny.edu

#include "SimOS.h"
#include <algorithm>

SimOS::SimOS(int numberOfDisks, unsigned long long amountOfRAM)
    : amountOfRAM_{amountOfRAM}
{
    for (int i = 0; i < numberOfDisks; ++i) {
        diskRequests_[i].reserve(1000);
    }
    processes_[0] = {0, 0, 0, 0, false, 0, {}, false};
}

bool SimOS::NewProcess(int priority, unsigned long long size)
{
    if (allocateMemory(size, nextPID_)) {
        processes_[nextPID_] = {nextPID_, priority, size, memoryUsage_.back().itemAddress, false, currentPID_, {}};
        if (currentPID_ != 0) {
            processes_[currentPID_].childrenPIDs.push_back(nextPID_);
        }
        readyQueue_.push(nextPID_);
        ++nextPID_;
        schedule();
        return true;
    }
    return false;
}

bool SimOS::SimFork()
{
    if (allocateMemory(processes_[currentPID_].size, nextPID_)) {
        processes_[nextPID_] = {nextPID_, processes_[currentPID_].priority, processes_[currentPID_].size, memoryUsage_.back().itemAddress, false, currentPID_, {}};
        processes_[currentPID_].childrenPIDs.push_back(nextPID_);
        readyQueue_.push(nextPID_);
        ++nextPID_;
        schedule();
        return true;
    }
    return false;
}

void SimOS::SimExit()
{
    int parentPID = processes_[currentPID_].parentPID;
    killProcess(currentPID_);
    if (processes_[parentPID].isWaiting) {
        processes_[parentPID].isWaiting = false;
        readyQueue_.push(parentPID);
    }
    schedule();
}


void SimOS::SimWait()
{
    if (!processes_[currentPID_].childrenPIDs.empty()) {
        int zombiePID{0};
        for (int PID : processes_[currentPID_].childrenPIDs) {
            if (processes_[PID].isZombie) {
                zombiePID = PID;
                break;
            }
        }
        if (zombiePID != 0) {
            killProcess(zombiePID);
            return;
        }
        processes_[currentPID_].isWaiting = true;
        schedule();
    }
}


/*void SimOS::DiskReadRequest(int diskNumber, std::string fileName)
{
    diskRequests_[diskNumber].push_back({currentPID_, fileName});
    diskQueue_[diskNumber].push(currentPID_);
    schedule();
}
*/

bool SimOS::allocateMemory(unsigned long long size, int PID)
{
    // Find a chunk of memory large enough to fit the process
    auto it = std::find_if(memoryUsage_.begin(), memoryUsage_.end(),
                           [size](const MemoryItem& item){ return item.itemSize >= size && item.PID == 0; });
    if (it == memoryUsage_.end()) {
        return false; // No suitable chunk of memory found
    }
    // Allocate memory for the process
    unsigned long long address = it->itemAddress;
    *it = {address, size, PID};
    // Sort memory by address
    std::sort(memoryUsage_.begin(), memoryUsage_.end(),
              [](const MemoryItem& item1, const MemoryItem& item2){ return item1.itemAddress < item2.itemAddress; });
    return true;
}

void SimOS::deallocateMemory(int PID)
{
    // Find the process in the memory usage vector
    auto it = std::find_if(memoryUsage_.begin(), memoryUsage_.end(),
                           [PID](const MemoryItem& item){ return item.PID == PID; });
    if (it != memoryUsage_.end()) {
        // Mark the memory as free
        it->PID = 0;
        // Merge adjacent free memory chunks
        auto prevIt = it - 1;
        auto nextIt = it + 1;
        if (prevIt != memoryUsage_.end() && prevIt->PID == 0) {
            prevIt->itemSize += it->itemSize;
            memoryUsage_.erase(it);
            it = prevIt;
        }
        if (nextIt != memoryUsage_.end() && nextIt->PID == 0) {
            it->itemSize += nextIt->itemSize;
            memoryUsage_.erase(nextIt);
        }
    }
}

void SimOS::killProcess(int PID)
{
    processes_[PID].isZombie = true;
    if (processes_[PID].isWaiting) {
        // Remove the process from the ready queue if it's waiting
        std::queue<int> tempQueue;
        while (!readyQueue_.empty()) {
            int readyPID = readyQueue_.front();
            readyQueue_.pop();
            if (readyPID != PID) {
                tempQueue.push(readyPID);
            }
        }
        std::swap(readyQueue_, tempQueue);
    }
    // Kill all the descendants of the process
    killDescendants(PID);
    // Deallocate memory used by the process
    deallocateMemory(PID);
    // Remove the process from the process table
    processes_.erase(PID);
}

void SimOS::killDescendants(int PID)
{
    for (int childPID : processes_[PID].childrenPIDs) {
        killDescendants(childPID);
        killProcess(childPID);
    }
}

int SimOS::findProcess(int PID)
{
    auto it = processes_.find(PID);
    return it != processes_.end() ? it->second.memoryAddress : -1;
}

void SimOS::schedule()
{
    int highestPriority{11};
    int nextPID{0};
    while (!readyQueue_.empty()) {
        int PID = readyQueue_.front();
        if (processes_[PID].priority < highestPriority) {
            highestPriority = processes_[PID].priority;
            nextPID = PID;
        }
        readyQueue_.pop();
    }
    currentPID_ = nextPID;
}



void SimOS::DiskReadRequest(int diskNumber, std::string fileName)
{
    diskRequests_[diskNumber].push_back({currentPID_, fileName});
    diskQueue_[diskNumber].push(currentPID_);
    processes_[currentPID_].isZombie = true;
    readyQueue_.pop();
    schedule();
}

void SimOS::DiskJobCompleted(int diskNumber) {

    FileReadRequest fileRequest = GetDisk(diskNumber);

    if (processes_.count(fileRequest.PID) != 0 && !processes_[fileRequest.PID].isZombie) {
        diskQueue_[diskNumber].push(fileRequest.PID);
    }
    else {
        auto it = std::find_if(diskRequests_[diskNumber].begin(), diskRequests_[diskNumber].end(),
            [&](DiskRequest dr) { return dr.PID == fileRequest.PID && dr.fileName == fileRequest.fileName; });

        if (it != diskRequests_[diskNumber].end()) {
            diskRequests_[diskNumber].erase(it);
        }
    }
    if (GetCPU() == 0) {
        schedule();
    }
}


int SimOS::GetCPU() {
  return currentPID_;
}

std::vector<int> SimOS::GetReadyQueue() {
  std::vector<int> readyQueue;
  while (!readyQueue_.empty()){
    int PID = readyQueue_.front();
    readyQueue_.pop();
    readyQueue.push_back(PID);
  }
  for(int PID : readyQueue){
    readyQueue_.push(PID);
  }
  return readyQueue;
}

MemoryUsage SimOS::GetMemory() {
  return memoryUsage_;
}

FileReadRequest SimOS::GetDisk(int diskNumber) {
  FileReadRequest fileReadRequest;
  if (!diskQueue_[diskNumber].empty()){
   auto fileReadRequest = diskQueue_[diskNumber].front();
  }
  return fileReadRequest;
}

std::queue<FileReadRequest> SimOS::GetDiskQueue(int diskNumber) {
    std::queue<FileReadRequest> diskQueue;
    for (const auto& request : diskRequests_[diskNumber]) {
        FileReadRequest fileRequest;
        fileRequest.PID = request.PID;
        fileRequest.fileName = request.fileName;
        diskQueue.push(fileRequest);
    }

    return diskQueue;
}