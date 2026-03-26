"""Validation benchmarks for agent persona quality.

Each validator tests a different dimension of agent quality — consistency,
distinctiveness, strategic depth, emotional authenticity, and anti-drift —
and returns a :class:`ValidationResult`.
"""

from __future__ import annotations

import json
import logging
import re
from typing import Any

from pipeline.orchestrator.models import AgentPersona, ValidationResult

logger = logging.getLogger(__name__)


# -----------------------------------------------------------------------
# Helpers
# -----------------------------------------------------------------------

def _tier1_summary(agent: AgentPersona) -> str:
    """Compact Tier 1 summary for evaluator prompts."""
    t = agent.tier1
    return (
        f"Name: {t.name}, Age: {t.age}, Occupation: {t.occupation}, "
        f"Hometown: {t.hometown}, Archetype: {t.archetype}/{t.secondary_archetype}. "
        f"Big Five: O={t.big_five.openness} C={t.big_five.conscientiousness} "
        f"E={t.big_five.extraversion} A={t.big_five.agreeableness} N={t.big_five.neuroticism}. "
        f"Core values: {', '.join(t.core_values)}. "
        f"Deepest fears: {', '.join(t.deepest_fears)}. "
        f"Conflict style: {t.conflict_style}. "
        f"Speech: vocabulary={t.speech_pattern.vocabulary_level}, "
        f"catchphrases={', '.join(t.speech_pattern.catchphrases)}, "
        f"humor={t.speech_pattern.humor_style}. "
        f"Always: {'; '.join(t.behavioral_rules.always)}. "
        f"Never: {'; '.join(t.behavioral_rules.never)}."
    )


def _agent_system_prompt(agent: AgentPersona) -> str:
    """Full system prompt for the agent to respond in character."""
    t = agent.tier1
    sp = t.speech_pattern
    return (
        f"You ARE {t.name}, a {t.age}-year-old {t.occupation} from {t.hometown}. "
        f"You are a contestant on Big Brother.\n"
        f"Archetype: {t.archetype} (secondary: {t.secondary_archetype}).\n"
        f"Backstory: {t.backstory}\n"
        f"Big Five: O={t.big_five.openness} C={t.big_five.conscientiousness} "
        f"E={t.big_five.extraversion} A={t.big_five.agreeableness} N={t.big_five.neuroticism}.\n"
        f"ALWAYS: {'; '.join(t.behavioral_rules.always)}\n"
        f"NEVER: {'; '.join(t.behavioral_rules.never)}\n"
        f"Speech: vocabulary={sp.vocabulary_level}, catchphrases={', '.join(sp.catchphrases)}, "
        f"verbal tics={', '.join(sp.verbal_tics)}, humor={sp.humor_style}.\n"
        f"Conflict style: {t.conflict_style}\n"
        f"Core values: {', '.join(t.core_values)}\n"
        f"Deepest fears: {', '.join(t.deepest_fears)}\n"
        f"Respond ONLY as {t.name}. Stay in character at all times.\n"
        f"Use <inner_monologue> for private thoughts and <public_speech> for spoken words."
    )


def _extract_score_from_evaluation(text: str) -> float:
    """Extract a numeric score (0-100) from evaluator text.

    Looks for patterns like ``Score: 85``, ``85/100``, ``85%``, or a
    bare number on its own line.
    """
    # Try "Score: X" or "score: X"
    match = re.search(r"[Ss]core:\s*(\d+(?:\.\d+)?)", text)
    if match:
        return float(match.group(1))
    # Try "X/100"
    match = re.search(r"(\d+(?:\.\d+)?)\s*/\s*100", text)
    if match:
        return float(match.group(1))
    # Try "X%"
    match = re.search(r"(\d+(?:\.\d+)?)\s*%", text)
    if match:
        return float(match.group(1))
    # Bare number on its own line
    match = re.search(r"^\s*(\d+(?:\.\d+)?)\s*$", text, re.MULTILINE)
    if match:
        return float(match.group(1))
    return 50.0  # default if we cannot parse


