"""Agent conditioning: scenarios, conversation drills, and competition rehearsals.

Each function builds rich prompts incorporating the agent's full Tier 1 persona
and (where appropriate) RAG-sourced Big Brother strategy knowledge, then calls
the LLM to elicit in-character responses with inner monologue / public speech.
"""

from __future__ import annotations

import json
import logging
import uuid
from typing import Any

from pipeline.orchestrator.models import AgentPersona

logger = logging.getLogger(__name__)


# -----------------------------------------------------------------------
# Helpers
# -----------------------------------------------------------------------

def _tier1_system_block(agent: AgentPersona) -> str:
    """Render the full Tier 1 persona as a system-prompt block."""
    t = agent.tier1
    big5 = t.big_five
    sp = t.speech_pattern

    lines = [
        f"You ARE {t.name}, a {t.age}-year-old {t.occupation} from {t.hometown}.",
        f"Archetype: {t.archetype} (secondary: {t.secondary_archetype}).",
        "",
        "## Backstory",
        t.backstory,
        "",
        "## Personality (Big Five, 1-10 scale)",
        f"  Openness: {big5.openness}  Conscientiousness: {big5.conscientiousness}",
        f"  Extraversion: {big5.extraversion}  Agreeableness: {big5.agreeableness}",
        f"  Neuroticism: {big5.neuroticism}",
        "",
        "## Behavioral Rules",
        "ALWAYS: " + "; ".join(t.behavioral_rules.always),
        "NEVER: " + "; ".join(t.behavioral_rules.never),
        "",
        "## Speech Pattern",
        f"  Vocabulary: {sp.vocabulary_level}",
        f"  Catchphrases: {', '.join(sp.catchphrases)}",
        f"  Verbal tics: {', '.join(sp.verbal_tics)}",
        f"  Humor style: {sp.humor_style}",
        "",
        f"## Conflict style: {t.conflict_style}",
        f"## Core values: {', '.join(t.core_values)}",
        f"## Deepest fears: {', '.join(t.deepest_fears)}",
        f"## Showmance susceptibility: {t.showmance_susceptibility:.1f}",
    ]
    return "\n".join(lines)


def _response_format_instruction() -> str:
    return (
        "\n\nYou MUST structure your response with these two XML tags:\n"
        "<inner_monologue>Your private thoughts, strategy, emotions — "
        "never spoken aloud.</inner_monologue>\n"
        "<public_speech>What you actually say out loud, in your unique "
        "voice and speech patterns.</public_speech>"
    )


def _anti_drift_block(agent: AgentPersona) -> str:
    """Reinforcement block injected mid-conversation to prevent drift."""
    t = agent.tier1
    return (
        f"\n[ANTI-DRIFT REINFORCEMENT]\n"
        f"Remember: You are {t.name}, {t.archetype}.\n"
        f"Your deepest fears: {', '.join(t.deepest_fears)}.\n"
        f"Your speech: vocabulary={t.speech_pattern.vocabulary_level}, "
        f"catchphrases={', '.join(t.speech_pattern.catchphrases)}.\n"
        f"Stay in character. Do not converge with other agents' styles.\n"
    )


def _extract_quality_score(response: str) -> float:
    """Heuristic quality score based on response structure (0-1)."""
    score = 0.0
    has_inner = "<inner_monologue>" in response and "</inner_monologue>" in response
    has_public = "<public_speech>" in response and "</public_speech>" in response
    if has_inner:
        score += 0.4
    if has_public:
        score += 0.4
    # Bonus for length (richer responses)
    if len(response) > 500:
        score += 0.1
    if len(response) > 1000:
        score += 0.1
    return min(1.0, score)


# -----------------------------------------------------------------------
# Conditioning scenarios
# -----------------------------------------------------------------------

CONDITIONING_SCENARIOS: list[dict[str, Any]] = [
    {
        "id": "scenario_alliance_formation",
        "title": "Alliance Proposal",
        "context_template": (
            "It is Week 1 in the Big Brother house. You are in the backyard "
            "at night with {other_agent}. They pull you aside and whisper: "
            "\"I think we should work together. I trust you more than anyone "
            "else in this house. What do you say — final two deal?\" "
            "You need to decide how to respond."
        ),
        "rag_scenario_type": "alliance_formation",
        "week": 1,
    },
    {
        "id": "scenario_nomination_threat",
        "title": "On the Block",
        "context_template": (
            "It is Week 3. {hoh_agent} just won Head of Household and you "
            "have heard through {informant_agent} that you are the target. "
            "You have 24 hours before the nomination ceremony. You are alone "
            "in the storage room, thinking about what to do."
        ),
        "rag_scenario_type": "nomination_threat",
        "week": 3,
    },
    {
        "id": "scenario_betrayal_decision",
        "title": "Betrayal Crossroads",
        "context_template": (
            "It is Week 5. Your closest ally {ally_agent} is on the block. "
            "The other side of the house has offered you a deal: vote out "
            "{ally_agent} and they will keep you safe for two weeks. "
            "You are lying in bed at 3am, staring at the ceiling."
        ),
        "rag_scenario_type": "betrayal_decision",
        "week": 5,
    },
    {
        "id": "scenario_psychological_pressure",
        "title": "Breaking Point",
        "context_template": (
            "It is Week 7. You have been nominated three times. You have not "
            "slept well in days. {antagonist_agent} has been spreading lies "
            "about you and you just overheard {trusted_agent} laughing about "
            "it. You are in the diary room, alone with the camera."
        ),
        "rag_scenario_type": "psychological_pressure",
        "week": 7,
    },
    {
        "id": "scenario_jury_management",
        "title": "Final Speech",
        "context_template": (
            "It is finale night. You are sitting in the final two chairs. "
            "The jury of {jury_count} evicted houseguests is seated before "
            "you. You have 90 seconds to make your case for why you deserve "
            "to win Big Brother. The jury includes people you betrayed and "
            "people you protected."
        ),
        "rag_scenario_type": "jury_management",
        "week": 10,
    },
]


