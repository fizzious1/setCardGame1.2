"""Cost tracking for the Big Brother AI Simulator conditioning pipeline.

Records every API call, computes cost breakdowns, and enforces budget limits.
"""

import threading
from datetime import datetime, timezone

from .config import PipelineConfig
from .models import APICallRecord, CostBreakdown


class BudgetExceededError(Exception):
    """Raised when the pipeline budget has been exceeded."""

    def __init__(self, current_cost: float, budget: float):
        self.current_cost = current_cost
        self.budget = budget
        super().__init__(
            f"Budget exceeded: ${current_cost:.2f} spent of ${budget:.2f} limit"
        )


class CostTracker:
    """Thread-safe cost tracker that records every API call and enforces budget."""

    def __init__(self, config: PipelineConfig | None = None):
        self._config = config or PipelineConfig()
        self._lock = threading.Lock()
        self._records: list[APICallRecord] = []
        self._total_cost: float = 0.0
        self._opus_cost: float = 0.0
        self._sonnet_cost: float = 0.0
        self._embedding_cost: float = 0.0
        self._opus_calls: int = 0
        self._sonnet_calls: int = 0
        self._cost_by_phase: dict[str, float] = {}
        self._cost_by_agent: dict[str, float] = {}
        self._cost_over_time: list[dict] = []
        self._budget_warning_issued = False

    def record(self, record: APICallRecord) -> None:
        """Record an API call and update all running totals.

        Raises BudgetExceededError if the budget limit has been breached.
        """
        with self._lock:
            self._records.append(record)
            self._total_cost += record.cost_usd

            # Track opus vs sonnet
            tier = PipelineConfig.get_pricing_tier(record.task_type)
            if tier == "opus":
                self._opus_cost += record.cost_usd
                self._opus_calls += 1
            else:
                self._sonnet_cost += record.cost_usd
                self._sonnet_calls += 1

            # Track by phase
            self._cost_by_phase[record.phase] = (
                self._cost_by_phase.get(record.phase, 0.0) + record.cost_usd
            )

            # Track by agent
            agent_key = record.agent_id or "system"
            self._cost_by_agent[agent_key] = (
                self._cost_by_agent.get(agent_key, 0.0) + record.cost_usd
            )

            # Timeline entry
            self._cost_over_time.append(
                {
                    "timestamp": record.timestamp,
                    "cumulative_cost": self._total_cost,
                    "model": record.model,
                }
            )

            # Budget warning
            threshold_amount = PipelineConfig.BUDGET_LIMIT * PipelineConfig.ALERT_THRESHOLD
            if self._total_cost >= threshold_amount and not self._budget_warning_issued:
                self._budget_warning_issued = True
                # Warning is logged; callers should check check_budget()

            # Hard budget guard
            if self._total_cost > PipelineConfig.BUDGET_LIMIT:
                raise BudgetExceededError(self._total_cost, PipelineConfig.BUDGET_LIMIT)

    def get_breakdown(self) -> CostBreakdown:
        """Return a full cost breakdown snapshot."""
        with self._lock:
            total_input = sum(r.input_tokens for r in self._records)
            total_output = sum(r.output_tokens for r in self._records)
            total_calls = len(self._records)

            avg_opus = (
                self._opus_cost / self._opus_calls if self._opus_calls > 0 else 0.0
            )
            avg_sonnet = (
                self._sonnet_cost / self._sonnet_calls
                if self._sonnet_calls > 0
                else 0.0
            )

            return CostBreakdown(
                total_cost=round(self._total_cost, 6),
                opus_cost=round(self._opus_cost, 6),
                sonnet_cost=round(self._sonnet_cost, 6),
                embedding_cost=round(self._embedding_cost, 6),
                cost_by_phase={k: round(v, 6) for k, v in self._cost_by_phase.items()},
                cost_by_agent={k: round(v, 6) for k, v in self._cost_by_agent.items()},
                total_input_tokens=total_input,
                total_output_tokens=total_output,
                total_api_calls=total_calls,
                opus_calls=self._opus_calls,
                sonnet_calls=self._sonnet_calls,
                avg_cost_per_opus_call=round(avg_opus, 6),
                avg_cost_per_sonnet_call=round(avg_sonnet, 6),
                estimated_total_cost=round(self._total_cost, 6),
                budget_remaining=round(
                    PipelineConfig.BUDGET_LIMIT - self._total_cost, 6
                ),
                cost_over_time=list(self._cost_over_time),
            )

    def get_cost_over_time(self) -> list[dict]:
        """Return the cumulative cost timeline."""
        with self._lock:
            return list(self._cost_over_time)

    def check_budget(self) -> bool:
        """Return True if we are still within budget.

        Also returns False (but does not raise) when at the warning threshold.
        """
        with self._lock:
            return self._total_cost <= PipelineConfig.BUDGET_LIMIT

    def is_warning(self) -> bool:
        """Return True if we have crossed the alert threshold."""
        with self._lock:
            return self._total_cost >= (
                PipelineConfig.BUDGET_LIMIT * PipelineConfig.ALERT_THRESHOLD
            )

    def get_total_cost(self) -> float:
        with self._lock:
            return self._total_cost

    def add_embedding_cost(self, cost: float) -> None:
        """Record embedding costs (ChromaDB default embeddings are free, but track anyway)."""
        with self._lock:
            self._embedding_cost += cost
            self._total_cost += cost
