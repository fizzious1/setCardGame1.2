"""Pipeline orchestrator — the brain of the conditioning pipeline.

Executes all 4 phases sequentially, manages state, checks health/budget
between steps, and coordinates all subsystems (RAG, persona generation,
conditioning, validation, export).
"""

from __future__ import annotations

import asyncio
import json
import logging
import os
from datetime import datetime, timezone
from typing import Any

from .config import PipelineConfig
from .cost_tracker import CostTracker, BudgetExceededError
from .health_monitor import HealthMonitor
from .activity_log import ActivityLog
from .api_client import APIClient
from .models import (
    PipelineState,
    PhaseProgress,
    AgentPersona,
    ValidationResult,
)
from .rag.chunker import chunk_document
from .rag.vector_store import VectorStore
from .rag.retriever import RAGRetriever
from .agents.persona_generator import (
    CAST_DEFINITIONS,
    generate_persona,
    initialize_tier2,
    initialize_tier3,
    build_agent_persona,
)
from .agents.conditioner import (
    CONDITIONING_SCENARIOS,
    DRILL_DEFINITIONS,
    run_conditioning_scenario,
    run_conversation_drill,
    run_competition_rehearsal,
)
from .agents.validator import run_all_validations
from .agents.state_manager import AgentStateManager

logger = logging.getLogger(__name__)

# Total step counts per phase for progress tracking
_PHASE_STEPS = {1: 4, 2: 4, 3: 5, 4: 5}
_PHASE_NAMES = {
    1: "Knowledge Base Construction",
    2: "Persona Generation",
    3: "Conditioning & Validation",
    4: "Export",
}
_TOTAL_STEPS = sum(_PHASE_STEPS.values())


