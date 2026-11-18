// Under load, fresh speech is worth more than a backlog nobody waits for anymore:
// dropping the oldest clips keeps transcription latency bounded.
#include "job_queue.h"
#include "debug_log.h"
#include <condition_variable>
#include <deque>
#include <mutex>

static std::mutex g_jobMutex;
static std::condition_variable g_jobCond;
static std::deque<TranscriptJob> g_jobs;
static size_t g_queuedSamples = 0;
static bool g_open = false;

void JobQueuePush(TranscriptJob job, size_t maxSamples) {
    std::lock_guard<std::mutex> lock(g_jobMutex);
    if (!g_open) return;
    g_queuedSamples += job.audio.size();
    g_jobs.push_back(std::move(job));
    // the newest job always stays, even when it alone exceeds the budget
    while (g_jobs.size() > 1 && g_queuedSamples > maxSamples) {
        size_t dropped = g_jobs.front().audio.size();
        WDEBUG("[Auris] Queue full: dropped %.1fs clip (key %d)\n",
               dropped / 16000.0f, g_jobs.front().key);
        g_queuedSamples -= dropped;
        g_jobs.pop_front();
    }
    g_jobCond.notify_one();
}

bool JobQueuePop(TranscriptJob& out) {
    std::unique_lock<std::mutex> lock(g_jobMutex);
    g_jobCond.wait(lock, [] { return !g_jobs.empty() || !g_open; });
    if (!g_open) return false;
    out = std::move(g_jobs.front());
    g_jobs.pop_front();
    g_queuedSamples -= out.audio.size();
    return true;
}

void JobQueueOpen() {
    std::lock_guard<std::mutex> lock(g_jobMutex);
    g_jobs.clear();
    g_queuedSamples = 0;
    g_open = true;
}

void JobQueueClose() {
    {
        std::lock_guard<std::mutex> lock(g_jobMutex);
        g_open = false;
        g_jobs.clear();
        g_queuedSamples = 0;
    }
    g_jobCond.notify_all();
}
