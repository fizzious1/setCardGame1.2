"""Agent persona generation for the Big Brother AI Simulator.

Generates the 12-agent cast, their Tier 1 personas (via Opus), and
initialises Tier 2 (social) and Tier 3 (emotional) states.
"""

from __future__ import annotations

import json
import logging
from typing import Any

from pipeline.orchestrator.models import (
    AgentPersona,
    BehavioralRules,
    Emotion,
    PersonalityBigFive,
    Relationship,
    SpeechPattern,
    Tier1Persona,
    Tier2State,
    Tier3State,
)

logger = logging.getLogger(__name__)

# -----------------------------------------------------------------------
# Full cast definitions
# -----------------------------------------------------------------------

CAST_DEFINITIONS: list[dict[str, Any]] = [
    {
        "id": "agent_01",
        "name": "Marcus Chen",
        "age": 28,
        "hometown": "Los Angeles",
        "occupation": "Criminal Defense Attorney",
        "archetype": "puppet_master",
        "secondary_archetype": "analytical_player",
        "designed_tensions": ["vs Priya (both too smart)"],
    },
    {
        "id": "agent_02",
        "name": "Destiny Williams",
        "age": 24,
        "hometown": "Atlanta",
        "occupation": "Bartender",
        "archetype": "social_butterfly",
        "secondary_archetype": "wild_card",
        "designed_tensions": ["vs Sofia (authenticity clash)"],
    },
    {
        "id": "agent_03",
        "name": "Yakov Petrov",
        "age": 35,
        "hometown": "Chicago",
        "occupation": "Software Engineer",
        "archetype": "analytical_player",
        "secondary_archetype": "loyal_soldier",
        "designed_tensions": ["could align with Marcus or oppose"],
    },
    {
        "id": "agent_04",
        "name": "Keisha Brown",
        "age": 31,
        "hometown": "Houston",
        "occupation": "Nurse",
        "archetype": "loyal_soldier",
        "secondary_archetype": "underdog",
        "designed_tensions": ["vs Sofia (values clash)"],
    },
    {
        "id": "agent_05",
        "name": "Jake Morrison",
        "age": 22,
        "hometown": "Miami",
        "occupation": "Personal Trainer",
        "archetype": "comp_beast",
        "secondary_archetype": "charming_villain",
        "designed_tensions": ["showmance bait with Brooklyn"],
    },
    {
        "id": "agent_06",
        "name": "Sofia Reyes",
        "age": 27,
        "hometown": "New York",
        "occupation": "Influencer",
        "archetype": "charming_villain",
        "secondary_archetype": "social_butterfly",
        "designed_tensions": ["vs Keisha", "vs Destiny"],
    },
    {
        "id": "agent_07",
        "name": "Tommy O'Brien",
        "age": 42,
        "hometown": "Boston",
        "occupation": "High School Teacher",
        "archetype": "goat",
        "secondary_archetype": "loyal_soldier",
        "designed_tensions": ["dark horse or early target"],
    },
    {
        "id": "agent_08",
        "name": "Priya Sharma",
        "age": 26,
        "hometown": "San Francisco",
        "occupation": "PhD Student",
        "archetype": "floater",
        "secondary_archetype": "analytical_player",
        "designed_tensions": ["vs Marcus (mutual threat detection)"],
    },
    {
        "id": "agent_09",
        "name": "DeShawn Carter",
        "age": 29,
        "hometown": "Detroit",
        "occupation": "Music Producer",
        "archetype": "wild_card",
        "secondary_archetype": "social_butterfly",
        "designed_tensions": ["unpredictable alliance partner"],
    },
    {
        "id": "agent_10",
        "name": "Emma Lindqvist",
        "age": 33,
        "hometown": "Seattle",
        "occupation": "Architect",
        "archetype": "analytical_player",
        "secondary_archetype": "puppet_master",
        "designed_tensions": ["sleeper threat, late-game power"],
    },
    {
        "id": "agent_11",
        "name": "Rami Hassan",
        "age": 25,
        "hometown": "Toronto",
        "occupation": "Stand-up Comedian",
        "archetype": "social_butterfly",
        "secondary_archetype": "floater",
        "designed_tensions": ["info broker via humor"],
    },
    {
        "id": "agent_12",
        "name": "Brooklyn Taylor",
        "age": 21,
        "hometown": "Nashville",
        "occupation": "Dance Instructor",
        "archetype": "underdog",
        "secondary_archetype": "comp_beast",
        "designed_tensions": ["showmance with Jake", "house sympathy"],
    },
]


# -----------------------------------------------------------------------
# Persona generation (Tier 1) — uses Opus
# -----------------------------------------------------------------------

