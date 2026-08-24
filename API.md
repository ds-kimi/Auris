# Auris — Submodule API

Auris is a silent platform. It loads the transcription module, polls results, and fires a hook. Your addon listens to that hook and does whatever it wants with the result.

---

## Receiving Transcriptions

Register a callback with `Auris.Subscribe`. Auris calls it once per transcription result.

```lua
Auris.Subscribe("MyAddon_Feature", function(ply, steamid64, text, audio)
    -- your logic here
end)
```

| Parameter | Type | Notes |
|---|---|---|
| `ply` | `GPlayer` or `nil` | `nil` if the player disconnected before the result arrived |
| `steamid64` | `string` | Always present — use this as your reliable identifier |
| `text` | `string` | The transcribed speech. May be whitespace-only (whisper hallucination) |
| `audio` | `string` or `nil` | Raw PCM binary (16 kHz, mono, float32 little-endian). `nil` on a rare cache miss. Always guard with `if audio then`. Convert to WAV with `Auris.PCMToWAV(audio)` before writing or sending |

> **Note:** Internally Auris fires `hook.Run("Auris_Transcription", ...)`. Do not use `hook.Add` on this hook directly — `Auris.Subscribe` adds duplicate-detection and namespacing that raw `hook.Add` skips.

---

## Filtering — Two Levels

Auris exposes two independent filtering mechanisms. They solve different problems and can be combined.

### Level 1 — Global C++ gate (`Auris.SetFilter`)

This is a **server-wide predicate** evaluated before any transcription work begins. Players that fail it are ignored completely — their voice buffer is never flushed, whisper never runs, no CPU is spent. Every subscriber on the server is affected.

Use this when you want to permanently restrict which players Auris processes at all — e.g. only a specific DarkRP job, only staff, only a whitelist.

```lua
-- DarkRP: only transcribe police job
Auris.SetFilter(function(ply)
    return ply:Team() == TEAM_POLICE
end)

-- Usergroup: superadmin only
Auris.SetFilter(function(ply)
    return ply:IsSuperAdmin()
end)

-- Custom ULX group
Auris.SetFilter(function(ply)
    return ply:IsUserGroup("staff")
end)

-- Clear — transcribe everyone (default)
Auris.SetFilter(nil)
```

> **Warning:** `SetFilter` is global. If your addon sets one, every other subscriber on the server is also restricted to that set of players. When a subscriber registers while a global filter is active, Auris will print a console warning so the conflict is visible immediately.

### Level 2 — Per-subscriber filter (`Auris.Subscribe` third arg)

This is a **per-callback predicate** evaluated after transcription is already complete. The transcription still ran — this filter only decides whether your specific callback fires for a given result. Other subscribers are unaffected.

Use this when transcription should still happen for everyone, but your addon only cares about a subset.

```lua
-- fires only for police, but other subscribers still see everyone
Auris.Subscribe("MyAddon_Police", function(ply, steamid64, text, audio)
    -- only cops reach here
end, function(ply)
    return IsValid(ply) and ply:Team() == TEAM_POLICE
end)
```

### Which one to use?

| | `SetFilter` | `Subscribe` filter |
|---|---|---|
| Stops transcription entirely | Yes — no CPU used | No — transcription already ran |
| Affects all subscribers | Yes | No — only your callback |
| Use when | You control the server and want a hard gate | You only need to narrow your own callback |

> The two levels stack: a player must pass `SetFilter` before transcription runs; the per-subscriber filter then gates individual callbacks on top of that.

### Filtering Subscribers

Pass a predicate as the third argument to restrict which transcriptions reach your callback. The predicate receives the same four arguments as the callback. When the predicate returns `false` or `nil`, the callback is skipped — no overhead beyond the filter call itself.

