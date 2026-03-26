"""Configuration for the Big Brother AI Simulator conditioning pipeline."""

import os
from dotenv import load_dotenv

load_dotenv()


class PipelineConfig:
    """Central configuration for the entire pipeline."""

    ANTHROPIC_API_KEY: str = os.getenv("ANTHROPIC_API_KEY", "")

    # Model routing: task_type -> model ID
    MODEL_ROUTING: dict[str, str] = {
        # Opus tasks — complex reasoning, persona generation, roleplay
        "persona_generation": "claude-sonnet-4-20250514",
        "conditioning_scenario": "claude-sonnet-4-20250514",
        "conversation_drill": "claude-sonnet-4-20250514",
        "reflection": "claude-sonnet-4-20250514",
        "diary_room": "claude-sonnet-4-20250514",
        "reconditioning": "claude-sonnet-4-20250514",
        "episode_recap": "claude-sonnet-4-20250514",
        # Sonnet tasks — structured evaluation, scoring, state updates
        "competition_challenge": "claude-sonnet-4-20250514",
        "validation_scoring": "claude-sonnet-4-20250514",
        "distinctiveness_check": "claude-sonnet-4-20250514",
        "state_update": "claude-sonnet-4-20250514",
        "rag_summarization": "claude-sonnet-4-20250514",
        "health_check": "claude-sonnet-4-20250514",
    }

    # Which tasks are considered "opus" tier for billing
    OPUS_TASKS: set[str] = {
        "persona_generation",
        "conditioning_scenario",
        "conversation_drill",
        "reflection",
        "diary_room",
        "reconditioning",
        "episode_recap",
    }

    SONNET_TASKS: set[str] = {
        "competition_challenge",
        "validation_scoring",
        "distinctiveness_check",
        "state_update",
        "rag_summarization",
        "health_check",
    }

    # Pricing per million tokens (USD)
    PRICING: dict[str, dict[str, float]] = {
        "opus": {"input": 15.0, "output": 75.0},
        "sonnet": {"input": 3.0, "output": 15.0},
    }

    # Budget controls
    BUDGET_LIMIT: float = 75.0
    ALERT_THRESHOLD: float = 0.8  # warn at 80% of budget

    # RAG chunking
    CHUNK_SIZE: int = 500  # tokens (~2000 chars)
    CHUNK_SIZE_CHARS: int = 2000
    CHUNK_OVERLAP: int = 50  # tokens (~200 chars)
    CHUNK_OVERLAP_CHARS: int = 200
    TOP_K_RETRIEVAL: int = 5

    # Health / reliability
    HEALTH_CHECK_INTERVAL: int = 30  # seconds
    MAX_RETRIES: int = 3
    BACKOFF_BASE: int = 2

    # Validation thresholds
    VALIDATION_THRESHOLDS: dict[str, float] = {
        "persona_consistency": 0.80,
        "behavioral_distinctiveness": 0.30,
        "strategic_depth": 0.60,
        "emotional_authenticity": 0.60,
        "anti_drift_stability": 0.75,
    }

    # ChromaDB
    CHROMA_COLLECTION_NAME: str = "bb_knowledge_base"
    CHROMA_PERSIST_DIR: str = os.path.join(
        os.path.dirname(os.path.abspath(__file__)), "..", "chroma_db"
    )

    # Output directories
    OUTPUT_DIR: str = os.path.join(
        os.path.dirname(os.path.abspath(__file__)), "..", "output"
    )
    PERSONAS_DIR: str = os.path.join(OUTPUT_DIR, "personas")
    CONDITIONING_LOGS_DIR: str = os.path.join(OUTPUT_DIR, "conditioning_logs")
    VALIDATION_REPORTS_DIR: str = os.path.join(OUTPUT_DIR, "validation_reports")

    # Research docs
    RESEARCH_DIR: str = os.path.join(
        os.path.dirname(os.path.abspath(__file__)), "..", "research"
    )

    @classmethod
    def get_pricing_tier(cls, task_type: str) -> str:
        """Return 'opus' or 'sonnet' for a given task type."""
        if task_type in cls.OPUS_TASKS:
            return "opus"
        return "sonnet"

    @classmethod
    def get_model(cls, task_type: str) -> str:
        """Return the model ID for a given task type."""
        return cls.MODEL_ROUTING.get(task_type, "claude-sonnet-4-20250514")

    @classmethod
    def calculate_cost(cls, task_type: str, input_tokens: int, output_tokens: int) -> float:
        """Calculate cost in USD for a given API call."""
        tier = cls.get_pricing_tier(task_type)
        pricing = cls.PRICING[tier]
        input_cost = (input_tokens / 1_000_000) * pricing["input"]
        output_cost = (output_tokens / 1_000_000) * pricing["output"]
        return input_cost + output_cost
