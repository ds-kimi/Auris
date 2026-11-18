// srcds runs its tick at normal priority: transcription must lose every contest
// for a core, so a busy pool delays transcripts instead of stuttering the server.
#include "thread_priority.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/resource.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif

void LowerCurrentThreadPriority() {
#ifdef _WIN32
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
#else
    // on Linux the nice value is per thread when addressed by its tid
    setpriority(PRIO_PROCESS, (id_t)syscall(SYS_gettid), 10);
#endif
}