```lua
-- No filter: every transcription fires the callback (default behaviour)
Auris.Subscribe("MyAddon_All", function(ply, steamid64, text, audio)
    -- fires for everyone
end)

-- DarkRP job filter: only players in a specific job
Auris.Subscribe("MyAddon_Police", function(ply, steamid64, text, audio)
    -- only runs for cops
end, function(ply)
    return IsValid(ply) and ply:Team() == TEAM_POLICE
end)

-- DarkRP team by name (more portable across server configs)
Auris.Subscribe("MyAddon_Medics", function(ply, steamid64, text, audio)
    -- only runs for medics
end, function(ply)
    if not IsValid(ply) then return false end
    local jobTable = DarkRP.getJobByCommand(ply:getDarkRPVar("job") or "")
    return jobTable and jobTable.category == "Medical"
end)

-- Usergroup filter: superadmin only
Auris.Subscribe("MyAddon_AdminLog", function(ply, steamid64, text, audio)
    -- only runs for superadmins
end, function(ply)
    return IsValid(ply) and ply:IsSuperAdmin()
end)

-- Custom usergroup (ULX / serverside group)
Auris.Subscribe("MyAddon_StaffLog", function(ply, steamid64, text, audio)
    -- only runs for staff group members
end, function(ply)
    return IsValid(ply) and ply:IsUserGroup("staff")
end)
```

### Backends

Auris ships with two backends selected by `config.lua`:

- **Local (default)** — `openai_api_key = ""`, runs whisper.cpp on the server host.
- **Remote** — `openai_api_key = "sk-..."`, forwards each clip to OpenAI's `/v1/audio/transcriptions`.

Both fire the same hook with identical arguments. In remote mode the `audio` hook arg is still supplied — it is the exact PCM sent to OpenAI — so audio-archival submodules (e.g. [auris-discord](../auris-discord/)) work unchanged.

---

## API Functions

```lua
-- Set a global gate. Only players for whom fn(ply) returns true will have their
-- voice buffered and transcribed. Players excluded here cost zero CPU — no flush,
-- no whisper work, no hook. Pass nil to clear the filter and transcribe everyone.
Auris.SetFilter(fn)

-- Register a listener. name must be globally unique across all installed submodules.
-- Convention: prefix with your addon folder name, e.g. "MyAddon_Feature"
-- filter is optional. When provided, callback only fires when filter returns truthy.
-- filter receives the same args as callback: (ply, steamid64, text, audio)
Auris.Subscribe(name, callback, filter)

-- Remove a listener.
Auris.Unsubscribe(name)

-- Returns a shallow copy of the active config table. Do not mutate it.
-- Keys: model, threads, language, debug, print_progress, print_timestamps,
--       single_segment, no_context
Auris.GetConfig()

-- Returns true once auris has initialised successfully and the poll loop is live.
Auris.IsReady()

-- Semver string. Guard against breaking changes with a version check.
Auris.VERSION  -- e.g. "1.0.0"

-- Wraps a raw PCM binary (the `audio` callback argument) in a WAV container.
-- Returns a complete WAV file as a Lua string — ready to pass to file.Write()
-- or an HTTP multipart body. Format: 16 kHz, mono, 32-bit IEEE 754 float PCM.
-- Always guard the input: if audio then Auris.PCMToWAV(audio) end
Auris.PCMToWAV(audio)
```

### Low-level module functions

These are exposed directly on the `auris` module table (lowercase), not on the `Auris` Lua wrapper. You only need them if you are building a custom backend or capturing audio outside the normal transcription pipeline.

```lua
-- Returns the buffered PCM for a player as a raw binary string and clears the
-- buffer, without queuing a whisper job. Returns nil if the buffer is empty.
-- The key is the player's AccountID (ply:AccountID()) — the low 32 bits of the
-- SteamID64, which is what the voice detour keys its buffers by. Passing a
-- UserID looks up an empty buffer and returns nil for every utterance.
-- Use this to capture audio without transcribing — save to disk, forward to a
-- custom ASR endpoint, etc.
auris.FlushRaw(accountid)

-- Keeps silence in captured audio so a clip is as long as it took to speak.
-- Off by default. See "Capturing audio on a timeline" below.
auris.SetPreserveTimeline(bool)
auris.PreservesTimeline()
```

---

## Working With Audio