_PERSONA_SYSTEM_PROMPT = """\
You are the character design engine for a Big Brother AI simulation.
Your job is to produce a richly detailed, psychologically realistic
contestant profile that will be used to drive an autonomous agent.

Return your answer as a single JSON object with EXACTLY these keys:

{{
  "backstory": "<2-3 paragraph backstory>",
  "big_five": {{
    "openness": <1-10>,
    "conscientiousness": <1-10>,
    "extraversion": <1-10>,
    "agreeableness": <1-10>,
    "neuroticism": <1-10>,
    "behavioral_descriptions": {{
      "openness": "<how this manifests>",
      "conscientiousness": "<how this manifests>",
      "extraversion": "<how this manifests>",
      "agreeableness": "<how this manifests>",
      "neuroticism": "<how this manifests>"
    }}
  }},
  "behavioral_rules": {{
    "always": ["<rule1>", "<rule2>", "<rule3>"],
    "never": ["<rule1>", "<rule2>", "<rule3>"]
  }},
  "speech_pattern": {{
    "vocabulary_level": "<low|moderate|high|academic>",
    "catchphrases": ["<phrase1>", "<phrase2>"],
    "verbal_tics": ["<tic1>", "<tic2>"],
    "humor_style": "<dry|sarcastic|goofy|dark|self-deprecating|observational>"
  }},
  "conflict_style": "<description of how they handle conflict>",
  "core_values": ["<value1>", "<value2>", "<value3>"],
  "deepest_fears": ["<fear1>", "<fear2>"],
  "showmance_susceptibility": <0.0-1.0>
}}

Do NOT include any text outside the JSON object.
"""

_PERSONA_USER_PROMPT = """\
Design a full Big Brother contestant persona for:

Name: {name}
Age: {age}
Hometown: {hometown}
Occupation: {occupation}
Primary Archetype: {archetype}
Secondary Archetype: {secondary_archetype}
Designed Narrative Tensions: {tensions}

The persona must feel like a real person. Ground the backstory in their
occupation and hometown. Make the Big Five scores reflect the archetype
(e.g., a puppet_master should have high conscientiousness and moderate-
to-low agreeableness). Speech patterns should be distinctive and
consistent with background. Behavioral rules should create dramatic
potential.
"""


def _parse_persona_json(raw: str, cast_def: dict[str, Any]) -> Tier1Persona:
    """Parse the LLM JSON response into a Tier1Persona."""
    # Strip markdown code fences if present
    text = raw.strip()
    if text.startswith("```"):
        first_newline = text.index("\n")
        last_fence = text.rfind("```")
        text = text[first_newline + 1 : last_fence].strip()

    data = json.loads(text)

    big_five = PersonalityBigFive(**data["big_five"])
    behavioral_rules = BehavioralRules(**data["behavioral_rules"])
    speech_pattern = SpeechPattern(**data["speech_pattern"])

    return Tier1Persona(
        name=cast_def["name"],
        age=cast_def["age"],
        hometown=cast_def["hometown"],
        occupation=cast_def["occupation"],
        backstory=data.get("backstory", ""),
        big_five=big_five,
        archetype=cast_def["archetype"],
        secondary_archetype=cast_def["secondary_archetype"],
        behavioral_rules=behavioral_rules,
        speech_pattern=speech_pattern,
        conflict_style=data.get("conflict_style", ""),
        core_values=data.get("core_values", []),
        deepest_fears=data.get("deepest_fears", []),
        showmance_susceptibility=float(data.get("showmance_susceptibility", 0.5)),
    )


async def generate_persona(cast_def: dict[str, Any], api_client: Any) -> Tier1Persona:
    """Generate a Tier 1 persona via an Opus API call.

    Parameters
    ----------
    cast_def:
        One element from :data:`CAST_DEFINITIONS`.
    api_client:
        Object exposing ``await api_client.call(...) -> str``.
    """
    user_prompt = _PERSONA_USER_PROMPT.format(
        name=cast_def["name"],
        age=cast_def["age"],
        hometown=cast_def["hometown"],
        occupation=cast_def["occupation"],
        archetype=cast_def["archetype"],
        secondary_archetype=cast_def["secondary_archetype"],
        tensions=", ".join(cast_def["designed_tensions"]),
    )

    response = await api_client.call(
        agent_id=cast_def["id"],
        phase="persona_generation",
        step="generate_tier1",
        system_prompt=_PERSONA_SYSTEM_PROMPT,
        user_prompt=user_prompt,
        task_type="persona_generation",
        max_tokens=2000,
    )

    return _parse_persona_json(response, cast_def)


# -----------------------------------------------------------------------
# Tier 2 initialisation — uses Sonnet
# -----------------------------------------------------------------------

