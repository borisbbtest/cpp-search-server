#include <map>
#include <string>
#include <tuple>
#include <vector>
#include <iostream>
#include <algorithm>

using namespace std;

// Перечислимый тип для статуса задачи
enum class TaskStatus {
	  NEW,          // новая
	  IN_PROGRESS,  // в разработке
	  TESTING,      // на тестировании
	  DONE          // завершена
};

// Объявляем тип-синоним для map<TaskStatus, int>,
// позволяющего хранить количество задач каждого статуса
using TasksInfo = map<TaskStatus, int>;

class TeamTasks {
public:
	  // Получить статистику по статусам задач конкретного разработчика
	  const TasksInfo& GetPersonTasksInfo(const string& person) const {
          return persons_.at(person);
      }

	  // Добавить новую задачу (в статусе NEW) для конкретного разработчитка
	  void AddNewTask(const string& person){
         ++persons_[person][TaskStatus::NEW];
      }
	  // Обновить статусы по данному количеству задач конкретного разработчика,
	  // подробности см. ниже
	  tuple<TasksInfo, TasksInfo> PerformPersonTasks(const string& person, int task_count){
          TasksInfo untouched=persons_[person],updated;
          for (const auto& [status_task,count_task] : persons_[person]){
              if(count_task>0 && task_count > 0) {
                 int x=count_task-task_count; // -1 +1
                 switch(status_task){
                      case TaskStatus::NEW:
                         if(x>=0){
                             untouched[TaskStatus::NEW]=count_task-task_count;
                             updated[TaskStatus::IN_PROGRESS]=task_count;
                             task_count=0;
                             untouched[TaskStatus::DONE]=0;
                             update(untouched,updated,person);
                             return tuple(updated,untouched);
                         }
                         else{
                             task_count=task_count-count_task;
                             untouched[TaskStatus::NEW]=0;
                             updated[TaskStatus::IN_PROGRESS]=updated[TaskStatus::IN_PROGRESS]+count_task;
                         }
                         break;
                      case TaskStatus::IN_PROGRESS:
                        if(x>=0){
                             untouched[TaskStatus::IN_PROGRESS]=count_task-task_count;
                             updated[TaskStatus::TESTING]=task_count;
                             task_count=0;
                             untouched[TaskStatus::DONE]=0;
                             update(untouched,updated,person);
                             return tuple(updated,untouched);
                         }
                         else{
                             task_count=task_count-count_task;
                             untouched[TaskStatus::IN_PROGRESS]=0;
                             updated[TaskStatus::TESTING]=updated[TaskStatus::TESTING]+count_task;
                         }
                         break;
                      case TaskStatus::TESTING:
                         if(x>=0){
                             untouched[TaskStatus::TESTING]=count_task-task_count;
                             updated[TaskStatus::DONE]=task_count;
                             task_count=0;
                             untouched[TaskStatus::DONE]=0;
                             update(untouched,updated,person);
                             return tuple(updated,untouched);
                         }
                         else{
                             task_count=task_count-count_task;
                             untouched[TaskStatus::TESTING]=0;
                             updated[TaskStatus::DONE]=updated[TaskStatus::DONE]+count_task;
                         }
                         break;
                      case TaskStatus::DONE:
                         break;
                     default:
                         break;
                 }
              }
          }
          untouched[TaskStatus::DONE]=0;
          update(untouched,updated,person);
          return tuple(updated,untouched);
      }
private:
      void update(const TasksInfo& untouched,const TasksInfo& updated,const string& person){
           persons_[person]=untouched;
           for (const auto& [status_task,count_task] : updated){
                persons_[person][status_task]+=count_task;
           }
      }
      map<string,TasksInfo> persons_;

};

// Принимаем словарь по значению, чтобы иметь возможность
// обращаться к отсутствующим ключам с помощью [] и получать 0,
// не меняя при этом исходный словарь.

/*
void PrintTasksInfo(TasksInfo tasks_info) {
    cout << tasks_info[TaskStatus::NEW] << " new tasks" <<
        ", " << tasks_info[TaskStatus::IN_PROGRESS] << " tasks in progress" <<
        ", " << tasks_info[TaskStatus::TESTING] << " tasks are being tested" <<
        ", " << tasks_info[TaskStatus::DONE] << " tasks are done" << endl;
}

int main() {
    TeamTasks tasks;
    tasks.AddNewTask("Ilia");
    for (int i = 0; i < 3; ++i) {
        tasks.AddNewTask("Ivan");
    }
    cout << "Ilia's tasks: ";
    PrintTasksInfo(tasks.GetPersonTasksInfo("Ilia"));
    cout << "Ivan's tasks: ";
    PrintTasksInfo(tasks.GetPersonTasksInfo("Ivan"));
    TasksInfo updated_tasks, untouched_tasks;
    tie(updated_tasks, untouched_tasks) = tasks.PerformPersonTasks("Ivan", 2);
    cout << "Updated Ivan's tasks: ";
    PrintTasksInfo(updated_tasks);
    cout << "Untouched Ivan's tasks: ";
    PrintTasksInfo(untouched_tasks);
    tie(updated_tasks, untouched_tasks) = tasks.PerformPersonTasks("Ivan", 2);
    cout << "Updated Ivan's tasks: ";
    PrintTasksInfo(updated_tasks);
    cout << "Untouched Ivan's tasks: ";
    PrintTasksInfo(untouched_tasks);
} */