Every `Auris.Subscribe` callback receives the raw voice recording as its 4th argument. This lets submodules archive, forward, or analyse the audio independently — the core never writes files itself.

### Save a clip to disk (after transcription)

```lua
Auris.Subscribe("MyAddon_SaveClip", function(ply, steamid64, text, audio)
    if not audio then return end
    file.CreateDir("myaddon/clips")
    file.Write("myaddon/clips/" .. steamid64 .. "_" .. os.time() .. ".wav",
        Auris.PCMToWAV(audio))
end)
```

### Save audio without transcribing

Auris fires `Auris_VoiceEnd` on the server the moment a player stops speaking, before any transcription work begins. Return `true` from the hook to consume the audio and skip transcription entirely.

```lua
hook.Add("Auris_VoiceEnd", "MyAddon_SaveRaw", function(ply)
    if not IsValid(ply) then return end

    local raw = auris.FlushRaw(ply:AccountID())
    if not raw then return end

    local dir = "auris_clips"
    file.CreateDir(dir)

    -- steamid64 + unix timestamp gives a unique, sortable filename per player.
    local filename = dir .. "/" .. ply:SteamID64() .. "_" .. os.time() .. ".wav"
    file.Write(filename, Auris.PCMToWAV(raw))

    return true -- skip transcription for this utterance
end)
```

> **Note:** Returning `true` prevents transcription — `Auris.Subscribe` callbacks will not fire for that utterance. Omit the `return true` if you want both the saved file and a transcript (Auris will flush the buffer itself on the next tick, but `FlushRaw` already drained it, so transcription will be skipped regardless). If you need both, use `Auris.Subscribe` to save the audio after transcription instead.

### Capturing audio on a timeline

Auris captures voiced audio only. Steam's voice stream marks pauses with a silence
opcode rather than sending samples, and a talker who goes quiet simply stops
transmitting — so by default both are dropped and the buffer is a run of speech
with the gaps closed up.

That is the right trade for transcription, which does not care where the pauses
were and pays for every extra sample. It is wrong for anything that plays the
audio back against the clock it was spoken on: a sentence with breaths in it
comes out shorter than it was spoken, and every word after a pause lands earlier
than it happened. An eight-second sentence can arrive as two seconds of audio.

Turn on timeline fidelity when you want the recording rather than the words:

```lua
-- In lua/auris/config.lua
preserve_timeline = true,
```

```lua
-- Or at runtime, before the utterance is spoken
auris.SetPreserveTimeline(true)
```

With it on, silence is written back into the buffer and gaps between packets are
padded against the clock, so `#audio / 4 / 16000` is the real duration of the
utterance and a clip lines up with whatever else you recorded alongside it.

> **Timing note:** padding happens as packets arrive, so flipping the flag
> mid-sentence only affects what comes after it. The flag is global to the
> server, not per player.

### Send to an HTTP endpoint

GMod's built-in `HTTP()` supports a binary `body` — no extra module needed for simple POST requests:

```lua
Auris.Subscribe("MyAddon_Upload", function(ply, steamid64, text, audio)
    if not audio then return end
    HTTP({
        method  = "POST",
        url     = "https://your-api.example.com/voice",
        body    = Auris.PCMToWAV(audio),
        type    = "audio/wav",
        success = function() end,
        failed  = function(err) MsgC(Color(255,80,80), err .. "\n") end,
    })
end)
```

