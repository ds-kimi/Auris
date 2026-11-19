// Optional (auto_workers) adaptive count of workers allowed to take jobs
#pragma once

// enabled=false keeps every worker active
void TunerReset(int maxWorkers, bool enabled);
// Feeds how long a job waited in the queue, in seconds
void TunerObserve(double waitSeconds);
// Worker indices below the active count may take jobs
bool TunerWorkerActive(int worker);
int TunerActiveWorkers();
