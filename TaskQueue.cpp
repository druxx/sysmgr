#include <queue>
#include <time.h>
#include <unistd.h>

#include "TaskQueue.h"

TaskQueue::taskid_t TaskQueue::schedule(time_t when, callback<void> cb, void *cb_data)
{
	scope_lock llock(&this->lock);

	TaskQueue::taskid_t id = this->next_id++;
	queue.push(ScheduledTask(id, when, cb, cb_data));
	return id;
}

void TaskQueue::cancel(TaskQueue::taskid_t id)
{
	scope_lock llock(&this->lock);

	std::priority_queue< ScheduledTask, std::vector<ScheduledTask>, std::greater<ScheduledTask> > newq;

	while (!queue.empty()) {
		ScheduledTask e = queue.top();
		if (e.id != id)
			newq.push(e);
		queue.pop();
	}
	queue = newq;
}

int TaskQueue::run_until(time_t stop_after)
{
	int processed = 0;
	time_t now = time(NULL);
	while (1) {
		scope_lock llock(&this->lock);

		ScheduledTask e = queue.top();
		if (e.when <= now) {
			queue.pop();

			llock.unlock();
			e.cb(e.cb_data);
			llock.lock();

			processed++;
		}
		else {
			if (stop_after && stop_after < now)
				return processed;
			else {
				sleep(1);
				now = time(NULL);
			}
		}
	}
}