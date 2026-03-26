"""Pydantic data models for the Big Brother AI Simulator conditioning pipeline."""

from pydantic import BaseModel, Field


# ---------------------------------------------------------------------------
# Agent persona models
# ---------------------------------------------------------------------------

class PersonalityBigFive(BaseModel):
    openness: int = Field(ge=1, le=10)
    conscientiousness: int = Field(ge=1, le=10)
    extraversion: int = Field(ge=1, le=10)
    agreeableness: int = Field(ge=1, le=10)
    neuroticism: int = Field(ge=1, le=10)
    behavioral_descriptions: dict[str, str] = Field(
        default_factory=dict,
        description="trait name -> behavioral description",
    )


class SpeechPattern(BaseModel):
    vocabulary_level: str = "moderate"
    catchphrases: list[str] = Field(default_factory=list)
    verbal_tics: list[str] = Field(default_factory=list)
    humor_style: str = "dry"


class BehavioralRules(BaseModel):
    always: list[str] = Field(default_factory=list, description="3 things agent always does")
    never: list[str] = Field(default_factory=list, description="3 things agent never does")


class Tier1Persona(BaseModel):
    name: str
    age: int
    hometown: str
    occupation: str
    backstory: str = ""
    big_five: PersonalityBigFive = Field(default_factory=lambda: PersonalityBigFive(
        openness=5, conscientiousness=5, extraversion=5, agreeableness=5, neuroticism=5
    ))
    archetype: str = ""
    secondary_archetype: str = ""
    behavioral_rules: BehavioralRules = Field(default_factory=BehavioralRules)
    speech_pattern: SpeechPattern = Field(default_factory=SpeechPattern)
    conflict_style: str = ""
    core_values: list[str] = Field(default_factory=list)
    deepest_fears: list[str] = Field(default_factory=list)
    showmance_susceptibility: float = Field(default=0.5, ge=0.0, le=1.0)


class Relationship(BaseModel):
    trust: float = Field(default=0.0, ge=-10.0, le=10.0)
    status: str = "neutral"
    notes: str = ""


class Alliance(BaseModel):
    name: str
    members: list[str] = Field(default_factory=list)
    strength: float = Field(default=0.5, ge=0.0, le=1.0)
    secret: bool = False


class Tier2State(BaseModel):
    relationships: dict[str, Relationship] = Field(default_factory=dict)
    alliances: list[Alliance] = Field(default_factory=list)
    strategic_position: str = "unknown"
    current_strategy: str = "observe"
    learned_lessons: list[str] = Field(default_factory=list)
    psychological_evolution: str = ""
    competition_record: dict[str, int] = Field(default_factory=dict)
    times_nominated: int = 0
    votes_received_against: list[str] = Field(default_factory=list)


class Emotion(BaseModel):
    intensity: float = Field(default=0.0, ge=0.0, le=10.0)
    decay_rate: float = Field(default=0.1, ge=0.0, le=1.0)


class Tier3State(BaseModel):
    emotions: dict[str, Emotion] = Field(default_factory=dict)
    energy_level: float = Field(default=7.0, ge=0.0, le=10.0)
    stress_level: float = Field(default=3.0, ge=0.0, le=10.0)
    sleep_quality_last_night: float = Field(default=7.0, ge=0.0, le=10.0)
    current_goal: str = "survive the week"
    information_held: list[str] = Field(default_factory=list)
    information_seeking: list[str] = Field(default_factory=list)
    reveal_strategy: str = "selective"
    conceal_strategy: str = "deflect"


class AgentPersona(BaseModel):
    id: str
    tier1: Tier1Persona
    tier2: Tier2State = Field(default_factory=Tier2State)
    tier3: Tier3State = Field(default_factory=Tier3State)
    conditioning_status: str = "not_started"  # not_started, in_progress, passed, failed
    validation_scores: dict[str, float] = Field(default_factory=dict)
    interactions_completed: int = 0
    token_count: int = 0


# ---------------------------------------------------------------------------
# Cost tracking models
# ---------------------------------------------------------------------------

class APICallRecord(BaseModel):
    timestamp: str
    agent_id: str | None = None
    phase: str
    step: str
    model: str
    task_type: str
    input_tokens: int
    output_tokens: int
    cost_usd: float
    latency_ms: float
    cached_tokens: int = 0


class CostBreakdown(BaseModel):
    total_cost: float = 0.0
    opus_cost: float = 0.0
    sonnet_cost: float = 0.0
    embedding_cost: float = 0.0
    cost_by_phase: dict[str, float] = Field(default_factory=dict)
    cost_by_agent: dict[str, float] = Field(default_factory=dict)
    total_input_tokens: int = 0
    total_output_tokens: int = 0
    total_api_calls: int = 0
    opus_calls: int = 0
    sonnet_calls: int = 0
    avg_cost_per_opus_call: float = 0.0
    avg_cost_per_sonnet_call: float = 0.0
    estimated_total_cost: float = 0.0
    budget_remaining: float = 75.0
    cost_over_time: list[dict] = Field(default_factory=list)


# ---------------------------------------------------------------------------
# Pipeline state models
# ---------------------------------------------------------------------------

class PhaseProgress(BaseModel):
    current_phase: int = 0
    current_step: str = "idle"
    phase_name: str = "Not Started"
    steps_completed: int = 0
    total_steps: int = 0
    progress_percent: float = 0.0
    estimated_remaining_seconds: float = 0.0
    last_action: str = ""
    last_action_timestamp: str = ""


class HealthStatus(BaseModel):
    api_status: str = "responding"  # responding, slow, error
    api_latency_ms: float = 0.0
    rate_limit_remaining: int = 1000
    rate_limit_total: int = 1000
    last_health_check: str = ""
    last_api_call: str = ""
    seconds_since_last_call: float = 0.0
    error_log: list[dict] = Field(default_factory=list)
    is_stuck: bool = False


class ValidationResult(BaseModel):
    agent_id: str
    benchmark: str
    score: float
    passed: bool
    details: str = ""


class ActivityEvent(BaseModel):
    timestamp: str
    phase: str
    agent_id: str | None = None
    event_type: str = "info"  # info, success, warning, error
    message: str


class PipelineState(BaseModel):
    progress: PhaseProgress = Field(default_factory=PhaseProgress)
    costs: CostBreakdown = Field(default_factory=CostBreakdown)
    agents: list[AgentPersona] = Field(default_factory=list)
    health: HealthStatus = Field(default_factory=HealthStatus)
    validation_results: list[ValidationResult] = Field(default_factory=list)
    events: list[ActivityEvent] = Field(default_factory=list)


# ---------------------------------------------------------------------------
# RAG models
# ---------------------------------------------------------------------------

class ChunkMetadata(BaseModel):
    category: str = "general"
    subtopic: str = ""
    applicable_game_phase: str = "all"
    applicable_archetype: str = "all"


class DocumentChunk(BaseModel):
    id: str
    text: str
    metadata: ChunkMetadata = Field(default_factory=ChunkMetadata)
    document_source: str = ""