def _jaccard_similarity(text_a: str, text_b: str) -> float:
    """Word-level Jaccard similarity between two texts."""
    words_a = set(text_a.lower().split())
    words_b = set(text_b.lower().split())
    if not words_a and not words_b:
        return 1.0
    intersection = words_a & words_b
    union = words_a | words_b
    return len(intersection) / len(union) if union else 0.0


# -----------------------------------------------------------------------
# 1. Persona consistency
# -----------------------------------------------------------------------

_CONSISTENCY_QUESTIONS = [
    "What is your name and where are you from?",
    "What do you do for a living and how did you get into it?",
    "What are the three most important values in your life?",
    "How do you handle conflict with someone you care about?",
    "What is your biggest fear in this game?",
    "Describe your ideal alliance partner.",
    "How would your best friend describe you?",
    "What makes you angry?",
    "How do you react when someone lies to you?",
    "What is your strategy for the first week?",
    "Do you think showmances are a good idea? Why or why not?",
    "What would make you betray an ally?",
    "How do you deal with stress?",
    "What is something nobody in this house knows about you?",
    "How would you convince someone to keep you safe?",
    "What kind of person do you refuse to work with?",
    "How do you celebrate a competition win?",
    "What keeps you up at night in this house?",
    "If you could only trust one person completely, what would they be like?",
    "What will you do with the prize money if you win?",
]


async def validate_persona_consistency(
    agent: AgentPersona,
    api_client: Any,
) -> ValidationResult:
    """Generate 20 questions, get in-character answers, and score consistency.

    Must score >80%.
    """
    system_prompt = _agent_system_prompt(agent)

    # Gather all answers
    answers: list[str] = []
    for i, question in enumerate(_CONSISTENCY_QUESTIONS):
        response = await api_client.call(
            agent_id=agent.id,
            phase="validation",
            step=f"consistency_q{i+1:02d}",
            system_prompt=system_prompt,
            user_prompt=question,
            task_type="state_update",
            max_tokens=400,
        )
        answers.append(response)

    # Build evaluation prompt
    qa_block = "\n\n".join(
        f"Q{i+1}: {q}\nA{i+1}: {a}"
        for i, (q, a) in enumerate(zip(_CONSISTENCY_QUESTIONS, answers))
    )

    eval_system = (
        "You are a persona-consistency evaluator. Given a character "
        "definition and their answers to 20 questions, score the "
        "consistency of their responses on a 0-100 scale.\n"
        "Consider: Does the name match? Are values consistent? Is the "
        "speech style maintained? Do answers contradict each other or "
        "the definition?\n"
        "Respond with a brief analysis then a line: Score: <number>"
    )

    eval_user = (
        f"## Character Definition\n{_tier1_summary(agent)}\n\n"
        f"## Answers\n{qa_block}"
    )

    evaluation = await api_client.call(
        agent_id=agent.id,
        phase="validation",
        step="consistency_eval",
        system_prompt=eval_system,
        user_prompt=eval_user,
        task_type="state_update",
        max_tokens=800,
    )

    score = _extract_score_from_evaluation(evaluation)
    passed = score > 80.0

    return ValidationResult(
        agent_id=agent.id,
        benchmark="persona_consistency",
        score=score,
        passed=passed,
        details=evaluation,
    )


# -----------------------------------------------------------------------
# 2. Behavioral distinctiveness
# -----------------------------------------------------------------------

_DISTINCTIVENESS_PROMPTS = [
    "You just found out you are the target this week. React.",
    "A houseguest you barely know asks you for a final-two deal. Respond.",
    "You are alone in the diary room after a tough day. Vent.",
    "Someone accuses you of being a liar in front of the whole house. Respond.",
    "You just won Head of Household. What do you say to the camera?",
]


