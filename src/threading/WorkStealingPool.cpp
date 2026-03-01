#include "threading/WorkStealingPool.hpp"
#include <iostream>
#include <random>
#include <algorithm>

namespace noita {

// 线程本地存储初始化
thread_local int WorkStealingPool::thread_id_ = -1;
thread_local WorkStealingPool::WorkStealingQueue* WorkStealingPool::local_queue_ = nullptr;

WorkStealingPool::WorkStealingPool(int thread_count) {
    // 确定线程数
    if (thread_count <= 0) {
        thread_count = std::thread::hardware_concurrency();
        if (thread_count == 0) thread_count = 8;  // 默认4线程
    }
    
    thread_count_ = thread_count;
    
    // 创建工作窃取队列
    local_queues_.reserve(thread_count);
    for (int i = 0; i < thread_count; ++i) {
        local_queues_.push_back(std::make_unique<WorkStealingQueue>());
    }
    
    // 创建工作线程
    workers_.reserve(thread_count);
    for (int i = 0; i < thread_count; ++i) {
        workers_.emplace_back(&WorkStealingPool::worker_thread, this, i);
    }
    
    std::cout << "WorkStealingPool initialized with " << thread_count << " threads" << std::endl;
}

WorkStealingPool::~WorkStealingPool() {
    stop_ = true;
    queue_cv_.notify_all();
    
    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    
    std::cout << "WorkStealingPool destroyed" << std::endl;
}

void WorkStealingPool::worker_thread(int id) {
    // 设置线程本地存储
    thread_id_ = id;
    local_queue_ = local_queues_[id].get();
    
    while (!stop_) {
        Task task = get_task();
        
        if (task) {
            try {
                task();
                active_tasks_--;
                completion_cv_.notify_all();
            } catch (const std::exception& e) {
                std::cerr << "Exception in worker thread " << id << ": " << e.what() << std::endl;
                active_tasks_--;
                completion_cv_.notify_all();
            }
        } else {
            // 没有任务,等待
            std::unique_lock<std::mutex> lock(queue_mutex_);
            queue_cv_.wait_for(lock, std::chrono::milliseconds(1), [this]() {
                return stop_ || active_tasks_ > 0;
            });
        }
    }
}

WorkStealingPool::Task WorkStealingPool::get_task() {
    // 1. 尝试从本地队列获取
    if (local_queue_) {
        Task task = local_queue_->pop();
        if (task) return task;
    }
    
    // 2. 尝试从其他队列窃取
    if (thread_id_ >= 0) {
        Task task = try_steal(thread_id_);
        if (task) return task;
    }
    
    return nullptr;
}

WorkStealingPool::Task WorkStealingPool::try_steal(int id) {
    // 随机选择一个队列尝试窃取
    static thread_local std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<> dis(0, local_queues_.size() - 1);
    
    // 尝试窃取多次
    for (int attempt = 0; attempt < 3; ++attempt) {
        int target = dis(gen);
        if (target == id) continue;
        
        Task task = local_queues_[target]->steal();
        if (task) return task;
    }
    
    return nullptr;
}

} // namespace noita
