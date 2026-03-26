"""Agent state management.

Provides a central registry for all :class:`AgentPersona` instances with
methods to update individual tiers, track conditioning status, and
persist / restore state to disk as JSON.
"""

from __future__ import annotations

import json
import logging
from pathlib import Path
from typing import Any

from pipeline.orchestrator.models import (
    AgentPersona,
    Emotion,
    Relationship,
    Alliance,
    Tier2State,
    Tier3State,
)

logger = logging.getLogger(__name__)


class AgentStateManager:
    """In-memory registry of all agent personas with persistence support."""

    def __init__(self) -> None:
        self.agents: dict[str, AgentPersona] = {}

    # ------------------------------------------------------------------
    # Basic CRUD
    # ------------------------------------------------------------------

    def add_agent(self, agent: AgentPersona) -> None:
        """Register an agent (overwrites if id already exists)."""
        self.agents[agent.id] = agent
        logger.debug("Added agent %s (%s)", agent.id, agent.tier1.name)

    def get_agent(self, agent_id: str) -> AgentPersona:
        """Return the agent with *agent_id*, or raise ``KeyError``."""
        if agent_id not in self.agents:
            raise KeyError(f"Agent '{agent_id}' not found in state manager")
        return self.agents[agent_id]

    def get_all_agents(self) -> list[AgentPersona]:
        """Return all registered agents, sorted by id."""
        return sorted(self.agents.values(), key=lambda a: a.id)

    # ------------------------------------------------------------------
    # Tier 2 (social / strategic) updates
    # ------------------------------------------------------------------

    def update_tier2(self, agent_id: str, updates: dict[str, Any]) -> None:
        """Apply partial updates to the agent's Tier 2 state.

        Supported keys in *updates*:

        - ``relationships``: dict mapping other-agent-id to
          ``{"trust": float, "status": str, "notes": str}`` (merged).
        - ``alliances``: list of alliance dicts (replaces).
        - ``strategic_position``, ``current_strategy``,
          ``psychological_evolution``: str (replaces).
        - ``learned_lessons``: list[str] (extends).
        - ``competition_record``: dict[str, int] (merged).
        - ``times_nominated``: int (replaces).
        - ``votes_received_against``: list[str] (extends).
        """
        agent = self.get_agent(agent_id)
        tier2 = agent.tier2

        if "relationships" in updates:
            for other_id, rel_data in updates["relationships"].items():
                if isinstance(rel_data, dict):
                    if other_id in tier2.relationships:
                        existing = tier2.relationships[other_id]
                        tier2.relationships[other_id] = Relationship(
                            trust=rel_data.get("trust", existing.trust),
                            status=rel_data.get("status", existing.status),
                            notes=rel_data.get("notes", existing.notes),
                        )
                    else:
                        tier2.relationships[other_id] = Relationship(**rel_data)

        if "alliances" in updates:
            tier2.alliances = [
                Alliance(**a) if isinstance(a, dict) else a
                for a in updates["alliances"]
            ]

        if "strategic_position" in updates:
            tier2.strategic_position = updates["strategic_position"]

        if "current_strategy" in updates:
            tier2.current_strategy = updates["current_strategy"]

        if "psychological_evolution" in updates:
            tier2.psychological_evolution = updates["psychological_evolution"]

        if "learned_lessons" in updates:
            tier2.learned_lessons.extend(updates["learned_lessons"])

        if "competition_record" in updates:
            tier2.competition_record.update(updates["competition_record"])

        if "times_nominated" in updates:
            tier2.times_nominated = updates["times_nominated"]

        if "votes_received_against" in updates:
            tier2.votes_received_against.extend(updates["votes_received_against"])

        logger.debug("Updated Tier 2 for %s: keys=%s", agent_id, list(updates.keys()))

    # ------------------------------------------------------------------
    # Tier 3 (emotional / moment-to-moment) updates
    # ------------------------------------------------------------------

    def update_tier3(self, agent_id: str, updates: dict[str, Any]) -> None:
        """Apply partial updates to the agent's Tier 3 state.

        Supported keys in *updates*:

        - ``emotions``: dict mapping emotion name to
          ``{"intensity": float, "decay_rate": float}`` (merged).
        - ``energy_level``, ``stress_level``,
          ``sleep_quality_last_night``: float (replaces).
        - ``current_goal``, ``reveal_strategy``,
          ``conceal_strategy``: str (replaces).
        - ``information_held``: list[str] (extends).
        - ``information_seeking``: list[str] (extends).
        """
        agent = self.get_agent(agent_id)
        tier3 = agent.tier3

        if "emotions" in updates:
            for emotion_name, emo_data in updates["emotions"].items():
                if isinstance(emo_data, dict):
                    if emotion_name in tier3.emotions:
                        existing = tier3.emotions[emotion_name]
                        tier3.emotions[emotion_name] = Emotion(
                            intensity=emo_data.get("intensity", existing.intensity),
                            decay_rate=emo_data.get("decay_rate", existing.decay_rate),
                        )
                    else:
                        tier3.emotions[emotion_name] = Emotion(**emo_data)

        for float_key in ("energy_level", "stress_level", "sleep_quality_last_night"):
            if float_key in updates:
                setattr(tier3, float_key, float(updates[float_key]))

        for str_key in ("current_goal", "reveal_strategy", "conceal_strategy"):
            if str_key in updates:
                setattr(tier3, str_key, updates[str_key])

        if "information_held" in updates:
            tier3.information_held.extend(updates["information_held"])

        if "information_seeking" in updates:
            tier3.information_seeking.extend(updates["information_seeking"])

        logger.debug("Updated Tier 3 for %s: keys=%s", agent_id, list(updates.keys()))

    # ------------------------------------------------------------------
    # Status & score tracking
    # ------------------------------------------------------------------

    def update_conditioning_status(self, agent_id: str, status: str) -> None:
        """Set the conditioning status for *agent_id*.

        Valid values: ``not_started``, ``in_progress``, ``passed``, ``failed``.
        """
        agent = self.get_agent(agent_id)
        agent.conditioning_status = status
        logger.info("Agent %s conditioning status -> %s", agent_id, status)

    def update_validation_scores(
        self, agent_id: str, scores: dict[str, float]
    ) -> None:
        """Merge *scores* (benchmark_name -> score) into the agent's record."""
        agent = self.get_agent(agent_id)
        agent.validation_scores.update(scores)
        logger.debug(
            "Agent %s validation scores updated: %s", agent_id, scores
        )

    def increment_interactions(self, agent_id: str) -> None:
        """Increment the interaction counter for *agent_id*."""
        agent = self.get_agent(agent_id)
        agent.interactions_completed += 1

    # ------------------------------------------------------------------
    # Persistence
    # ------------------------------------------------------------------

    def save_all(self, output_dir: str) -> None:
        """Save every agent as a JSON file in *output_dir*.

        Files are named ``<agent_id>.json``.
        """
        out = Path(output_dir)
        out.mkdir(parents=True, exist_ok=True)

        for agent_id, agent in self.agents.items():
            filepath = out / f"{agent_id}.json"
            data = agent.model_dump(mode="json")
            filepath.write_text(json.dumps(data, indent=2, ensure_ascii=False), encoding="utf-8")
            logger.debug("Saved %s to %s", agent_id, filepath)

        logger.info("Saved %d agents to %s", len(self.agents), output_dir)

    def load_all(self, output_dir: str) -> None:
        """Load agent JSON files from *output_dir* into the registry.

        Existing agents with the same id are overwritten.
        """
        out = Path(output_dir)
        if not out.is_dir():
            logger.warning("Directory %s does not exist; nothing to load.", output_dir)
            return

        loaded = 0
        for filepath in sorted(out.glob("agent_*.json")):
            try:
                data = json.loads(filepath.read_text(encoding="utf-8"))
                agent = AgentPersona(**data)
                self.agents[agent.id] = agent
                loaded += 1
            except Exception as exc:
                logger.error("Failed to load %s: %s", filepath, exc)

        logger.info("Loaded %d agents from %s", loaded, output_dir)