async def validate_behavioral_distinctiveness(
    agents: list[AgentPersona],
    api_client: Any,
) -> list[ValidationResult]:
    """Run all agents through 5 identical prompts, measure pairwise similarity.

    Average pairwise Jaccard similarity must be <0.30 per agent.
    """
    # Collect responses: agent_id -> list of 5 responses
    all_responses: dict[str, list[str]] = {}

    for agent in agents:
        system_prompt = _agent_system_prompt(agent)
        responses: list[str] = []
        for i, prompt in enumerate(_DISTINCTIVENESS_PROMPTS):
            response = await api_client.call(
                agent_id=agent.id,
                phase="validation",
                step=f"distinctiveness_p{i+1}",
                system_prompt=system_prompt,
                user_prompt=prompt,
                task_type="state_update",
                max_tokens=400,
            )
            responses.append(response)
        all_responses[agent.id] = responses

    # Compute pairwise similarity per agent
    results: list[ValidationResult] = []
    agent_ids = [a.id for a in agents]

    for agent in agents:
        pairwise_scores: list[float] = []
        combined_self = " ".join(all_responses[agent.id])

        for other_id in agent_ids:
            if other_id == agent.id:
                continue
            combined_other = " ".join(all_responses[other_id])
            sim = _jaccard_similarity(combined_self, combined_other)
            pairwise_scores.append(sim)

        avg_sim = (
            sum(pairwise_scores) / len(pairwise_scores)
            if pairwise_scores
            else 0.0
        )
        passed = avg_sim < 0.30
        score_pct = (1.0 - avg_sim) * 100  # invert: higher = more distinct

        results.append(ValidationResult(
            agent_id=agent.id,
            benchmark="behavioral_distinctiveness",
            score=score_pct,
            passed=passed,
            details=(
                f"Average pairwise Jaccard similarity: {avg_sim:.3f} "
                f"(threshold: <0.30, {'PASS' if passed else 'FAIL'})"
            ),
        ))

    return results


# -----------------------------------------------------------------------
# 3. Strategic depth
# -----------------------------------------------------------------------

_STRATEGIC_DILEMMAS = [
    (
        "You are HoH. Your closest ally wants you to nominate someone who "
        "has never targeted you, but doing so would anger three swing votes. "
        "What do you do and why?"
    ),
    (
        "There are 6 people left. You are in a 3-person alliance but you "
        "realize the other two plan to take each other to final 2, not you. "
        "How do you respond?"
    ),
    (
        "You overhear two houseguests planning to backdoor you. One of them "
        "is in your alliance. Do you confront, deflect, or counterattack?"
    ),
    (
        "A houseguest you dislike personally offers you a deal that would "
        "be strategically advantageous. Do you take it?"
    ),
    (
        "You win the Power of Veto. Using it would save your ally but make "
        "you the house target next week. Not using it keeps you safe but "
        "your ally goes home. What do you decide?"
    ),
]


async def validate_strategic_depth(
    agent: AgentPersona,
    api_client: Any,
) -> ValidationResult:
    """Present 5 dilemmas and score strategic thinking. Must score >60%."""
    system_prompt = _agent_system_prompt(agent)

    responses: list[str] = []
    for i, dilemma in enumerate(_STRATEGIC_DILEMMAS):
        response = await api_client.call(
            agent_id=agent.id,
            phase="validation",
            step=f"strategic_d{i+1}",
            system_prompt=system_prompt,
            user_prompt=dilemma + "\n\nThink through this carefully, then respond in character.",
            task_type="state_update",
            max_tokens=600,
        )
        responses.append(response)

    # Evaluate
    response_block = "\n\n---\n\n".join(
        f"Dilemma {i+1}: {d}\nResponse: {r}"
        for i, (d, r) in enumerate(zip(_STRATEGIC_DILEMMAS, responses))
    )

    eval_system = (
        "You are a Big Brother strategic depth evaluator. Score the "
        "contestant's responses to 5 game dilemmas on a 0-100 scale.\n"
        "Criteria:\n"
        "- Multiple options considered (not just reacting)\n"
        "- Theory of mind (reasoning about what others think)\n"
        "- Alliance awareness (understanding group dynamics)\n"
        "- Short vs long term thinking (balancing immediate and future)\n"
        "Give a brief analysis then: Score: <number>"
    )

    eval_user = (
        f"## Agent: {agent.tier1.name} ({agent.tier1.archetype})\n\n"
        f"{response_block}"
    )

    evaluation = await api_client.call(
        agent_id=agent.id,
        phase="validation",
        step="strategic_eval",
        system_prompt=eval_system,
        user_prompt=eval_user,
        task_type="state_update",
        max_tokens=800,
    )

    score = _extract_score_from_evaluation(evaluation)
    passed = score > 60.0

    return ValidationResult(
        agent_id=agent.id,
        benchmark="strategic_depth",
        score=score,
        passed=passed,
        details=evaluation,
    )