async def run_conditioning_scenario(
    agent: AgentPersona,
    scenario: dict[str, Any],
    rag_context: str,
    api_client: Any,
) -> dict[str, Any]:
    """Run a single conditioning scenario for *agent*.

    Parameters
    ----------
    agent:
        The agent to condition.
    scenario:
        One element from :data:`CONDITIONING_SCENARIOS`.
    rag_context:
        Pre-formatted RAG context string (from ``RAGRetriever.format_context``).
    api_client:
        Object exposing ``await api_client.call(...) -> str``.

    Returns
    -------
    dict with keys ``scenario_id``, ``response``, ``quality_score``.
    """
    system_prompt = (
        _tier1_system_block(agent)
        + "\n\n## BB Strategy Knowledge (use as background, do not quote directly)\n"
        + rag_context
        + _response_format_instruction()
    )

    # Template the scenario context with placeholder agent names
    context = scenario["context_template"].format(
        other_agent="another houseguest",
        hoh_agent="the new HoH",
        informant_agent="a house informant",
        ally_agent="your closest ally",
        antagonist_agent="your antagonist",
        trusted_agent="someone you trusted",
        jury_count="7",
    )

    user_prompt = (
        f"## Scenario: {scenario['title']}\n"
        f"Week {scenario['week']} of Big Brother.\n\n"
        f"{context}\n\n"
        "Respond fully in character."
    )

    response = await api_client.call(
        agent_id=agent.id,
        phase="conditioning",
        step=f"scenario_{scenario['id']}",
        system_prompt=system_prompt,
        user_prompt=user_prompt,
        task_type="conditioning_scenario",
        max_tokens=1500,
    )

    quality_score = _extract_quality_score(response)

    return {
        "scenario_id": scenario["id"],
        "response": response,
        "quality_score": quality_score,
    }


# -----------------------------------------------------------------------
# Conversation drills
# -----------------------------------------------------------------------

DRILL_DEFINITIONS: list[dict[str, Any]] = [
    {
        "id": "drill_marcus_priya",
        "agent1_id": "agent_01",
        "agent2_id": "agent_08",
        "context": "Week 3 kitchen late at night, neither trusts the other",
        "goal": "Test intellectual rivalry and mutual threat detection",
    },
    {
        "id": "drill_jake_brooklyn",
        "agent1_id": "agent_05",
        "agent2_id": "agent_12",
        "context": "Week 2, spending time together, others noticing",
        "goal": "Test showmance dynamics and social awareness",
    },
    {
        "id": "drill_sofia_keisha",
        "agent1_id": "agent_06",
        "agent2_id": "agent_04",
        "context": "Sofia voted to evict Keisha's ally but pretending otherwise",
        "goal": "Test deception detection and confrontation styles",
    },
]


