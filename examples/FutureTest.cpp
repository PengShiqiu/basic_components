#include "util/future/Future.h"
#include "util/threadpool/ThreadPool.h"
#include "util/timer/Timer.h"
#define PRINT(t, arg...) printf("%s:%d " t, __FILE__, __LINE__, ##arg)

class FutureSchedule : public ananas::Scheduler {
 public:
  FutureSchedule(std::shared_ptr<ThreadPool> threadpool,
                 std::shared_ptr<ananas::TimerManager> timer)
      : threadpool_(threadpool), timer_(timer) {}
  virtual ~FutureSchedule() {}
  virtual void ScheduleLater(std::chrono::milliseconds duration,
                             std::function<void()> f) override {
    timer_->ScheduleAfter(duration, f);
  }
  virtual void Schedule(std::function<void()> f) override {
    threadpool_->Push(f);
  }

 private:
  std::shared_ptr<ThreadPool> threadpool_;
  std::shared_ptr<ananas::TimerManager> timer_;
};

int main(void) {
  std::shared_ptr<ananas::TimerManager> timer =std::make_shared<ananas::TimerManager>();
  std::shared_ptr<ThreadPool> tp = std::make_shared<ThreadPool>();
  tp->Start(2);

  std::shared_ptr<FutureSchedule> sche = std::make_shared<FutureSchedule>(tp, timer);
  {
    auto future = tp->Push([] {
      std::this_thread::sleep_for(std::chrono::seconds(2));
      PRINT("executor 1.\n");
    });

    future.Then([] { PRINT("executor 2.\n"); })
        .Then(sche.get(), [] { PRINT("executor 3.\n"); })
        .Then([] { PRINT("executor 4.\n"); })
        .Then([] { PRINT("executor 5.\n"); })
        .OnTimeout(
            std::chrono::seconds(1), [] { PRINT("OnTimeout.\n"); },
            sche.get());
  }

  while (true) {
    timer->Update();
  }
  return 0;
}

// TODO: 未实现内容
//  1 - 线程池JoinAll所有线程推出