# Agent Architecture Research for Social Simulation

## 1. Stanford Generative Agents Architecture

The Stanford Generative Agents paper (Park et al., 2023) introduced the foundational architecture for believable LLM-driven social simulation. The architecture centers on three core mechanisms — memory, reflection, and planning — that work together to produce emergent social behavior from individual agent interactions.

### Memory Stream

The memory stream is the agent's primary data structure: a chronologically ordered list of observations recorded as natural language descriptions. Each observation is timestamped and stored as a plain text record such as "Isabella Rodriguez is cooking breakfast" or "Klaus Mueller mentioned he's working on a research paper about gentrification." The memory stream captures both direct observations (things the agent perceives) and internal reflections (higher-level insights the agent generates).

Memory retrieval uses a scoring function that combines three factors: recency (how recently the memory was created, using exponential decay), importance (a pre-assigned score for each memory), and relevance (cosine similarity between the query embedding and the memory embedding). The combined score is: score = α × recency + β × importance + γ × relevance, where the weights are tuned to balance between recent events, significant memories, and contextually relevant information.

This retrieval mechanism is critical for social simulation because it naturally produces human-like memory patterns. Recent events are easily recalled (recency bias), important life events remain accessible over time (importance weighting), and contextually relevant memories surface when triggered by related observations (relevance matching). The combination prevents the agent from being overwhelmed by its full history while ensuring significant events remain accessible.

### Importance Scoring

Each observation in the memory stream receives an importance score on a 1-10 scale, assigned by the LLM itself. The prompt asks the model to rate "On a scale of 1 to 10, where 1 is purely mundane and 10 is extremely important, rate the importance of the following observation." Mundane observations like "eating breakfast" receive scores of 1-2, while significant events like "discovering a betrayal" receive scores of 8-10.

Importance scoring serves two functions: it weights memory retrieval toward significant events, and it triggers the reflection mechanism. When the sum of importance scores for recent observations exceeds a threshold (set at 150 in the original paper), the agent generates a reflection — a higher-level insight synthesized from recent memories.

### Reflection Mechanism

Reflections are the key innovation that separates the Stanford architecture from simpler memory-based systems. When the importance threshold is exceeded, the agent is prompted to generate three high-level insights based on its most recent significant memories. For example, after observing multiple social interactions, an agent might generate the reflection: "Klaus seems to be avoiding me since our disagreement about the party planning."

Reflections are stored in the memory stream alongside direct observations, creating a hierarchical memory structure. First-order memories are direct observations, second-order memories are reflections on those observations, and third-order memories are reflections on previous reflections. This hierarchy allows agents to develop increasingly abstract understandings of their social environment, moving from "X did Y" to "X seems to be pursuing strategy Z" to "X and Y are in an alliance that threatens my position."