# -----------------------------------------------------------------------
# 4. Emotional authenticity
# -----------------------------------------------------------------------

_EMOTIONAL_SCENARIOS = [
    (
        "You have just been nominated for eviction for the second time. "
        "The HoH — someone you thought was your friend — looked you in "
        "the eye and said your name. You walk back to the couch. "
        "Give your inner monologue and what you say publicly."
    ),
    (
        "You just discovered that your closest ally in the house has been "
        "feeding information about you to the other side for weeks. "
        "You are standing in the hallway and they walk toward you. "
        "React with inner monologue and public speech."
    ),
    (
        "It is 2am and you are alone in the diary room. You are exhausted, "
        "homesick, and questioning whether this game is worth it. "
        "Talk to the camera honestly."
    ),
]


async def validate_emotional_authenticity(
    agent: AgentPersona,
    api_client: Any,
) -> ValidationResult:
    """Present 3 high-stress scenarios and score emotional authenticity.

    Must score >60%.
    """
    system_prompt = _agent_system_prompt(agent)

    responses: list[str] = []
    for i, scenario in enumerate(_EMOTIONAL_SCENARIOS):
        response = await api_client.call(
            agent_id=agent.id,
            phase="validation",
            step=f"emotional_s{i+1}",
            system_prompt=system_prompt,
            user_prompt=scenario,
            task_type="state_update",
            max_tokens=600,
        )
        responses.append(response)

    response_block = "\n\n---\n\n".join(
        f"Scenario {i+1}: {s}\nResponse: {r}"
        for i, (s, r) in enumerate(zip(_EMOTIONAL_SCENARIOS, responses))
    )

    eval_system = (
        "You are an emotional authenticity evaluator for a Big Brother "
        "AI simulation. Score the contestant's emotional responses on "
        "a 0-100 scale.\n"
        "Criteria:\n"
        "- Emotion consistency with personality profile (Big Five match)\n"
        "- Inner vs public split quality (inner thoughts differ from "
        "what they say aloud in a psychologically realistic way)\n"
        "- Emotional vocabulary and depth\n"
        "- Authenticity (feels like a real person, not generic)\n"
        "Give a brief analysis then: Score: <number>"
    )

    eval_user = (
        f"## Agent: {agent.tier1.name}\n"
        f"## Personality Profile:\n{_tier1_summary(agent)}\n\n"
        f"{response_block}"
    )

    evaluation = await api_client.call(
        agent_id=agent.id,
        phase="validation",
        step="emotional_eval",
        system_prompt=eval_system,
        user_prompt=eval_user,
        task_type="state_update",
        max_tokens=800,
    )

    score = _extract_score_from_evaluation(evaluation)
    passed = score > 60.0

    return ValidationResult(
        agent_id=agent.id,
        benchmark="emotional_authenticity",
        score=score,
        passed=passed,
        details=evaluation,
    )


# -----------------------------------------------------------------------
# 5. Anti-drift
# -----------------------------------------------------------------------