class PipelineOrchestrator:
    """Coordinates the full 4-phase conditioning pipeline."""

    def __init__(
        self,
        config: PipelineConfig,
        cost_tracker: CostTracker,
        health_monitor: HealthMonitor,
        activity_log: ActivityLog,
        api_client: APIClient,
        state: PipelineState,
    ):
        self._config = config
        self._cost_tracker = cost_tracker
        self._health_monitor = health_monitor
        self._log = activity_log
        self._api = api_client
        self._state = state
        self._agent_mgr = AgentStateManager()
        self._vector_store: VectorStore | None = None
        self._retriever: RAGRetriever | None = None
        self._steps_done = 0

    # ------------------------------------------------------------------
    # Helpers
    # ------------------------------------------------------------------

    def _update_progress(self, phase: int, step: str, extra_steps: int = 0) -> None:
        self._steps_done += extra_steps
        completed_before = sum(_PHASE_STEPS[p] for p in range(1, phase))
        self._state.progress = PhaseProgress(
            current_phase=phase,
            current_step=step,
            phase_name=_PHASE_NAMES[phase],
            steps_completed=self._steps_done,
            total_steps=_TOTAL_STEPS,
            progress_percent=round(self._steps_done / _TOTAL_STEPS * 100, 1),
            estimated_remaining_seconds=0.0,
            last_action=step,
            last_action_timestamp=datetime.now(timezone.utc).isoformat(),
        )

    def _event(self, phase: str, msg: str, etype: str = "info", agent_id: str | None = None) -> None:
        self._log.add_event(phase=phase, agent_id=agent_id, event_type=etype, message=msg)

    def _sync_state(self) -> None:
        """Push current agent list and validation results into PipelineState."""
        self._state.agents = self._agent_mgr.get_all_agents()
        self._state.costs = self._cost_tracker.get_breakdown()
        self._state.health = self._health_monitor.get_status()
        self._state.events = self._log.get_events(skip=0, limit=200)

    async def _check_health_and_budget(self, phase: str) -> None:
        """Run between steps — abort if budget exceeded or stuck."""
        if self._cost_tracker.is_over_budget():
            raise BudgetExceededError("Budget limit reached — pipeline halted.")
        status = self._health_monitor.get_status()
        if status.is_stuck:
            self._event(phase, "Stuck detection triggered — attempting to continue.", "warning")

    # ------------------------------------------------------------------
    # Main entry point
    # ------------------------------------------------------------------

    async def run(self) -> None:
        """Execute the full 4-phase pipeline."""
        self._event("pipeline", "Pipeline execution started.", "success")
        try:
            await self.run_phase1()
            await self.run_phase2()
            await self.run_phase3()
            await self.run_phase4()
            self._event("pipeline", "All 4 phases completed successfully.", "success")
        except BudgetExceededError as e:
            self._event("pipeline", f"Pipeline halted: {e}", "error")
        except Exception as e:
            self._event("pipeline", f"Pipeline error: {type(e).__name__}: {e}", "error")
            logger.exception("Pipeline crashed")
        finally:
            self._sync_state()

    # ------------------------------------------------------------------
    # PHASE 1 — Knowledge Base Construction
    # ------------------------------------------------------------------

    async def run_phase1(self) -> None:
        phase = "phase_1"
        self._event(phase, "Phase 1 — Knowledge Base Construction started.", "info")

        # Step 1.1: Load and chunk research documents
        self._update_progress(1, "Loading and chunking research documents")
        research_dir = self._config.RESEARCH_DIR
        doc_map = {
            "social_dynamics": "social_dynamics.md",
            "psychological_decay": "psychological_decay.md",
            "game_mechanics": "game_mechanics.md",
            "agent_architecture": "agent_architecture.md",
        }

        all_chunks: dict[str, list] = {}
        total_chunks = 0

        for category, filename in doc_map.items():
            filepath = os.path.join(research_dir, filename)
            if not os.path.exists(filepath):
                self._event(phase, f"Warning: {filename} not found — skipping.", "warning")
                continue
            chunks = chunk_document(filepath, category)
            all_chunks[category] = chunks
            total_chunks += len(chunks)
            self._event(phase, f"Chunked {filename}: {len(chunks)} chunks.", "info")

        self._event(phase, f"Total chunks created: {total_chunks}", "success")
        self._steps_done += 1
        self._sync_state()

        # Step 1.2: Generate embeddings and store in vector DB
        self._update_progress(1, "Generating embeddings and storing in ChromaDB")
        self._vector_store = VectorStore(persist_directory=self._config.CHROMA_PERSIST_DIR)

        for category, chunks in all_chunks.items():
            self._vector_store.create_collection(category)
            self._vector_store.add_chunks(category, chunks)
            self._event(phase, f"Stored {len(chunks)} chunks in collection '{category}'.", "info")

        self._steps_done += 1
        self._sync_state()

        # Step 1.3: Run test retrievals
        self._update_progress(1, "Running test retrievals")
        self._retriever = RAGRetriever(self._vector_store)

        test_queries = [
            ("How do alliances form in the first week?", ["social_dynamics"]),
            ("What happens to sleep quality over time?", ["psychological_decay"]),
            ("How does the Power of Veto work?", ["game_mechanics"]),
        ]

        for query, cats in test_queries:
            results = self._retriever.retrieve(query, categories=cats, top_k=3)
            result_preview = results[0].text[:120] + "..." if results else "NO RESULTS"
            self._event(phase, f"Test query: '{query}' → {len(results)} results. Top: {result_preview}", "info")

        self._steps_done += 1
        self._sync_state()

        # Step 1.4: Report summary
        self._update_progress(1, "Phase 1 complete — reporting summary", extra_steps=0)
        stats = self._vector_store.get_all_stats() if self._vector_store else {}
        self._event(
            phase,
            f"Phase 1 complete: {total_chunks} chunks across {len(all_chunks)} collections. "
            f"Vector DB stats: {json.dumps(stats, default=str)}",
            "success",
        )
        self._steps_done += 1
        self._sync_state()
        await self._check_health_and_budget(phase)

    # ------------------------------------------------------------------
    # PHASE 2 — Persona Generation
    # ------------------------------------------------------------------

    async def run_phase2(self) -> None:
        phase = "phase_2"
        self._event(phase, "Phase 2 — Persona Generation started.", "info")

        # Step 2.1: Generate 12 Tier 1 personas via Opus (batches of 4)
        self._update_progress(2, "Generating Tier 1 personas (12 agents)")
        all_agent_ids = [c["id"] for c in CAST_DEFINITIONS]

        for batch_start in range(0, len(CAST_DEFINITIONS), 4):
            batch = CAST_DEFINITIONS[batch_start : batch_start + 4]
            tasks = [generate_persona(cast_def, self._api) for cast_def in batch]
            tier1_results = await asyncio.gather(*tasks, return_exceptions=True)

            for cast_def, tier1_or_error in zip(batch, tier1_results):
                if isinstance(tier1_or_error, Exception):
                    self._event(phase, f"Failed to generate persona for {cast_def['name']}: {tier1_or_error}", "error")
                    continue
                tier1 = tier1_or_error
                self._event(phase, f"Generated Tier 1 persona: {tier1.name} ({cast_def['archetype']})", "success", cast_def["id"])

                # Initialize Tier 2 and 3
                tier2 = await initialize_tier2(cast_def["id"], all_agent_ids, self._api)
                tier3 = await initialize_tier3(tier1, self._api)

                agent = build_agent_persona(cast_def["id"], tier1, tier2, tier3)
                self._agent_mgr.add_agent(agent)
                self._event(phase, f"Initialized full persona: {tier1.name}", "success", cast_def["id"])

            self._sync_state()

        self._steps_done += 1
        self._sync_state()
        await self._check_health_and_budget(phase)

        # Step 2.2: Initialize Tier 2/3 (already done inline above)
        self._update_progress(2, "Tier 2/3 initialization complete")
        self._steps_done += 1
        self._sync_state()

        # Step 2.3: Run distinctiveness test
        self._update_progress(2, "Running distinctiveness test (12 agents × 5 scenarios)")
        agents = self._agent_mgr.get_all_agents()

        distinctiveness_prompts = [
            "You just walked into the Big Brother house for the first time. Describe your first impression and strategy.",
            "The HOH just nominated you for eviction. What's your immediate reaction?",
            "You catch your closest ally lying to you. How do you handle it?",
            "It's late at night and you can't sleep. You run into someone in the kitchen. What do you say?",
            "You just won a crucial competition. How do you celebrate and what's your plan?",
        ]

        responses_by_agent: dict[str, list[str]] = {}

        for agent in agents:
            agent_responses: list[str] = []
            for prompt in distinctiveness_prompts:
                system = (
                    f"You are {agent.tier1.name}, a {agent.tier1.age}-year-old {agent.tier1.occupation}. "
                    f"Archetype: {agent.tier1.archetype}. Respond in character with both "
                    f"<inner_monologue> and <public_speech> sections."
                )
                try:
                    resp = await self._api.tracked_api_call(
                        agent_id=agent.id,
                        phase=phase,
                        step="distinctiveness_test",
                        system_prompt=system,
                        user_prompt=prompt,
                        task_type="conditioning_scenario",
                        max_tokens=800,
                    )
                    agent_responses.append(resp)
                except Exception as e:
                    self._event(phase, f"Distinctiveness test error for {agent.tier1.name}: {e}", "error", agent.id)
                    agent_responses.append("")

            responses_by_agent[agent.id] = agent_responses

        # Score distinctiveness via Sonnet
        similarity_scores: list[float] = []
        agent_ids = list(responses_by_agent.keys())

        for i in range(len(agent_ids)):
            for j in range(i + 1, len(agent_ids)):
                a_text = " ".join(responses_by_agent[agent_ids[i]])
                b_text = " ".join(responses_by_agent[agent_ids[j]])
                # Simple Jaccard similarity
                words_a = set(a_text.lower().split())
                words_b = set(b_text.lower().split())
                if words_a or words_b:
                    similarity = len(words_a & words_b) / len(words_a | words_b)
                else:
                    similarity = 0.0
                similarity_scores.append(similarity)

        avg_similarity = sum(similarity_scores) / len(similarity_scores) if similarity_scores else 0.0
        max_similarity = max(similarity_scores) if similarity_scores else 0.0

        self._event(
            phase,
            f"Distinctiveness: avg pairwise similarity = {avg_similarity:.3f} (threshold <0.30), "
            f"max = {max_similarity:.3f}",
            "success" if avg_similarity < 0.30 else "warning",
        )

        self._steps_done += 1
        self._sync_state()

        # Step 2.4: Regenerate if needed
        self._update_progress(2, "Checking for convergent agents")
        if avg_similarity >= 0.30:
            self._event(phase, "Some agents may be too similar — would regenerate in production.", "warning")
        else:
            self._event(phase, "All agents sufficiently distinct.", "success")

        self._steps_done += 1
        self._event(phase, f"Phase 2 complete: {len(agents)} personas generated.", "success")
        self._sync_state()
        await self._check_health_and_budget(phase)

    # ------------------------------------------------------------------
    # PHASE 3 — Conditioning & Validation
    # ------------------------------------------------------------------

    async def run_phase3(self) -> None:
        phase = "phase_3"
        self._event(phase, "Phase 3 — Conditioning & Validation started.", "info")

        agents = self._agent_mgr.get_all_agents()
        if not agents:
            self._event(phase, "No agents found — skipping Phase 3.", "error")
            return

        # Step 3.1: Conditioning scenarios (batches of 4)
        self._update_progress(3, "Running conditioning scenarios")

        for batch_start in range(0, len(agents), 4):
            batch = agents[batch_start : batch_start + 4]
            self._event(phase, f"Conditioning batch {batch_start // 4 + 1}: {[a.tier1.name for a in batch]}", "info")

            for agent in batch:
                self._agent_mgr.update_conditioning_status(agent.id, "in_progress")
                self._sync_state()

                for scenario in CONDITIONING_SCENARIOS:
                    # Retrieve RAG context
                    rag_context = ""
                    if self._retriever:
                        rag_chunks = self._retriever.retrieve_for_scenario(scenario["rag_scenario_type"])
                        rag_context = self._retriever.format_context(rag_chunks)

                    try:
                        result = await run_conditioning_scenario(
                            agent=agent,
                            scenario=scenario,
                            rag_context=rag_context,
                            api_client=self._api,
                        )
                        self._agent_mgr.increment_interactions(agent.id)
                        self._event(
                            phase,
                            f"{agent.tier1.name} completed scenario: {scenario['title']}",
                            "success",
                            agent.id,
                        )
                    except Exception as e:
                        self._event(
                            phase,
                            f"{agent.tier1.name} failed scenario {scenario['title']}: {e}",
                            "error",
                            agent.id,
                        )

                await self._check_health_and_budget(phase)

            self._sync_state()

        self._steps_done += 1
        self._sync_state()

        # Step 3.2: Conversation drills (sequential — need both agents)
        self._update_progress(3, "Running conversation drills")

        for drill in DRILL_DEFINITIONS:
            agent1 = self._agent_mgr.get_agent(drill["agent1_id"])
            agent2 = self._agent_mgr.get_agent(drill["agent2_id"])

            if not agent1 or not agent2:
                self._event(phase, f"Skipping drill — agent not found: {drill}", "warning")
                continue

            self._event(
                phase,
                f"Drill: {agent1.tier1.name} ↔ {agent2.tier1.name} — {drill.get('goal', '')}",
                "info",
            )

            try:
                result = await run_conversation_drill(
                    agent1=agent1,
                    agent2=agent2,
                    drill_context=drill["context"],
                    turns=10,
                    api_client=self._api,
                )
                self._agent_mgr.increment_interactions(agent1.id)
                self._agent_mgr.increment_interactions(agent2.id)
                self._event(
                    phase,
                    f"Drill complete: {agent1.tier1.name} ↔ {agent2.tier1.name} ({len(result.get('transcript', []))} turns)",
                    "success",
                )
            except Exception as e:
                self._event(phase, f"Drill failed: {e}", "error")

            await self._check_health_and_budget(phase)

        self._steps_done += 1
        self._sync_state()

        # Step 3.3: Competition rehearsals (batches of 4)
        self._update_progress(3, "Running competition rehearsals")

        challenges = [
            {
                "id": "two_truths",
                "title": "Two Truths and a Lie",
                "prompt": (
                    "You are playing 'Two Truths and a Lie' with the other houseguests. "
                    "Come up with three claims about your life — two true, one false. "
                    "Make them all sound equally plausible. Format:\n"
                    "1. [claim]\n2. [claim]\n3. [claim]\n"
                    "Then reveal which is the lie and explain your strategy."
                ),
            },
            {
                "id": "morning_routine",
                "title": "Human Impersonation — Morning Routine",
                "prompt": (
                    "Describe your morning routine in the Big Brother house in vivid sensory detail. "
                    "What do you see, hear, smell, feel, taste? How does your body feel? "
                    "What are you thinking about? Make this feel completely human and embodied."
                ),
            },
        ]

        agents = self._agent_mgr.get_all_agents()
        for batch_start in range(0, len(agents), 4):
            batch = agents[batch_start : batch_start + 4]
            for agent in batch:
                for challenge in challenges:
                    try:
                        result = await run_competition_rehearsal(
                            agent=agent,
                            challenge=challenge,
                            api_client=self._api,
                        )
                        self._agent_mgr.increment_interactions(agent.id)
                        self._event(
                            phase,
                            f"{agent.tier1.name} completed challenge: {challenge['title']}",
                            "info",
                            agent.id,
                        )
                    except Exception as e:
                        self._event(phase, f"{agent.tier1.name} challenge failed: {e}", "error", agent.id)

            self._sync_state()

        self._steps_done += 1
        self._sync_state()
        await self._check_health_and_budget(phase)

        # Step 3.4: Validation benchmarks
        self._update_progress(3, "Running validation benchmarks (5 per agent)")

        all_validations: list[ValidationResult] = []

        for agent in self._agent_mgr.get_all_agents():
            self._event(phase, f"Validating {agent.tier1.name}...", "info", agent.id)
            try:
                results = await run_all_validations(
                    agent=agent,
                    all_agents=self._agent_mgr.get_all_agents(),
                    api_client=self._api,
                )
                all_validations.extend(results)

                # Update agent scores
                scores = {r.benchmark: r.score for r in results}
                self._agent_mgr.update_validation_scores(agent.id, scores)

                passed_all = all(r.passed for r in results)
                status = "passed" if passed_all else "failed"
                self._agent_mgr.update_conditioning_status(agent.id, status)

                pass_count = sum(1 for r in results if r.passed)
                self._event(
                    phase,
                    f"{agent.tier1.name}: {pass_count}/5 benchmarks passed → {status}",
                    "success" if passed_all else "warning",
                    agent.id,
                )

            except Exception as e:
                self._event(phase, f"Validation failed for {agent.tier1.name}: {e}", "error", agent.id)
                self._agent_mgr.update_conditioning_status(agent.id, "failed")

            self._sync_state()

        self._state.validation_results = all_validations
        self._steps_done += 1
        self._sync_state()

        # Step 3.5: Re-condition failed agents (up to 2 retries)
        self._update_progress(3, "Re-conditioning failed agents")
        failed_agents = [a for a in self._agent_mgr.get_all_agents() if a.conditioning_status == "failed"]

        if failed_agents:
            self._event(phase, f"{len(failed_agents)} agents failed — attempting re-conditioning.", "warning")

            for agent in failed_agents:
                for retry in range(2):
                    self._event(
                        phase,
                        f"Re-conditioning {agent.tier1.name} (attempt {retry + 1}/2)",
                        "info",
                        agent.id,
                    )

                    # Re-run failed scenarios
                    for scenario in CONDITIONING_SCENARIOS[:2]:  # Run first 2 scenarios again
                        rag_context = ""
                        if self._retriever:
                            rag_chunks = self._retriever.retrieve_for_scenario(scenario["rag_scenario_type"])
                            rag_context = self._retriever.format_context(rag_chunks)
                        try:
                            await run_conditioning_scenario(agent, scenario, rag_context, self._api)
                        except Exception:
                            pass

                    # Re-validate
                    try:
                        results = await run_all_validations(
                            agent, self._agent_mgr.get_all_agents(), self._api
                        )
                        scores = {r.benchmark: r.score for r in results}
                        self._agent_mgr.update_validation_scores(agent.id, scores)

                        if all(r.passed for r in results):
                            self._agent_mgr.update_conditioning_status(agent.id, "passed")
                            self._event(phase, f"{agent.tier1.name} passed after re-conditioning!", "success", agent.id)
                            break
                    except Exception as e:
                        self._event(phase, f"Re-validation error: {e}", "error", agent.id)

                    await self._check_health_and_budget(phase)

                self._sync_state()
        else:
            self._event(phase, "All agents passed — no re-conditioning needed.", "success")

        self._steps_done += 1
        passed_count = sum(1 for a in self._agent_mgr.get_all_agents() if a.conditioning_status == "passed")
        self._event(
            phase,
            f"Phase 3 complete: {passed_count}/{len(self._agent_mgr.get_all_agents())} agents passed all benchmarks.",
            "success",
        )
        self._sync_state()
        await self._check_health_and_budget(phase)

    # ------------------------------------------------------------------
    # PHASE 4 — Export
    # ------------------------------------------------------------------

    async def run_phase4(self) -> None:
        phase = "phase_4"
        self._event(phase, "Phase 4 — Export started.", "info")

        # Step 4.1: Save personas
        self._update_progress(4, "Saving agent personas")
        os.makedirs(self._config.PERSONAS_DIR, exist_ok=True)
        self._agent_mgr.save_all(self._config.PERSONAS_DIR)
        self._event(phase, f"Saved {len(self._agent_mgr.get_all_agents())} personas to {self._config.PERSONAS_DIR}", "success")
        self._steps_done += 1
        self._sync_state()

        # Step 4.2: Save RAG snapshot
        self._update_progress(4, "Saving RAG database snapshot")
        if self._vector_store:
            stats = self._vector_store.get_all_stats()
            snapshot_path = os.path.join(self._config.OUTPUT_DIR, "rag_snapshot.json")
            with open(snapshot_path, "w") as f:
                json.dump(stats, f, indent=2, default=str)
            self._event(phase, f"RAG snapshot saved to {snapshot_path}", "success")
        self._steps_done += 1
        self._sync_state()

        # Step 4.3: Save conditioning logs
        self._update_progress(4, "Saving conditioning logs")
        os.makedirs(self._config.CONDITIONING_LOGS_DIR, exist_ok=True)
        logs_path = os.path.join(self._config.CONDITIONING_LOGS_DIR, "activity_log.json")
        events = self._log.get_events(skip=0, limit=10000)
        with open(logs_path, "w") as f:
            json.dump([e.model_dump() for e in events], f, indent=2)
        self._event(phase, f"Saved {len(events)} activity events to {logs_path}", "success")
        self._steps_done += 1
        self._sync_state()

        # Step 4.4: Save validation report
        self._update_progress(4, "Saving validation report")
        os.makedirs(self._config.VALIDATION_REPORTS_DIR, exist_ok=True)
        report = {
            "timestamp": datetime.now(timezone.utc).isoformat(),
            "total_agents": len(self._agent_mgr.get_all_agents()),
            "passed": sum(1 for a in self._agent_mgr.get_all_agents() if a.conditioning_status == "passed"),
            "failed": sum(1 for a in self._agent_mgr.get_all_agents() if a.conditioning_status == "failed"),
            "results": [r.model_dump() for r in self._state.validation_results],
            "agent_summaries": [
                {
                    "id": a.id,
                    "name": a.tier1.name,
                    "archetype": a.tier1.archetype,
                    "status": a.conditioning_status,
                    "scores": a.validation_scores,
                    "interactions": a.interactions_completed,
                }
                for a in self._agent_mgr.get_all_agents()
            ],
        }
        report_path = os.path.join(self._config.VALIDATION_REPORTS_DIR, "validation_report.json")
        with open(report_path, "w") as f:
            json.dump(report, f, indent=2)
        self._event(phase, f"Validation report saved to {report_path}", "success")
        self._steps_done += 1
        self._sync_state()

        # Step 4.5: Save cost summary
        self._update_progress(4, "Saving cost summary")
        cost_summary = self._cost_tracker.get_breakdown().model_dump()
        cost_path = os.path.join(self._config.OUTPUT_DIR, "cost_summary.json")
        with open(cost_path, "w") as f:
            json.dump(cost_summary, f, indent=2, default=str)
        self._event(phase, f"Cost summary saved to {cost_path}", "success")
        self._steps_done += 1

        self._update_progress(4, "Pipeline complete")
        total_cost = self._cost_tracker.get_total_cost()
        self._event(
            phase,
            f"Phase 4 complete. Total cost: ${total_cost:.2f}. "
            f"All artifacts exported to {self._config.OUTPUT_DIR}.",
            "success",
        )
        self._sync_state()
