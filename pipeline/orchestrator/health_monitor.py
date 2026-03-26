"""Health monitoring for the Big Brother AI Simulator conditioning pipeline."""

import time
import asyncio
from datetime import datetime, timezone
from collections import deque

from .config import PipelineConfig
from .models import HealthStatus
from .cost_tracker import CostTracker


class HealthMonitor:
    """Monitors API health, budget status, and pipeline liveness."""

    STUCK_THRESHOLD_SECONDS = 300  # 5 minutes
    MAX_ERROR_LOG = 10

    def __init__(self, cost_tracker: CostTracker, config: PipelineConfig | None = None):
        self._config = config or PipelineConfig()
        self._cost_tracker = cost_tracker
        self._last_api_call_time: float | None = None
        self._last_health_check_time: float | None = None
        self._last_latency_ms: float = 0.0
        self._api_status: str = "responding"
        self._rate_limit_remaining: int = 1000
        self._rate_limit_total: int = 1000
        self._error_log: deque[dict] = deque(maxlen=self.MAX_ERROR_LOG)
        self._pipeline_running: bool = False

    def record_api_call(self, latency_ms: float) -> None:
        """Called after every successful API call."""
        self._last_api_call_time = time.time()
        self._last_latency_ms = latency_ms
        if latency_ms > 10000:
            self._api_status = "slow"
        else:
            self._api_status = "responding"

    def record_error(self, error_type: str, message: str, phase: str = "", agent_id: str = "") -> None:
        """Record an error in the rolling error log."""
        self._error_log.append(
            {
                "timestamp": datetime.now(timezone.utc).isoformat(),
                "error_type": error_type,
                "message": message,
                "phase": phase,
                "agent_id": agent_id,
            }
        )
        self._api_status = "error"

    def update_rate_limits(self, remaining: int, total: int) -> None:
        """Update rate limit info from API response headers."""
        self._rate_limit_remaining = remaining
        self._rate_limit_total = total

    def set_pipeline_running(self, running: bool) -> None:
        self._pipeline_running = running

    def check_api(self) -> str:
        """Return current API status string."""
        return self._api_status

    def check_budget(self) -> bool:
        """Return True if within budget."""
        return self._cost_tracker.check_budget()

    def check_stuck(self) -> bool:
        """Return True if the pipeline appears stuck (no API call in 5 min while running)."""
        if not self._pipeline_running:
            return False
        if self._last_api_call_time is None:
            return False
        elapsed = time.time() - self._last_api_call_time
        return elapsed > self.STUCK_THRESHOLD_SECONDS

    def check_rate_limit(self) -> tuple[int, int]:
        """Return (remaining, total) rate limit."""
        return self._rate_limit_remaining, self._rate_limit_total

    def get_status(self) -> HealthStatus:
        """Build and return a full HealthStatus snapshot."""
        now = time.time()
        self._last_health_check_time = now

        seconds_since = 0.0
        last_call_str = ""
        if self._last_api_call_time is not None:
            seconds_since = now - self._last_api_call_time
            last_call_str = datetime.fromtimestamp(
                self._last_api_call_time, tz=timezone.utc
            ).isoformat()

        return HealthStatus(
            api_status=self._api_status,
            api_latency_ms=self._last_latency_ms,
            rate_limit_remaining=self._rate_limit_remaining,
            rate_limit_total=self._rate_limit_total,
            last_health_check=datetime.now(timezone.utc).isoformat(),
            last_api_call=last_call_str,
            seconds_since_last_call=round(seconds_since, 1),
            error_log=list(self._error_log),
            is_stuck=self.check_stuck(),
        )

    async def run_periodic_check(self, interval: int | None = None) -> None:
        """Background coroutine that periodically updates health status.

        Runs forever; meant to be launched as an asyncio task.
        """
        check_interval = interval or PipelineConfig.HEALTH_CHECK_INTERVAL
        while True:
            self.get_status()  # refresh cached values
            await asyncio.sleep(check_interval)