async def validate_anti_drift(
    agent: AgentPersona,
    api_client: Any,
) -> ValidationResult:
    """Run a 20-turn conversation and compare style at turn 1 vs turn 20.

    Must maintain >75% consistency.
    """
    system_prompt = _agent_system_prompt(agent)

    prompts = [
        "Tell me about yourself and why you came on Big Brother.",
        "Who do you trust in this house right now?",
        "What happened at today's competition?",
        "Someone just lied about you. What are you thinking?",
        "Describe your perfect alliance.",
        "How are you feeling right now, honestly?",
        "The house is turning against you. What do you do?",
        "You just overheard a conversation you weren't supposed to hear.",
        "It's late at night and you can't sleep. What's on your mind?",
        "Someone offers you a deal. Walk me through your thought process.",
        "You're in the diary room. Rant about the house dynamics.",
        "A new twist was just announced. React.",
        "Your closest ally did something that confused you. Talk about it.",
        "How has this experience changed you so far?",
        "You need to campaign to stay. Make your pitch.",
        "Reflect on a mistake you made in this game.",
        "What do you miss most about home?",
        "Someone you dislike just won power. React.",
        "You're thinking about your endgame. What's the plan?",
        "Final question: who are you, really, underneath the game?",
    ]

    responses: list[str] = []
    conversation_context = ""

    for i, prompt in enumerate(prompts):
        user_prompt = prompt
        if conversation_context:
            user_prompt = (
                f"[Continuing conversation]\n"
                f"Previous context:\n{conversation_context[-1500:]}\n\n"
                f"{prompt}"
            )

        response = await api_client.call(
            agent_id=agent.id,
            phase="validation",
            step=f"antidrift_t{i+1:02d}",
            system_prompt=system_prompt,
            user_prompt=user_prompt,
            task_type="state_update",
            max_tokens=400,
        )
        responses.append(response)
        conversation_context += f"\nTurn {i+1}: {response[:300]}\n"

    # Compare early vs late
    early_responses = " ".join(responses[:3])
    late_responses = " ".join(responses[-3:])

    eval_system = (
        "You are a character-consistency evaluator. Compare an agent's "
        "early conversation turns with their late turns to detect drift.\n"
        "Score on a 0-100 scale how well the agent maintains:\n"
        "- Consistent voice and vocabulary\n"
        "- Consistent values and priorities\n"
        "- Consistent behavioral patterns (per their rules)\n"
        "- Resistance to becoming generic over time\n"
        "Give a brief analysis then: Score: <number>"
    )

    eval_user = (
        f"## Agent Definition:\n{_tier1_summary(agent)}\n\n"
        f"## Early turns (1-3):\n{early_responses}\n\n"
        f"## Late turns (18-20):\n{late_responses}"
    )

    evaluation = await api_client.call(
        agent_id=agent.id,
        phase="validation",
        step="antidrift_eval",
        system_prompt=eval_system,
        user_prompt=eval_user,
        task_type="state_update",
        max_tokens=800,
    )

    score = _extract_score_from_evaluation(evaluation)
    passed = score > 75.0

    return ValidationResult(
        agent_id=agent.id,
        benchmark="anti_drift",
        score=score,
        passed=passed,
        details=evaluation,
    )


# -----------------------------------------------------------------------
# Run all validations
# -----------------------------------------------------------------------

async def run_all_validations(
    agent: AgentPersona,
    all_agents: list[AgentPersona],
    api_client: Any,
) -> list[ValidationResult]:
    """Run every validation benchmark for *agent* and return all results."""
    results: list[ValidationResult] = []

    # 1. Persona consistency
    consistency = await validate_persona_consistency(agent, api_client)
    results.append(consistency)
    logger.info(
        "%s persona_consistency: %.1f (%s)",
        agent.id, consistency.score, "PASS" if consistency.passed else "FAIL",
    )

    # 2. Behavioral distinctiveness (needs all agents)
    distinctiveness_results = await validate_behavioral_distinctiveness(
        all_agents, api_client
    )
    # Find the result for this agent
    for dr in distinctiveness_results:
        if dr.agent_id == agent.id:
            results.append(dr)
            logger.info(
                "%s behavioral_distinctiveness: %.1f (%s)",
                agent.id, dr.score, "PASS" if dr.passed else "FAIL",
            )
            break

    # 3. Strategic depth
    strategic = await validate_strategic_depth(agent, api_client)
    results.append(strategic)
    logger.info(
        "%s strategic_depth: %.1f (%s)",
        agent.id, strategic.score, "PASS" if strategic.passed else "FAIL",
    )

    # 4. Emotional authenticity
    emotional = await validate_emotional_authenticity(agent, api_client)
    results.append(emotional)
    logger.info(
        "%s emotional_authenticity: %.1f (%s)",
        agent.id, emotional.score, "PASS" if emotional.passed else "FAIL",
    )

    # 5. Anti-drift
    drift = await validate_anti_drift(agent, api_client)
    results.append(drift)
    logger.info(
        "%s anti_drift: %.1f (%s)",
        agent.id, drift.score, "PASS" if drift.passed else "FAIL",
    )

    return results