For Big Brother simulation, the reflection mechanism is essential for producing strategic reasoning. Without reflection, agents would respond only to immediate stimuli. With reflection, agents can recognize patterns (alliance formation, betrayal signals, power dynamics), form theories of mind (understanding other agents' likely strategies), and develop long-term strategies (planning multiple moves ahead).

### Top-Down Recursive Planning

Planning in the Stanford architecture follows a top-down recursive decomposition. At the start of each day, the agent generates a high-level plan: "Today I want to strengthen my alliance with Isabella, avoid Klaus, and work on my painting." This day-level plan is recursively decomposed into hour-level blocks ("Morning: have breakfast and talk to Isabella") and then into 5-15 minute action steps ("Go to the kitchen, start cooking, wait for Isabella to arrive").

Plans are not fixed — they are revisable when new observations arrive that score high on importance and relevance. If an agent planned to avoid Klaus but encounters Klaus in the kitchen, the agent evaluates whether to modify its plan based on the observation's importance. Low-importance encounters ("Klaus is in the kitchen eating") might not trigger a plan revision, while high-importance encounters ("Klaus is telling others that I can't be trusted") would immediately revise the plan to address this threat.

For Big Brother simulation, planning should be adapted to game-specific categories: social plan (who to talk to, who to avoid), competition plan (whether to win or throw competitions), nomination plan (who to target if in power), alliance plan (which relationships to strengthen or weaken), and information plan (what to share, what to conceal, what to investigate).

## 2. Elimination Game Design

The Elimination Game paper explored LLM agents in a social deduction game where players vote to eliminate each other. The architecture is deliberately minimalist, testing how much emergent social behavior arises from simple personality descriptions and structured interaction.

### Minimalist Personality

Each agent receives a minimal personality description: a name and 2-3 trait adjectives (e.g., "Alex: strategic, cautious, observant"). This minimalist approach was deliberate — the researchers wanted to test whether LLMs could generate complex social behavior from sparse personality specifications. The finding was affirmative: even with minimal personality descriptions, agents displayed strategic thinking, deception, alliance formation, and voting behavior that reflected their assigned traits.

The minimalist personality approach has implications for Big Brother simulation. While richer personality descriptions produce more distinctive characters, the core strategic behaviors emerge from relatively simple trait specifications. This suggests that the Big Five personality framework — five dimensions with behavioral descriptions — may provide sufficient personality specification for strategic behavior, with richer backstory and speech patterns adding distinctiveness rather than strategic depth.

### Public/Private Conversation Split

The Elimination Game introduced a critical architectural decision: splitting communication into public group conversations and private one-on-one channels. In public conversations, agents must manage their image and messaging to the entire group. In private conversations, agents can share sensitive strategic information, propose alliances, and discuss targets.

This split is directly applicable to Big Brother simulation, where houseguests constantly navigate the tension between public persona and private strategy. Agents must maintain consistency between their public statements and private conversations while selectively revealing information to different allies. The private channel also enables deception — an agent can tell different allies different things in private while maintaining a neutral public position.

### Key Findings

The most significant finding was that LLM agents successfully engaged in deception, strategic voting, alliance formation, and social manipulation without explicit instructions to do so. When given a competitive elimination structure and the ability to communicate, strategic behavior emerged naturally. This validates the feasibility of LLM-driven Big Brother simulation — the game's competitive structure should naturally elicit strategic behavior from well-conditioned agents.

## 3. The Traitors Simulation

The Traitors simulation adapted the "Mafia" social deduction game format, where a subset of players (traitors) secretly eliminate other players (faithfuls) while the group votes to identify and banish traitors. The architecture introduced several innovations relevant to Big Brother simulation.

### YAML Trait Configuration

Agent personalities are defined in structured YAML configuration files, specifying traits across multiple dimensions: social style (outgoing vs. reserved), decision-making style (impulsive vs. deliberate), trust tendency (trusting vs. suspicious), conflict approach (confrontational vs. avoidant), and emotional expression (expressive vs. guarded). This structured approach ensures consistency in personality specification and makes it easy to systematically vary traits across the agent population.

### Persistent Categorized Memory

Rather than a single memory stream, the Traitors architecture uses four separate memory stores: observations (what the agent directly witnessed), conversations (summaries of dialogues with other agents), strategic thoughts (private strategic calculations and plans), and emotional reactions (feelings generated by events and interactions). This categorization improves retrieval relevance — when an agent needs strategic information, it queries the strategic memory store; when processing an emotional situation, it queries the emotional store.

For Big Brother simulation, categorized memory prevents strategic contamination of emotional processing and vice versa. An agent should be able to access their strategic assessment of an ally ("Marcus is a threat who should be targeted in Week 6") independently of their emotional memory ("Marcus comforted me when I was feeling homesick"), allowing the tension between strategic and emotional reasoning that characterizes authentic Big Brother gameplay.

### Trust Network with 3× Negativity Bias

The Traitors architecture maintains a numerical trust network where each agent holds a trust score for every other agent, updated after each interaction. The critical design decision is the 3× negativity bias: negative events (betrayals, lies discovered, hostile actions) affect trust scores three times more than positive events of equivalent magnitude. A positive interaction might increase trust by +1, while a negative interaction of similar intensity decreases trust by -3.

This negativity bias reflects real-world social psychology research showing that negative experiences have disproportionately greater impact on relationship formation than positive experiences. In Big Brother, this means that a single betrayal can permanently damage a relationship that took weeks of positive interactions to build — a dynamic that is essential for realistic alliance and jury simulation.

## 4. AI Town Schema

AI Town (Convex) implemented a spatial social simulation where agents navigate a 2D world, interact when in proximity, and maintain persistent identities and relationships. The architecture emphasizes structured data objects rather than free-form memory.

### Identity and Plan Objects

Each agent has an identity object containing: name, personality description (paragraph-length), initial plan for the day, and relationship summaries for each other agent. Plan objects are generated at the start of each day based on the agent's identity, recent events, and current goals. Plans are structured as time-blocked schedules that the agent follows until interrupted by significant events.

### Relationship Memory Objects

AI Town maintains per-pair relationship summaries — a single text summary for each pair of agents that captures the relationship's history, current state, and emotional valence. These summaries are updated after each interaction between the pair, with old information being compressed rather than discarded. This approach manages context window limitations while preserving relationship continuity.

### Vector-Embedded Conversation Summaries

After each conversation, AI Town generates a summary and embeds it in a vector database for later retrieval. When an agent encounters another agent or needs to make a decision about someone, relevant conversation summaries are retrieved and included in the prompt context. This allows agents to recall the content and emotional tone of past conversations without storing full transcripts.

### Spatial Awareness

Agents in AI Town navigate a 2D world where physical proximity triggers interactions. This spatial component is directly relevant to Big Brother simulation, where the house's physical layout (rooms, common areas, private spaces) shapes social dynamics. Agents' decisions about where to be in the house — the kitchen during meal time, the backyard for private conversations, the diary room for confessionals — are strategic decisions that affect who they interact with and what information they access.

## 5. Big Five Personality Framework for LLMs

Research on personality simulation in LLMs has established that the Big Five personality framework (OCEAN: Openness, Conscientiousness, Extraversion, Agreeableness, Neuroticism) effectively influences LLM behavior when properly specified. However, trait effectiveness varies significantly across the five dimensions.

### Agreeableness: Strongest Impact on Cooperation and Conflict

Agreeableness has the strongest and most reliable effect on LLM agent behavior. High-agreeableness agents consistently cooperate more, avoid confrontation, and seek consensus. Low-agreeableness agents reliably generate more conflict, pursue self-interest more aggressively, and challenge others' positions. This trait is the primary driver of alliance dynamics — high-agreeableness agents form and maintain alliances more readily, while low-agreeableness agents are more likely to betray or create conflict.

### Extraversion: Strongest Impact on Conversation Behavior

Extraversion reliably affects conversation initiation frequency, response length, and social seeking behavior. High-extraversion agents generate longer responses, initiate more conversations, and express more enthusiasm in social situations. Low-extraversion agents produce shorter responses, are more selective about social interactions, and are more comfortable with silence. For Big Brother simulation, extraversion drives social game strength — extraverted agents naturally build more relationships, while introverted agents must rely on the quality of fewer relationships.

### Neuroticism: Moderate Impact on Emotional Response

Neuroticism moderately affects emotional expression, worry frequency, and stress response intensity. High-neuroticism agents generate more anxiety-related language, express more concern about threats, and react more intensely to negative events. For Big Brother simulation, neuroticism drives the psychological decay trajectory — high-neuroticism agents should deteriorate faster under the game's stressors.

### Openness and Conscientiousness: Lower Impact

Openness has moderate impact on creativity and willingness to try new strategies but is less reliably expressed than other traits. Conscientiousness has the weakest and least consistent impact on LLM behavior — agents specified as highly conscientious do not reliably produce more organized or disciplined behavior, possibly because conscientiousness is more about sustained behavioral patterns than moment-to-moment responses.

### Behavioral Descriptions Over Raw Scores

The critical recommendation from personality research is to translate numerical scores into behavioral descriptions rather than providing raw numbers. The prompt "You avoid conflict at almost any cost, often agreeing with others even when you disagree internally" produces more consistent behavior than "Agreeableness: 9/10." Behavioral descriptions give the LLM concrete behavioral patterns to emulate, while raw numbers require the model to interpret what a score means in context.

## 6. Anti-Drift Techniques

Persona drift — the gradual degradation of an agent's distinctive personality characteristics over extended interactions — is one of the most significant challenges in LLM-driven social simulation. Research measures drift at 30%+ degradation after 8-12 dialogue turns without intervention, measured by cosine similarity between early and late responses to identical prompts.

### Character Reinforcement Every 8-10 Turns

The most effective anti-drift technique is periodic character reinforcement: re-injecting key identity markers into the conversation context every 8-10 turns. The reinforcement block includes the agent's name, archetype, core values, deepest fears, and speech pattern reminders. This technique reduces drift to below 10% even over 30+ turn conversations.

### Private/Public Reasoning Split

Requiring agents to produce both inner monologue (private thoughts in character) and public speech (what they say out loud) anchors the character in two ways. The inner monologue forces the agent to think through their character's perspective before speaking, maintaining strategic and emotional consistency. The public speech forces the agent to maintain their distinctive voice and speech patterns. The gap between inner and outer expression is itself a character-defining feature — a manipulative character's inner monologue should be calculating while their public speech is warm.

### Layered Identity Architecture

The three-tier identity architecture (immutable core, mutable state, volatile emotions) prevents drift by separating what changes from what doesn't. The immutable core — name, backstory, personality traits, speech patterns — never changes regardless of game events. The mutable state — relationships, alliances, strategic position — evolves in structured ways. Volatile emotions fluctuate rapidly but within bounds defined by the personality. This layered approach prevents game events from corrupting core identity while allowing authentic character development.

### Temperature Variation by Emotional State

Varying the LLM's temperature parameter based on the character's emotional state produces more authentic behavior. Calm, strategic moments use lower temperature (0.6-0.7) for more consistent and logical responses. High-stress emotional moments use higher temperature (0.8-0.95) for more variable and unpredictable responses. This technique simulates the human tendency to become less predictable under emotional duress.

## 7. Memory Architecture Comparison

| Feature | Stanford Agents | Elimination Game | Traitors | AI Town |
|---------|----------------|-----------------|----------|---------|
| Memory Type | Single stream | Conversation history | 4 categorized stores | Structured objects |
| Retrieval | Recency × Importance × Relevance | Sliding window | Category-specific query | Vector similarity |
| Capacity Management | Importance-weighted retrieval | Truncation | Summarization per category | Summary compression |
| Drift Resistance | Moderate (reflection helps) | Low (minimal personality) | High (persistent categories) | Moderate (identity objects) |
| Reflection | Yes (threshold-triggered) | No | Implicit in strategic memory | No |
| Planning | Yes (recursive decomposition) | No | No | Yes (daily plans) |
| Trust Tracking | Implicit in memory | No | Explicit network (3× negativity) | Implicit in relationship objects |
| Spatial Awareness | Yes (2D world) | No | No | Yes (2D world) |
| Computational Cost | High (frequent LLM calls for importance, reflection) | Low (minimal prompting) | Medium (categorized storage + retrieval) | Medium (summary generation + retrieval) |
| Best For | Rich emergent behavior | Competitive games | Trust-based social games | Spatial social simulation |

## 8. Practical Implementation for Big Brother Simulation

### Recommended Architecture

The optimal Big Brother simulation architecture combines elements from all four systems:

**From Stanford Agents:** Adopt the reflection mechanism with an importance threshold of 150. This enables agents to develop strategic insights from accumulated observations. Adapt the planning system to BB-specific categories: social plan, competition plan, nomination plan, alliance plan, and information management plan.

**From Elimination Game:** Implement the public/private conversation split. All agent interactions should distinguish between group scenes (where information is shared with everyone present) and private conversations (where agents can be strategically selective). The minimalist personality finding suggests focusing conditioning effort on behavioral distinctiveness rather than increasingly elaborate backstories.

**From Traitors:** Use categorized memory stores (observations, conversations, strategic thoughts, emotional reactions) and the 3× negativity bias for trust updates. The trust network should be maintained as an explicit numerical structure rather than relying on the LLM to infer trust levels from conversation history.

**From AI Town:** Use vector-embedded conversation summaries for efficient retrieval and relationship memory objects for persistent relationship tracking. Implement spatial awareness so agents make strategic decisions about house locations.

### Character Reinforcement Protocol

Reinforce character identity every 8 turns using a standardized block: "Remember: You are [name], a [age]-year-old [occupation] from [hometown]. Your archetype is [archetype]. You always [always rules]. You never [never rules]. Your deepest fears are [fears]. You speak with [speech pattern description]."

### Inner/Public Split Enforcement

Every agent response must contain both `<inner_monologue>` (private strategic and emotional reasoning) and `<public_speech>` (what the character says out loud). Responses lacking this split should be rejected and re-prompted. The inner monologue serves as a character anchor and provides strategic depth, while the public speech maintains the character's distinctive voice.

### Trust Matrix Implementation

Maintain a numerical trust matrix updated after each interaction: alliance formation +2.0, shared information +1.0, casual conversation +0.5, disagreement -1.5, betrayal discovered -6.0 (3× the alliance formation bonus), nomination by ally -4.5, vote against -3.0. Trust scores range from -10 to +10 and decay toward neutral at a rate of 0.1 per week (relationships require maintenance).

### BB-Specific Planning Categories

Each agent should maintain five concurrent plans updated weekly: a social plan identifying target relationships to build or maintain, a competition plan deciding whether to win or throw upcoming competitions, a nomination plan identifying preferred targets if the agent gains power, an alliance plan evaluating current alliances and potential new partnerships, and an information plan cataloging what the agent knows, what they want to learn, and what they need to conceal.
