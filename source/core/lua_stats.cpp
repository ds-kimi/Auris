// auris.GetStats(): numbers only, so an addon can graph the pool's cost
// without learning anything about the server beyond its own transcription load
#include "lua_bindings.h"
#include "worker_stats.h"

using namespace GarrysMod::Lua;

static void PushWorker(ILuaBase* LUA, const WorkerStatsView& w) {
    LUA->CreateTable();
    LUA->PushNumber((double)w.jobs);       LUA->SetField(-2, "jobs");
    LUA->PushNumber(w.cpuSeconds * 1000.0); LUA->SetField(-2, "cpu_ms");
    LUA->PushNumber(w.audioSeconds);       LUA->SetField(-2, "audio_seconds");
    LUA->PushNumber(w.core);               LUA->SetField(-2, "core");
}

static void PushTotals(ILuaBase* LUA, const PoolStatsView& view) {
    double jobs = 0.0, cpu = 0.0, audio = 0.0;
    for (const WorkerStatsView& w : view.workers) {
        jobs += (double)w.jobs;
        cpu += w.cpuSeconds;
        audio += w.audioSeconds;
    }
    LUA->PushNumber(jobs);                      LUA->SetField(-2, "jobs");
    LUA->PushNumber((double)view.dropped);      LUA->SetField(-2, "dropped");
    LUA->PushNumber(cpu * 1000.0);              LUA->SetField(-2, "cpu_ms");
    LUA->PushNumber(audio);                     LUA->SetField(-2, "audio_seconds");
    LUA->PushNumber(audio > 0.0 ? cpu / audio : 0.0); LUA->SetField(-2, "cpu_per_audio");
    LUA->PushNumber(view.queueSeconds);         LUA->SetField(-2, "queue_seconds");
    LUA->PushNumber(view.activeWorkers);        LUA->SetField(-2, "active_workers");
}

// auris.GetStats() -> table
LUA_FUNCTION(Whisper_GetStats) {
    PoolStatsView view = StatsSnapshot();
    LUA->CreateTable();
    PushTotals(LUA, view);
    LUA->CreateTable();
    for (size_t i = 0; i < view.workers.size(); i++) {
        LUA->PushNumber((double)(i + 1));
        PushWorker(LUA, view.workers[i]);
        LUA->SetTable(-3);
    }
    LUA->SetField(-2, "workers");
    return 1;
}
