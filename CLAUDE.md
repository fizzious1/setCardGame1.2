# Big Brother AI Simulator — Project Handoff

## What This Project Is

A multi-platform Big Brother reality TV simulator with AI-driven contestants. The system has two consumer apps (web + desktop) sharing a single simulation core, plus a conditioning pipeline that uses RAG and LLM calls to create deeply characterized AI agents.

## Repository Structure

```
/shared/                    — Shared data contract (TypeScript types + demo-season.json)
/simulation-core/           — Rule-based simulation engine (TypeScript)
/web-control-room/          — React web app: surveillance-style control room UI
/desktop-prestige-house/    — Unreal Engine 5 C++ project: 3D house visualization
/pipeline/                  — LLM agent conditioning pipeline
/tests/                     — Acceptance test suites
```

## Architecture Overview

### Shared Data Contract (`/shared/types.ts`)
The single source of truth for all data structures consumed by both apps. Key types: `Season`, `Participant`, `Snapshot`, `GameEvent`, `DailyRecap`, `HighlightIndex`, `ParticipantState`, `PlaybackState`. The demo data lives in `/shared/demo-season.json` (960KB, 8 participants, 168 snapshots, 238 events, 7 daily recaps, 49 highlights over a 7-day season).

### Simulation Core (`/simulation-core/`)
Rule-based tick-by-tick engine (TypeScript). 1 tick = 1 hour, 24 ticks/day. Each tick, every participant runs through an AI decision loop:
1. **Room movement** — personality-driven (extroverts seek crowds, introverts seek solitude, allies gravitate together, enemies avoid)
2. **Social action** — probabilistic based on 5 personality traits (extraversion, agreeableness, strategy, emotionality, loyalty) + relationship state
3. **Mood/energy update** — actions shift mood, energy drains during day and recovers at night
4. **Trust update** — per-action deltas (alliance +0.2, conflict -0.25, betrayal -0.4)
5. **Goal setting** — contextual priority chain based on game state

Key files:
- `src/ai-behavior.ts` — Decision engine with seeded PRNG (mulberry32, seed 42)
- `src/engine.ts` — Main simulation loop
- `src/events.ts` — Event generation from AI decisions
- `src/participants.ts` — 8 hardcoded participants with personality traits
- `src/house.ts` — 9-room house layout with connections
- `src/recap.ts` — Daily recap generation
- `src/highlights.ts` — Highlight detection and scoring
- `src/generate-demo.ts` — Runs full 7-day sim, outputs to `/shared/demo-season.json`

**Current limitation:** This is a rule-based system with weighted random rolls, not actual AI reasoning. No learning, no planning, no theory of mind. The pipeline (below) is building the LLM-based replacement.

### Web Control Room (`/web-control-room/`)
React 18 + Vite + TypeScript. Dark-themed surveillance UI.

Components:
- `App.tsx` — 3-panel layout (participant list | house view + timeline | inspector/recap)
- `HouseView.tsx` — SVG cutaway of 9-room house with participant dots
- `Timeline.tsx` — Horizontal scrubber with event markers, day dividers
- `PlaybackControls.tsx` — Play/pause, 1x/2x/4x/8x speed, step, time display
- `ParticipantList.tsx` — Left sidebar with avatars, status badges, mood bars
- `ParticipantInspector.tsx` — Mood gauge, trust bars, personality radar, goal, recent events
- `DailyRecap.tsx` — Summary, key events, alliance map, drama score
- `EventPopup.tsx` — Modal with event details and impact

Hooks: `useSimulation.ts` (data access), `usePlayback.ts` (playback state)

Run: `cd web-control-room && npm install && npm run dev`