async def initialize_tier2(
    agent_id: str,
    all_agent_ids: list[str],
    api_client: Any,
) -> Tier2State:
    """Create a blank Tier 2 social state for *agent_id*.

    All relationships start at trust 0 / neutral. No alliances. Position
    is ``invisible`` (pre-game). The API call is a lightweight Sonnet
    request that simply acknowledges initialisation.
    """
    relationships: dict[str, Relationship] = {}
    for other_id in all_agent_ids:
        if other_id != agent_id:
            relationships[other_id] = Relationship(trust=0.0, status="neutral", notes="")

    tier2 = Tier2State(
        relationships=relationships,
        alliances=[],
        strategic_position="invisible",
        current_strategy="observe",
        learned_lessons=[],
        psychological_evolution="",
        competition_record={},
        times_nominated=0,
        votes_received_against=[],
    )

    # Lightweight confirmation call via Sonnet
    await api_client.call(
        agent_id=agent_id,
        phase="persona_generation",
        step="init_tier2",
        system_prompt="You are an initialisation helper. Confirm initialisation.",
        user_prompt=f"Tier 2 social state initialised for {agent_id} with {len(relationships)} relationships.",
        task_type="state_update",
        max_tokens=100,
    )

    return tier2


# -----------------------------------------------------------------------
# Tier 3 initialisation — uses Sonnet
# -----------------------------------------------------------------------

def _baseline_emotions(big_five: PersonalityBigFive) -> dict[str, Emotion]:
    """Derive baseline emotions from Big Five scores."""
    emotions: dict[str, Emotion] = {}

    # Anxiety: primarily driven by Neuroticism
    anxiety_intensity = min(10.0, max(0.0, big_five.neuroticism * 0.4))
    emotions["anxiety"] = Emotion(intensity=anxiety_intensity, decay_rate=0.08)

    # Excitement: driven by Extraversion and Openness
    excitement = min(10.0, max(0.0, (big_five.extraversion + big_five.openness) * 0.25))
    emotions["excitement"] = Emotion(intensity=excitement, decay_rate=0.15)

    # Confidence: inversely related to Neuroticism, boosted by Conscientiousness
    confidence = min(10.0, max(0.0, (10 - big_five.neuroticism + big_five.conscientiousness) * 0.3))
    emotions["confidence"] = Emotion(intensity=confidence, decay_rate=0.05)

    # Loneliness: inversely related to Extraversion and Agreeableness
    loneliness = min(10.0, max(0.0, (20 - big_five.extraversion - big_five.agreeableness) * 0.2))
    emotions["loneliness"] = Emotion(intensity=loneliness, decay_rate=0.1)

    # Anger: related to low Agreeableness and high Neuroticism
    anger = min(10.0, max(0.0, (10 - big_five.agreeableness + big_five.neuroticism) * 0.2))
    emotions["anger"] = Emotion(intensity=anger, decay_rate=0.12)

    # Happiness: related to Extraversion and Agreeableness
    happiness = min(10.0, max(0.0, (big_five.extraversion + big_five.agreeableness) * 0.3))
    emotions["happiness"] = Emotion(intensity=happiness, decay_rate=0.1)

    # Paranoia: driven by Neuroticism and low Agreeableness
    paranoia = min(10.0, max(0.0, (big_five.neuroticism + (10 - big_five.agreeableness)) * 0.15))
    emotions["paranoia"] = Emotion(intensity=paranoia, decay_rate=0.07)

    return emotions


async def initialize_tier3(
    tier1: Tier1Persona,
    api_client: Any,
) -> Tier3State:
    """Create initial Tier 3 emotional state derived from Big Five scores."""
    emotions = _baseline_emotions(tier1.big_five)

    # Baseline energy and stress from personality
    energy = min(10.0, max(1.0, 5.0 + tier1.big_five.extraversion * 0.3))
    stress = min(10.0, max(0.0, tier1.big_five.neuroticism * 0.35))

    tier3 = Tier3State(
        emotions=emotions,
        energy_level=energy,
        stress_level=stress,
        sleep_quality_last_night=7.0,
        current_goal="survive the week",
        information_held=[],
        information_seeking=[],
        reveal_strategy="selective",
        conceal_strategy="deflect",
    )

    # Lightweight confirmation call via Sonnet
    await api_client.call(
        agent_id="system",
        phase="persona_generation",
        step="init_tier3",
        system_prompt="You are an initialisation helper. Confirm initialisation.",
        user_prompt=(
            f"Tier 3 emotional state initialised for {tier1.name}. "
            f"Baseline emotions derived from Big Five: "
            f"anxiety={emotions['anxiety'].intensity:.1f}, "
            f"confidence={emotions['confidence'].intensity:.1f}, "
            f"paranoia={emotions['paranoia'].intensity:.1f}."
        ),
        task_type="state_update",
        max_tokens=100,
    )

    return tier3


# -----------------------------------------------------------------------
# Build complete AgentPersona
# -----------------------------------------------------------------------

def build_agent_persona(
    agent_id: str,
    tier1: Tier1Persona,
    tier2: Tier2State,
    tier3: Tier3State,
) -> AgentPersona:
    """Assemble a complete :class:`AgentPersona` from its three tiers."""
    return AgentPersona(
        id=agent_id,
        tier1=tier1,
        tier2=tier2,
        tier3=tier3,
        conditioning_status="not_started",
        validation_scores={},
        interactions_completed=0,
        token_count=0,
    )