void PrintTasksInfo(const TasksInfo& tasks_info) {
    if (tasks_info.count(TaskStatus::NEW)) {
        std::cout << "NEW: " << tasks_info.at(TaskStatus::NEW) << " ";
    }
    if (tasks_info.count(TaskStatus::IN_PROGRESS)) {
        std::cout << "IN_PROGRESS: " << tasks_info.at(TaskStatus::IN_PROGRESS) << " ";
    }
    if (tasks_info.count(TaskStatus::TESTING)) {
        std::cout << "TESTING: " << tasks_info.at(TaskStatus::TESTING) << " ";
    }
    if (tasks_info.count(TaskStatus::DONE)) {
        std::cout << "DONE: " << tasks_info.at(TaskStatus::DONE) << " ";
    }
}
void PrintTasksInfo(const TasksInfo& updated_tasks, const TasksInfo& untouched_tasks) {
    std::cout << "Updated: [";
    PrintTasksInfo(updated_tasks);
    std::cout << "] ";
    std::cout << "Untouched: [";
    PrintTasksInfo(untouched_tasks);
    std::cout << "] ";
    std::cout << std::endl;
}
int main() {
    TeamTasks tasks;
    TasksInfo updated_tasks;
    TasksInfo untouched_tasks;
    /* TEST 3 */
    std::cout << "\nLisa" << std::endl;
    for (auto i = 0; i < 5; ++i) {
        tasks.AddNewTask("Lisa");
    }
    tie(updated_tasks, untouched_tasks) = tasks.PerformPersonTasks("Lisa", 5);
    PrintTasksInfo(updated_tasks, untouched_tasks);  // [{"IN_PROGRESS": 5}, {}]
    tie(updated_tasks, untouched_tasks) = tasks.PerformPersonTasks("Lisa", 5);
    PrintTasksInfo(updated_tasks, untouched_tasks);  // [{"TESTING": 5}, {}]
    tie(updated_tasks, untouched_tasks) = tasks.PerformPersonTasks("Lisa", 1);
    PrintTasksInfo(updated_tasks, untouched_tasks);  // [{"DONE": 1}, {"TESTING": 4}]
    for (auto i = 0; i < 5; ++i) {
        tasks.AddNewTask("Lisa");
    }
    tie(updated_tasks, untouched_tasks) = tasks.PerformPersonTasks("Lisa", 2);
    PrintTasksInfo(updated_tasks, untouched_tasks);  // [{"IN_PROGRESS": 2}, {"NEW": 3, "TESTING": 4}]
    PrintTasksInfo(tasks.GetPersonTasksInfo("Lisa"));  // {"NEW": 3, "IN_PROGRESS": 2, "TESTING": 4, "DONE": 1}
    std::cout << std::endl;
    tie(updated_tasks, untouched_tasks) = tasks.PerformPersonTasks("Lisa", 4);
    PrintTasksInfo(updated_tasks, untouched_tasks);  // [{"IN_PROGRESS": 3, "TESTING": 1}, {"IN_PROGRESS": 1, "TESTING": 4}]
    PrintTasksInfo(tasks.GetPersonTasksInfo("Lisa"));  // {"IN_PROGRESS": 4, "TESTING": 5, "DONE": 1}
    std::cout << std::endl;
    tie(updated_tasks, untouched_tasks) = tasks.PerformPersonTasks("Lisa", 5);
    PrintTasksInfo(updated_tasks, untouched_tasks);  // [{"TESTING": 4, "DONE": 1}, {"TESTING": 4}]
    PrintTasksInfo(tasks.GetPersonTasksInfo("Lisa"));  // {"TESTING": 8, "DONE": 2}
    std::cout << std::endl;
    tie(updated_tasks, untouched_tasks) = tasks.PerformPersonTasks("Lisa", 10);
    PrintTasksInfo(updated_tasks, untouched_tasks);  // [{"DONE": 8}, {}]
    PrintTasksInfo(tasks.GetPersonTasksInfo("Lisa"));  // {"DONE": 10}
    std::cout << std::endl;
    tie(updated_tasks, untouched_tasks) = tasks.PerformPersonTasks("Lisa", 10);
    PrintTasksInfo(updated_tasks, untouched_tasks);  // [{}, {}]
    PrintTasksInfo(tasks.GetPersonTasksInfo("Lisa"));  // {"DONE": 10}
    std::cout << std::endl;
    tasks.AddNewTask("Lisa");
    PrintTasksInfo(tasks.GetPersonTasksInfo("Lisa"));  // {"NEW": 1, "DONE": 10}
    std::cout << std::endl;
    tie(updated_tasks, untouched_tasks) = tasks.PerformPersonTasks("Lisa", 2);
    PrintTasksInfo(updated_tasks, untouched_tasks);  // [{"IN_PROGRESS": 1}, {}]
    PrintTasksInfo(tasks.GetPersonTasksInfo("Lisa"));  // {"IN_PROGRESS": 1, "DONE": 10}
    std::cout << std::endl;
    tie(updated_tasks, untouched_tasks) = tasks.PerformPersonTasks("Bob", 3);
    PrintTasksInfo(updated_tasks, untouched_tasks);  // [{}, {}]
    return 0;
}