### Desktop Prestige House (`/desktop-prestige-house/`)
Unreal Engine 5.3 C++ project. 13 gameplay systems:
- `BBSimDataSubsystem` — Loads shared Season JSON, provides tick-indexed access
- `BBGameMode` — Tick progression, agent spawning, play/pause/speed
- `BBAgentCharacter` — Character with state machine (Idle/Walking/SocialAction/Confessing), mood indicator, name tag
- `BBAgentAIController` — Nav mesh pathfinding, animation triggers
- `BBHouseFloor` — Spawns room volumes from data
- `BBRoomVolume` — Room boundaries, nav points, event anchors
- `BBDirectorCamera` — Auto-directing cinematic camera (drama-weighted room scoring)
- `BBFollowCamera` — Spring arm character follow with orbit mode
- `BBReplayManager` — Ring buffer replay with scrub/speed control
- `BBConfessionalRoom` — Spotlight, camera switch, subtitle display
- `BBEventVisualizer` — Per-event-type visuals (alliance=handshake+green particles, conflict=argument+red+screen shake, betrayal=purple+slow-mo)
- `BBHUD` — Canvas HUD with mini-map, agent info, event popups
- `BBPlayerController` — Enhanced Input for all controls (1/2/3=camera, Space=play/pause, Tab=cycle, R=replay, C=confessional)

### Conditioning Pipeline (`/pipeline/`)
Python FastAPI backend + React dashboard for conditioning 12 LLM agents with RAG-sourced Big Brother knowledge.

**Research documents** (`/pipeline/research/`):
- `social_dynamics.md` — Alliance patterns, betrayal triggers, player archetypes, winner profiles
- `psychological_decay.md` — Stress model, third-quarter phenomenon, numerical parameters
- `game_mechanics.md` — US/UK/Brazil/Israel/Canada/Australia formats, special powers, jury voting
- `agent_architecture.md` — Stanford Generative Agents, Elimination Game, Traitors, AI Town, anti-drift techniques

**Orchestrator** (`/pipeline/orchestrator/`):
- `main.py` — FastAPI app on port 8420, POST /api/start launches pipeline
- `orchestrator.py` — 4-phase pipeline brain:
  - Phase 1: Chunk research docs → ChromaDB embeddings
  - Phase 2: Generate 12 Tier 1 personas (Opus) + Tier 2/3 init (Sonnet) + distinctiveness test
  - Phase 3: 5 conditioning scenarios per agent + 3 conversation drills + 2 competitions + 5 validation benchmarks + re-conditioning loop
  - Phase 4: Export all artifacts to `/pipeline/output/`
- `config.py` — Model routing (Opus for creative, Sonnet for evaluation), pricing, thresholds
- `models.py` — Pydantic models for everything (3-tier persona, costs, health, progress)
- `api_client.py` — Anthropic SDK wrapper with prompt caching, exponential backoff, cost tracking
- `cost_tracker.py` — Per-call cost recording, budget guards ($75 limit)
- `health_monitor.py` — API status, stuck detection, rate limit tracking
- `activity_log.py` — Thread-safe event log (max 10K events)
- `api/routes.py` — GET /api/state, /api/health, /api/costs, /api/agents, /api/events, /api/progress

**RAG system** (`/pipeline/orchestrator/rag/`):
- `chunker.py` — Splits markdown into ~2000-char chunks respecting paragraph boundaries, infers metadata
- `vector_store.py` — ChromaDB wrapper with 4 collections (social_dynamics, psychological_decay, game_mechanics, agent_architecture)
- `retriever.py` — Multi-collection retrieval, scenario-specific query mapping

**Agent conditioning** (`/pipeline/orchestrator/agents/`):
- `persona_generator.py` — 12-agent cast definitions, Tier 1 generation (Opus), Tier 2/3 initialization
- `conditioner.py` — 5 conditioning scenarios, 3 conversation drills, competition rehearsals, anti-drift reinforcement
- `validator.py` — 5 benchmarks: persona consistency (>80%), behavioral distinctiveness (<0.30 similarity), strategic depth (>60%), emotional authenticity (>60%), anti-drift stability (>75%)
- `state_manager.py` — CRUD for agent personas, JSON persistence

**Dashboard** (`/pipeline/dashboard/`):
React monitoring UI with 6 panels: PipelineProgress, CostTracker, AgentGrid (12 cards), ValidationResults, HealthMonitor, ActivityFeed. Polls /api/state every 5s. Falls back to mock data standalone.

Run dashboard: `cd pipeline/dashboard && npm install && npm run dev`
Run backend: `cd pipeline && pip install -r orchestrator/requirements.txt && python -m pipeline.orchestrator.main`

