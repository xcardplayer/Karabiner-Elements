#pragma once

#include <chrono>
#include <functional>
#include <mutex>
#include <thread>

namespace krbn {
class thread_utility final {
public:
  static std::thread::id get_main_thread_id(void) {
    static std::mutex mutex;
    std::lock_guard<std::mutex> guard(mutex);

    static std::thread::id id = std::this_thread::get_id();
    return id;
  }

  static void register_main_thread(void) {
    get_main_thread_id();
  }

  static bool is_main_thread(void) {
    return get_main_thread_id() == std::this_thread::get_id();
  }

  class timer final {
  public:
    timer(std::chrono::milliseconds interval,
          bool repeats,
          const std::function<void(void)>& function) : cancel_flag_(false),
                                                       repeats_(repeats) {
      thread_ = std::thread([this, interval, function] {
        do {
          // Wait
          {
            std::unique_lock<std::mutex> lock(timer_mutex_);
            timer_cv_.wait_for(lock, interval, [this] {
              return cancel_flag_ == true;
            });
          }

          if (cancel_flag_) {
            return;
          }

          function();
        } while (repeats_);
      });
    }

    ~timer(void) {
      if (thread_.joinable()) {
        cancel();

        thread_.join();
      }
    }

    void wait(void) {
      if (thread_.joinable()) {
        thread_.join();
      }
    }

    void cancel(void) {
      cancel_flag_ = true;
      repeats_ = false;

      timer_cv_.notify_one();
    }

    void unset_repeats(void) {
      repeats_ = false;
    }

  private:
    std::thread thread_;
    std::atomic<bool> cancel_flag_;
    std::atomic<bool> repeats_;

    std::mutex timer_mutex_;
    std::condition_variable timer_cv_;
  };
};
} // namespace krbn
