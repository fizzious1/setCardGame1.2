"""Anthropic API client wrapper with cost tracking, retry logic, and model routing."""

import asyncio
import time
from datetime import datetime, timezone

import anthropic

from .config import PipelineConfig
from .cost_tracker import CostTracker, BudgetExceededError
from .health_monitor import HealthMonitor
from .activity_log import ActivityLog


class APIClient:
    """Wrapper around the Anthropic SDK with automatic routing, cost tracking,
    exponential backoff, and prompt caching."""

    def __init__(
        self,
        config: PipelineConfig,
        cost_tracker: CostTracker,
        health_monitor: HealthMonitor,
        activity_log: ActivityLog,
    ):
        self._config = config
        self._cost_tracker = cost_tracker
        self._health_monitor = health_monitor
        self._activity_log = activity_log
        self._client = anthropic.AsyncAnthropic(
            api_key=config.ANTHROPIC_API_KEY,
        )

    async def tracked_api_call(
        self,
        agent_id: str | None,
        phase: str,
        step: str,
        system_prompt: str,
        user_prompt: str,
        task_type: str,
        max_tokens: int = 4096,
    ) -> str:
        """Make an API call with automatic model routing, cost tracking, and retries.

        Returns the parsed response text.
        Raises BudgetExceededError if the budget is breached.
        """
        model = PipelineConfig.get_model(task_type)

        last_error: Exception | None = None
        for attempt in range(PipelineConfig.MAX_RETRIES):
            try:
                start_time = time.time()

                # Build the message with prompt caching on system prompt
                response = await self._client.messages.create(
                    model=model,
                    max_tokens=max_tokens,
                    system=[
                        {
                            "type": "text",
                            "text": system_prompt,
                            "cache_control": {"type": "ephemeral"},
                        }
                    ],
                    messages=[{"role": "user", "content": user_prompt}],
                )

                elapsed_ms = (time.time() - start_time) * 1000

                # Extract token counts
                input_tokens = response.usage.input_tokens
                output_tokens = response.usage.output_tokens
                cached_tokens = getattr(
                    response.usage, "cache_read_input_tokens", 0
                ) or 0

                # Calculate cost
                cost = PipelineConfig.calculate_cost(task_type, input_tokens, output_tokens)

                # Record the call
                from .models import APICallRecord

                record = APICallRecord(
                    timestamp=datetime.now(timezone.utc).isoformat(),
                    agent_id=agent_id,
                    phase=phase,
                    step=step,
                    model=model,
                    task_type=task_type,
                    input_tokens=input_tokens,
                    output_tokens=output_tokens,
                    cost_usd=cost,
                    latency_ms=round(elapsed_ms, 1),
                    cached_tokens=cached_tokens,
                )
                self._cost_tracker.record(record)

                # Update health monitor
                self._health_monitor.record_api_call(elapsed_ms)

                # Extract rate limit headers if available
                # (The SDK may expose these through response headers in some versions)

                # Log activity
                tier = PipelineConfig.get_pricing_tier(task_type)
                agent_label = f" [{agent_id}]" if agent_id else ""
                self._activity_log.add_event(
                    phase=phase,
                    agent_id=agent_id,
                    event_type="info",
                    message=(
                        f"API call: {step}{agent_label} | {tier}/{task_type} | "
                        f"{input_tokens}+{output_tokens} tokens | "
                        f"${cost:.4f} | {elapsed_ms:.0f}ms"
                    ),
                )

                # Budget warning
                if self._cost_tracker.is_warning():
                    self._activity_log.add_event(
                        phase=phase,
                        agent_id=None,
                        event_type="warning",
                        message=(
                            f"Budget alert: ${self._cost_tracker.get_total_cost():.2f} "
                            f"of ${PipelineConfig.BUDGET_LIMIT:.2f} "
                            f"({PipelineConfig.ALERT_THRESHOLD * 100:.0f}% threshold reached)"
                        ),
                    )

                # Extract text from response
                response_text = ""
                for block in response.content:
                    if block.type == "text":
                        response_text += block.text

                return response_text

            except BudgetExceededError:
                raise

            except anthropic.RateLimitError as e:
                last_error = e
                wait = PipelineConfig.BACKOFF_BASE ** (attempt + 1)
                self._activity_log.add_event(
                    phase=phase,
                    agent_id=agent_id,
                    event_type="warning",
                    message=f"Rate limited on attempt {attempt + 1}/{PipelineConfig.MAX_RETRIES}. Waiting {wait}s.",
                )
                self._health_monitor.record_error(
                    "RateLimitError", str(e), phase=phase, agent_id=agent_id or ""
                )
                await asyncio.sleep(wait)

            except anthropic.APIStatusError as e:
                last_error = e
                wait = PipelineConfig.BACKOFF_BASE ** (attempt + 1)
                self._activity_log.add_event(
                    phase=phase,
                    agent_id=agent_id,
                    event_type="error",
                    message=f"API error ({e.status_code}) on attempt {attempt + 1}: {e.message}. Retrying in {wait}s.",
                )
                self._health_monitor.record_error(
                    "APIStatusError",
                    f"{e.status_code}: {e.message}",
                    phase=phase,
                    agent_id=agent_id or "",
                )
                await asyncio.sleep(wait)

            except anthropic.APIConnectionError as e:
                last_error = e
                wait = PipelineConfig.BACKOFF_BASE ** (attempt + 1)
                self._activity_log.add_event(
                    phase=phase,
                    agent_id=agent_id,
                    event_type="error",
                    message=f"Connection error on attempt {attempt + 1}: {e}. Retrying in {wait}s.",
                )
                self._health_monitor.record_error(
                    "APIConnectionError", str(e), phase=phase, agent_id=agent_id or ""
                )
                await asyncio.sleep(wait)

            except Exception as e:
                last_error = e
                self._health_monitor.record_error(
                    type(e).__name__, str(e), phase=phase, agent_id=agent_id or ""
                )
                if attempt < PipelineConfig.MAX_RETRIES - 1:
                    wait = PipelineConfig.BACKOFF_BASE ** (attempt + 1)
                    self._activity_log.add_event(
                        phase=phase,
                        agent_id=agent_id,
                        event_type="error",
                        message=f"Unexpected error on attempt {attempt + 1}: {e}. Retrying in {wait}s.",
                    )
                    await asyncio.sleep(wait)

        # All retries exhausted
        self._activity_log.add_event(
            phase=phase,
            agent_id=agent_id,
            event_type="error",
            message=f"All {PipelineConfig.MAX_RETRIES} retries exhausted for {step}. Last error: {last_error}",
        )
        raise RuntimeError(
            f"API call failed after {PipelineConfig.MAX_RETRIES} retries: {last_error}"
        ) from last_error