### 3-Tier Persona Schema
Each of the 12 agents has:
- **Tier 1 (Immutable Core, ~500 tokens):** Name, age, backstory, Big Five scores with behavioral translations, archetype, speech patterns (catchphrases, verbal tics, humor style), behavioral rules (3 always/3 never), conflict style, core values, deepest fears, showmance susceptibility
- **Tier 2 (Mutable State, updated weekly):** Relationships (trust -10 to +10 per agent), alliances, strategic position, current strategy, learned lessons, competition record
- **Tier 3 (Volatile State, updated per scene):** 8 emotions with intensity + decay rate, energy, stress, sleep quality, current goal, information held/seeking, reveal/conceal strategy

### The 12-Agent Cast
1. Marcus Chen, 28, Criminal Defense Attorney — Puppet Master
2. Destiny Williams, 24, Bartender — Social Butterfly
3. Yakov Petrov, 35, Software Engineer — Strategic Introvert
4. Keisha Brown, 31, Nurse — Loyal Soldier
5. Jake Morrison, 22, Personal Trainer — Comp Beast
6. Sofia Reyes, 27, Influencer — Charming Villain
7. Tommy O'Brien, 42, High School Teacher — Dad Figure/Goat
8. Priya Sharma, 26, PhD Student — Analytical Floater
9. DeShawn Carter, 29, Music Producer — Charismatic Wild Card
10. Emma Lindqvist, 33, Architect — Quiet Strategist
11. Rami Hassan, 25, Stand-up Comedian — Court Jester
12. Brooklyn Taylor, 21, Dance Instructor — Underdog

### Model Routing Strategy
- **Opus** (`claude-opus-4-6`): Persona generation, conditioning scenarios, conversation drills, reflections, diary rooms, reconditioning, episode recaps
- **Sonnet** (`claude-sonnet-4-6`): Competition challenges, validation scoring, distinctiveness checks, state updates, RAG summarization, health checks

## Current State & Known Issues

### What Works
- Simulation core generates complete 7-day seasons with deterministic output
- Web control room builds and renders with demo data
- Desktop Unreal project has all C++ source (needs UE5 editor for compile)
- Pipeline orchestrator, RAG, and dashboard are structurally complete
- Acceptance tests pass for Demo A and shared contract

### What Needs Work
1. **Research docs may need expansion** — The 4 research markdown files in `/pipeline/research/` should ideally be 8,000-12,000 words each for optimal RAG chunking. Check their word counts and expand if needed.
2. **Pipeline not yet run** — The conditioning pipeline has never been executed against the Anthropic API. It needs an `ANTHROPIC_API_KEY` env var. Create a `.env` file in `/pipeline/orchestrator/`.
3. **Config uses Sonnet for all tasks** — `config.py` currently routes ALL tasks (including Opus tasks) to `claude-sonnet-4-20250514`. The Opus tasks should use `claude-opus-4-6` for production quality. This was done to keep dev costs low.
4. **Web app imports** — The web app imports demo data from `../../shared/demo-season.json`. Verify the import path works with Vite's config.
5. **Unreal project needs assets** — C++ is complete but no meshes, materials, animations, or levels exist. Needs UE5 editor work for visual content.
6. **Integration between pipeline output and simulation** — The conditioned LLM agents (pipeline output) are not yet wired into the simulation core. The simulation core still uses the rule-based system. Future work: create an LLM-backed decision engine that replaces `ai-behavior.ts` with API calls using the conditioned personas.

## Git
- Branch: `claude/rename-directory-i6TEK`
- Remote: `origin` → `fizzious1/setCardGame1.2`
- All work is committed and pushed

## Commands
```bash
# Web control room
cd web-control-room && npm install && npm run dev

# Pipeline dashboard (standalone with mock data)
cd pipeline/dashboard && npm install && npm run dev

# Pipeline backend
cd pipeline && pip install -r orchestrator/requirements.txt
ANTHROPIC_API_KEY=sk-... python -m pipeline.orchestrator.main
# Then POST http://localhost:8420/api/start to begin conditioning

# Generate new demo season data
cd simulation-core && npm install && npx ts-node src/generate-demo.ts

# Run acceptance tests
bash tests/run-acceptance.sh
```
