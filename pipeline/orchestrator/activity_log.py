"""Thread-safe activity log for the Big Brother AI Simulator conditioning pipeline."""

import threading
from collections import deque
from datetime import datetime, timezone

from .models import ActivityEvent


class ActivityLog:
    """Thread-safe, bounded FIFO event log."""

    MAX_EVENTS = 10_000

    def __init__(self) -> None:
        self._lock = threading.Lock()
        self._events: deque[ActivityEvent] = deque(maxlen=self.MAX_EVENTS)

    def add_event(
        self,
        phase: str,
        agent_id: str | None,
        event_type: str,
        message: str,
    ) -> ActivityEvent:
        """Create and store a new activity event. Returns the created event."""
        event = ActivityEvent(
            timestamp=datetime.now(timezone.utc).isoformat(),
            phase=phase,
            agent_id=agent_id,
            event_type=event_type,
            message=message,
        )
        with self._lock:
            self._events.append(event)
        return event

    def get_events(
        self,
        skip: int = 0,
        limit: int = 100,
        phase_filter: str | None = None,
        agent_filter: str | None = None,
        type_filter: str | None = None,
    ) -> list[ActivityEvent]:
        """Retrieve events with optional filtering and pagination.

        Returned in reverse chronological order (newest first).
        """
        with self._lock:
            events = list(self._events)

        # Newest first
        events.reverse()

        # Apply filters
        if phase_filter:
            events = [e for e in events if e.phase == phase_filter]
        if agent_filter:
            events = [e for e in events if e.agent_id == agent_filter]
        if type_filter:
            events = [e for e in events if e.event_type == type_filter]

        # Pagination
        return events[skip : skip + limit]

    def count(self) -> int:
        with self._lock:
            return len(self._events)

    def get_all(self) -> list[ActivityEvent]:
        """Return all events newest-first."""
        with self._lock:
            events = list(self._events)
        events.reverse()
        return events
