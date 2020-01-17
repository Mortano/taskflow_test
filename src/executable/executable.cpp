
#include <taskflow/taskflow.hpp>

#include <chrono>
#include <iostream>
#include <mutex>
#include <random>
#include <sstream>
#include <string>

static std::mt19937 s_rng{
    std::chrono::high_resolution_clock::now().time_since_epoch().count()};

/**
 * Record all tasks that are executed in order. I'm using a custom observer here
 * so that printing the tasks is cleaner, but the tf::ExecutorObserver also
 * stores essentially std::vector<tf::TaskView>.
 */
struct CustomExecutorObserver : tf::ExecutorObserverInterface {
  virtual void on_entry(unsigned worker_id, tf::TaskView task_view) override {
    executed_tasks.push_back(task_view); // If I store task_view.name() here, I
                                         // get the expected result
  }
  virtual void on_exit(unsigned worker_id, tf::TaskView task_view) override {}

  std::vector<tf::TaskView> executed_tasks;
};

template <typename... Args> std::string concat(Args &&... args) {
  std::stringstream ss;
  (ss << ... << args);
  return ss.str();
}

template <typename RNG>
std::vector<std::string> generate_random_task_names(size_t count, RNG &rng) {
  std::vector<std::string> random_name_pool = {"A", "B", "C", "D",
                                               "E", "F", "G", "H"};

  auto random_names = random_name_pool;
  std::shuffle(std::begin(random_names), std::end(random_names), rng);

  return {std::begin(random_names), std::begin(random_names) + count};
}

template <typename Taskflow>
void run_recursive_task(const std::string &identifier, Taskflow &taskflow) {
  std::cout << identifier << "\n";
  if (identifier.size() >= 20)
    return; // Don't generate too many tasks

  // Generate between 0 and 2 new tasks. This count varies from execution to
  // execution!
  std::uniform_int_distribution<int> count_dist{0, 2};
  const auto tasks_to_spawn = count_dist(s_rng);
  // Give tasks random names to distinguish them better. Task names are
  // cumulative and indicate the parent/child relationship. Think of these tasks
  // like nodes in a tree, where the name is just a concatenation of all parent
  // nodes
  const auto task_names = generate_random_task_names(tasks_to_spawn, s_rng);

  for (auto idx = 0; idx < tasks_to_spawn; ++idx) {
    const auto subtask_name = concat(identifier, task_names[idx]);
    taskflow
        .emplace([subtask_name](tf::Subflow &subflow) {
          run_recursive_task(subtask_name, subflow);
        })
        .name(subtask_name);
  }
}

int main(int argc, char **argv) {
  // Run only a single thread to see the order of execution
  tf::Executor executor{1};
  tf::Taskflow taskflow;
  const auto observer = executor.make_observer<CustomExecutorObserver>();

  const std::string root_task_identifier = "r";
  taskflow
      .emplace([&root_task_identifier](tf::Subflow &subflow) {
        run_recursive_task(root_task_identifier, subflow);
      })
      .name(root_task_identifier);

  constexpr static auto NUM_RUNS = 10;

  std::cout << "Running the taskflow...\n";
  for (auto run = 0; run < NUM_RUNS; ++run) {

    executor.run(taskflow).wait();
  }

  std::cout
      << "Executor observer recorded the following task execution order:\n";
  for (auto &executed_task : observer->executed_tasks) {
    std::cout << executed_task.name() << "\n";
  }

  /**
   * I would expect that the Observer records the same order of execution that
   * the tasks themselves print to cout. In particular, there is an inherent
   * order that any task with name X must occur before all tasks with names
   * longer than X and starting with X (i.e. tasks that are spawned by X).
   *
   * If I run this program, two things happen:
   *   1) The order of execution is not the same
   *   2) I get broken strings for some of the TaskView objects in the Executor
   * (e.g. �<�AHAEBCCFC)
   *
   * This leads me to the assumption that what I am doing here is not supported
   * and some of the TaskView objects are pointing to corrupted nodes?
   *
   * I do get the same behaviour (wrong order, broken strings) when using
   * tf::ExecutorObserver!
   */

  return 0;
}