#pragma once

#include <vector>
#include <thread>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <random>
#include <atomic>
#include <functional>
#include <future>
#include <memory>

namespace noita {

/**
 * @brief 工作窃取线程池
 * 
 * 基于Noita的性能优化需求,实现高效的任务窃取调度
 */
class WorkStealingPool {
public:
    using Task = std::function<void()>;
    
private:
    // 工作窃取队列
    class WorkStealingQueue {
    private:
        std::deque<Task> queue_;
        mutable std::mutex mutex_;
        
    public:
        void push(Task&& task) {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push_back(std::move(task));
        }
        
        Task pop() {
            std::lock_guard<std::mutex> lock(mutex_);
            if (queue_.empty()) return nullptr;
            auto task = std::move(queue_.front());
            queue_.pop_front();
            return task;
        }
        
        Task steal() {
            std::lock_guard<std::mutex> lock(mutex_);
            if (queue_.empty()) return nullptr;
            auto task = std::move(queue_.back());
            queue_.pop_back();
            return task;
        }
        
        bool empty() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return queue_.empty();
        }
        
        size_t size() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return queue_.size();
        }
    };
    
    std::vector<std::unique_ptr<WorkStealingQueue>> local_queues_;
    std::vector<std::thread> workers_;
    
    std::atomic<bool> stop_{false};
    std::atomic<int> active_tasks_{0};
    
    std::mutex completion_mutex_;
    std::condition_variable completion_cv_;
    
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    
    // 线程本地存储
    static thread_local int thread_id_;
    static thread_local WorkStealingQueue* local_queue_;
    
    int thread_count_;
    
public:
    explicit WorkStealingPool(int thread_count = -1);
    ~WorkStealingPool();
    
    // 提交任务
    template<typename F>
    std::future<void> enqueue(F&& task) {
        auto promise = std::make_shared<std::promise<void>>();
        auto future = promise->get_future();
        
        auto wrapped_task = [this, promise, task = std::forward<F>(task)]() mutable {
            try {
                task();
                promise->set_value();
            } catch (...) {
                promise->set_exception(std::current_exception());
            }
            active_tasks_--;
            completion_cv_.notify_all();
        };
        
        int id = thread_id_;
        if (id >= 0 && id < static_cast<int>(local_queues_.size())) {
            local_queues_[id]->push(std::move(wrapped_task));
        } else {
            // 随机分配
            static thread_local std::mt19937 gen(std::random_device{}());
            std::uniform_int_distribution<> dis(0, local_queues_.size() - 1);
            int target = dis(gen);
            local_queues_[target]->push(std::move(wrapped_task));
        }
        
        active_tasks_++;
        queue_cv_.notify_one();
        
        return future;
    }
    
    // 批量提交任务
    template<typename F>
    void enqueue_batch(std::vector<F>& tasks) {
        for (auto& task : tasks) {
            enqueue(std::move(task));
        }
    }
    
    // 等待所有任务完成
    void wait_all() {
        std::unique_lock<std::mutex> lock(completion_mutex_);
        completion_cv_.wait(lock, [this]() {
            return active_tasks_ == 0;
        });
    }
    
    // 并行for循环
    template<typename F>
    void parallel_for(int start, int end, F&& func) {
        if (start >= end) return;
        
        int range = end - start;
        int chunk_size = std::max(1, range / thread_count_);
        
        std::vector<std::future<void>> futures;
        futures.reserve(thread_count_);
        
        for (int i = 0; i < thread_count_; ++i) {
            int chunk_start = start + i * chunk_size;
            int chunk_end = (i == thread_count_ - 1) ? end : chunk_start + chunk_size;
            
            if (chunk_start >= end) break;
            
            futures.push_back(enqueue([chunk_start, chunk_end, &func]() {
                for (int i = chunk_start; i < chunk_end; ++i) {
                    func(i);
                }
            }));
        }
        
        // 等待所有任务完成
        for (auto& future : futures) {
            future.wait();
        }
    }
    
    // 获取线程数
    int get_thread_count() const { return thread_count_; }
    
    // 获取活跃任务数
    int get_active_tasks() const { return active_tasks_; }
    
    // 检查是否空闲
    bool is_idle() const { return active_tasks_ == 0; }
    
private:
    void worker_thread(int id);
    Task get_task();
    Task try_steal(int id);
};

} // namespace noita