> **Multipart / Discord file uploads** require [gmsv_reqwest](https://github.com/williamvenner/gmsv_reqwest). GMod's `HTTP()` has no multipart support, so it cannot attach a file alongside JSON (e.g. Discord webhooks with an audio attachment). See [auris-discord](../auris-discord/) for a working multipart example.

### Audio format reference

| Property | Value |
|---|---|
| Container | WAV (RIFF) after `PCMToWAV`; raw float32 binary before |
| Sample rate | 16 000 Hz |
| Channels | 1 (mono) |
| Bit depth | 32-bit IEEE 754 float (audioFormat = 3) |
| Byte order | Little-endian |
| Sample range | ≈ −1.0 to +1.0 |
| Duration | `#audio / 4 / 16000` seconds (raw binary) |

> The `audio` binary is the exact buffer passed to whisper.cpp for transcription — the same audio that produced the `text` argument.

---

## Load Order

GMod does not guarantee autorun execution order across addons. Always wrap your init in `timer.Simple(0, ...)` to defer until after all autorun files have run.

```lua
timer.Simple(0, function()
    if not Auris then
        ErrorNoHalt("[myaddon] Auris core not found\n")
        return
    end
    -- safe to subscribe here
end)
```

---

## Minimal Example — Console Logger

Prints every transcription to the server console with the speaker's name and SteamID.


### `sv_logger_init.lua`

```lua
-- Deferred so Auris core is guaranteed to have loaded regardless of
-- which autorun folder GMod processed first.
timer.Simple(0, function()
    if not Auris then
        ErrorNoHalt("[auris-logger] Auris core not found — is it installed?\n")
        return
    end

    Auris.Subscribe("Logger_Console", function(ply, steamid64, text, audio)
        -- ply is nil when the player disconnected before transcription finished;
        -- fall back to the SteamID so the log line is still useful.
        local name = IsValid(ply) and ply:Nick() or "Disconnected"
        Msg("[Auris] " .. name .. " (" .. steamid64 .. "): " .. text .. "\n")
        -- audio is also available here if you want to do something with the clip
    end)
end)
```

That's the entire addon. No other files needed.

---

## Subscriber Name Convention

The name passed to `Auris.Subscribe` must be unique across every installed submodule. If two addons use the same name, the second one silently overwrites the first.

Use your addon folder name as a prefix:

```
auris-logger    →  "Logger_Console"
auris-badwords  →  "BadWords_Detector"
auris-commands  →  "Commands_Handler"
```

---

## Handling Disconnected Players

Always check `ply` before acting on a live player:

```lua
Auris.Subscribe("MyAddon_Feature", function(ply, steamid64, text)
    if not IsValid(ply) then
        -- player left before transcription finished; log by SteamID only
        return
    end
    -- safe to call ply:Nick(), ply:Kick(), etc.
end)
```

---

## Version Guard

If your addon uses API features that may not exist in older Auris versions, guard on load:

```lua
timer.Simple(0, function()
    if not Auris then ErrorNoHalt("[myaddon] Auris not found\n") return end

    local major = tonumber(string.match(Auris.VERSION, "^(%d+)"))
    if major < 2 then
        ErrorNoHalt("[myaddon] Requires Auris 1.1.2+, found " .. Auris.VERSION .. "\n")
        return
    end

    Auris.Subscribe("MyAddon_Feature", function(ply, sid, text, audio)
        -- audio available on Auris 1.1.2+
    end)
end)
```

---

## Publishing Your Submodule

### Step 1 — README

Your repo **must** have a README that follows the official template. Open [SUBMODULE_README_TEMPLATE.md](SUBMODULE_README_TEMPLATE.md) in your editor, copy everything below the scissors line, paste it as your `README.md`, and fill in every placeholder. Do not skip the **Auris Submodule Info** table — that is what we display in the community list.

### Step 2 — Open an issue

Go to [github.com/ds-kimi/Auris/issues/new/choose](https://github.com/ds-kimi/Auris/issues/new/choose) and select **Submit Submodule**. Fill out every field and check all boxes in the compliance checklist. Incomplete submissions will not be reviewed.

### Compliance checklist

All of the following must be true before submitting:

- Init wrapped in `timer.Simple(0, ...)`
- Guards missing Auris with `if not Auris then ErrorNoHalt(...) return end`
- Subscriber key prefixed with your addon folder name
- No ConVar uses the `auris_` prefix
- Repo README follows the submodule template
- Addon never calls `auris.*` directly — only uses the `Auris` API
- If consuming `audio`, guarded with `if audio then` (4th arg can be `nil` on cache miss)
- WAV conversion done via `Auris.PCMToWAV(audio)` — not a custom reimplementation

Submissions that fail any of these will not be listed.