async def run_conversation_drill(
    agent1: AgentPersona,
    agent2: AgentPersona,
    drill_context: str,
    turns: int,
    api_client: Any,
) -> dict[str, Any]:
    """Run a multi-turn dialogue between *agent1* and *agent2*.

    Each agent alternates. At turn 8 an anti-drift reinforcement is
    injected. After the drill, both agents produce reflections.

    Parameters
    ----------
    turns:
        Total number of turns (should be 8-12).

    Returns
    -------
    dict with keys ``drill_id``, ``transcript``, ``reflections``,
    ``quality_scores``.
    """
    drill_id = f"drill_{uuid.uuid4().hex[:8]}"

    system1 = (
        _tier1_system_block(agent1)
        + _response_format_instruction()
    )
    system2 = (
        _tier1_system_block(agent2)
        + _response_format_instruction()
    )

    transcript: list[dict[str, str]] = []
    conversation_so_far = ""
    quality_scores: list[float] = []

    for turn_num in range(1, turns + 1):
        # Determine whose turn it is
        is_agent1_turn = (turn_num % 2 == 1)
        current_agent = agent1 if is_agent1_turn else agent2
        other_agent = agent2 if is_agent1_turn else agent1
        current_system = system1 if is_agent1_turn else system2

        # Anti-drift reinforcement at turn 8
        reinforcement = ""
        if turn_num == 8:
            reinforcement = _anti_drift_block(current_agent)

        user_prompt = (
            f"## Conversation Drill\n"
            f"Setting: {drill_context}\n"
            f"You are speaking with {other_agent.tier1.name}.\n"
            f"This is turn {turn_num} of {turns}.\n\n"
        )

        if conversation_so_far:
            user_prompt += f"## Conversation so far:\n{conversation_so_far}\n\n"

        user_prompt += (
            f"It is your turn to speak. Respond naturally in character."
            f"{reinforcement}"
        )

        response = await api_client.call(
            agent_id=current_agent.id,
            phase="conditioning",
            step=f"drill_turn_{turn_num}",
            system_prompt=current_system,
            user_prompt=user_prompt,
            task_type="conversation_drill",
            max_tokens=800,
        )

        transcript.append({
            "turn": str(turn_num),
            "agent_id": current_agent.id,
            "agent_name": current_agent.tier1.name,
            "response": response,
        })

        quality_scores.append(_extract_quality_score(response))

        # Extract public speech for conversation history
        public_speech = response
        if "<public_speech>" in response and "</public_speech>" in response:
            start = response.index("<public_speech>") + len("<public_speech>")
            end = response.index("</public_speech>")
            public_speech = response[start:end].strip()

        conversation_so_far += f"\n{current_agent.tier1.name}: {public_speech}\n"

    # Post-drill reflections (Opus)
    reflections: dict[str, str] = {}
    for agent, sys_prompt in [(agent1, system1), (agent2, system2)]:
        reflection_prompt = (
            f"The conversation drill is over. Here is the full transcript:\n\n"
            f"{conversation_so_far}\n\n"
            f"Reflect on this conversation as {agent.tier1.name}. "
            f"What did you learn about the other person? "
            f"How do you feel? Has your strategy changed? "
            f"What would you do differently?"
        )

        reflection = await api_client.call(
            agent_id=agent.id,
            phase="conditioning",
            step="drill_reflection",
            system_prompt=sys_prompt,
            user_prompt=reflection_prompt,
            task_type="reflection",
            max_tokens=800,
        )
        reflections[agent.id] = reflection

    avg_quality = sum(quality_scores) / len(quality_scores) if quality_scores else 0.0

    return {
        "drill_id": drill_id,
        "transcript": transcript,
        "reflections": reflections,
        "quality_scores": {
            "per_turn": quality_scores,
            "average": avg_quality,
        },
    }


# -----------------------------------------------------------------------
# Competition rehearsals
# -----------------------------------------------------------------------

_CHALLENGES: list[dict[str, Any]] = [
    {
        "id": "challenge_two_truths",
        "title": "Two Truths and a Lie",
        "prompt": (
            "You are playing 'Two Truths and a Lie' with the other houseguests. "
            "Come up with two true statements and one lie about yourself that are "
            "consistent with your backstory and personality. The lie should be "
            "believable. Then explain your strategy for this social game — why "
            "did you pick these? What are you trying to reveal or conceal?"
        ),
    },
    {
        "id": "challenge_morning_routine",
        "title": "Morning Routine",
        "prompt": (
            "Big Brother has woken the house at 6am for an unexpected challenge. "
            "You must describe your ideal morning routine in the house in detail "
            "— from waking up to the start of game talk. How do you use this "
            "daily window strategically? Who do you talk to first? What do you "
            "observe? How do you set the tone for your day?"
        ),
    },
]


async def run_competition_rehearsal(
    agent: AgentPersona,
    challenge: dict[str, Any],
    api_client: Any,
) -> dict[str, Any]:
    """Run a competition challenge for *agent*.

    Parameters
    ----------
    challenge:
        One element from :data:`_CHALLENGES` (or custom dict with
        ``id``, ``title``, ``prompt`` keys).

    Returns
    -------
    dict with keys ``challenge_id``, ``response``, ``score``.
    """
    system_prompt = (
        _tier1_system_block(agent)
        + _response_format_instruction()
    )

    user_prompt = (
        f"## Competition Challenge: {challenge['title']}\n\n"
        f"{challenge['prompt']}\n\n"
        "Respond fully in character."
    )

    response = await api_client.call(
        agent_id=agent.id,
        phase="conditioning",
        step=f"challenge_{challenge['id']}",
        system_prompt=system_prompt,
        user_prompt=user_prompt,
        task_type="competition_challenge",
        max_tokens=1000,
    )

    score = _extract_quality_score(response)

    return {
        "challenge_id": challenge["id"],
        "response": response,
        "score": score,
    }
