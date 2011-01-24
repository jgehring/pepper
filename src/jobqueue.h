/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2011 Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
 *
 * file: jobqueue.h
 * Simple thread-safe job queue
 */


#ifndef JOBQUEUE_H_
#define JOBQUEUE_H_


#include <map>
#include <queue>
#include <vector>

#include "syslib/parallel.h"

#include "logger.h"


template <typename Arg, typename Result>
class JobQueue
{
	public:
		JobQueue(size_t max = 512) : m_max(max), m_end(false) { }

		void put(const std::vector<Arg> &args) {
			m_mutex.lock();
			for (unsigned int i = 0; i < args.size(); i++) {
				m_queue.push(args[i]);
				m_status[args[i]] = -1;
			}
			m_mutex.unlock();
			m_argWait.wakeAll();
		}

		void stop() {
			m_mutex.lock();
			m_end = true;
			m_mutex.unlock();
			m_argWait.wakeAll();
			m_resultWait.wakeAll();
		}

		bool getArg(Arg *arg) {
			m_mutex.lock();
			while (!m_end && (m_queue.empty() || m_results.size() > m_max)) {
				m_argWait.wait(&m_mutex);
			}
			if (m_end) {
				m_mutex.unlock();
				return false;
			}
			*arg = m_queue.front();
			m_queue.pop();
			m_mutex.unlock();
			return true;
		}

		bool hasArg(const Arg &arg) {
			m_mutex.lock();
			bool has = (m_status.find(arg) != m_status.end());
			m_mutex.unlock();
			return has;
		}

		bool getResult(const Arg &arg, Result *res) {
			m_mutex.lock();
			if (m_status.find(arg) == m_status.end()) {
				m_mutex.unlock();
				return false;
			}
			while (!m_end && m_status[arg] < 0) {
				m_resultWait.wait(&m_mutex);
			}
			if (m_end || !m_status[arg]) {
				m_mutex.unlock();
				return false;
			}
			*res = m_results[arg];
			m_results.erase(arg);
			m_status.erase(arg);
			m_mutex.unlock();
			m_argWait.wake();
			return true;
		}

		void done(const Arg &arg, const Result &result) {
			m_mutex.lock();
			m_results[arg] = result;
			m_status[arg] = 1;
#ifdef DEBUG
			PTRACE << arg << " ok, " << m_results.size() << " results in queue" << endl;
#endif
			m_mutex.unlock();
			m_resultWait.wakeAll();
		}
		
		void failed(const Arg &arg) {
			m_mutex.lock();
			m_status[arg] = 0;
#ifdef DEBUG
			PTRACE << arg << " FAILED, " << m_results.size() << " results in queue" << endl;
#endif
			m_mutex.unlock();
			m_resultWait.wakeAll();
		}

	private:
		sys::parallel::Mutex m_mutex;
		sys::parallel::WaitCondition m_argWait, m_resultWait;
		std::queue<Arg> m_queue;
		std::map<Arg, Result> m_results;
		std::map<Arg, int> m_status;
		size_t m_max;
		bool m_end;
};


#endif // JOBQUEUE_H_
