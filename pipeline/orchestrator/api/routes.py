"""FastAPI routes for the Big Brother AI Simulator pipeline dashboard API."""

from fastapi import APIRouter, HTTPException, Query

from ..models import (
    PipelineState,
    HealthStatus,
    CostBreakdown,
    AgentPersona,
    ActivityEvent,
    PhaseProgress,
)

router = APIRouter(prefix="/api", tags=["pipeline"])


def _get_state() -> "PipelineState":
    """Retrieve the global pipeline state. Imported lazily to avoid circular deps."""
    from ..main import get_pipeline_state
    return get_pipeline_state()


def _get_activity_log():
    from ..main import get_activity_log
    return get_activity_log()


def _get_cost_tracker():
    from ..main import get_cost_tracker
    return get_cost_tracker()


def _get_health_monitor():
    from ..main import get_health_monitor
    return get_health_monitor()


@router.get("/state", response_model=PipelineState)
async def get_state():
    """Return full pipeline state for the dashboard."""
    state = _get_state()
    # Refresh costs and health
    state.costs = _get_cost_tracker().get_breakdown()
    state.health = _get_health_monitor().get_status()
    state.events = _get_activity_log().get_events(skip=0, limit=50)
    return state


@router.get("/health", response_model=HealthStatus)
async def get_health():
    """Return current health status."""
    return _get_health_monitor().get_status()


@router.get("/costs", response_model=CostBreakdown)
async def get_costs():
    """Return full cost breakdown."""
    return _get_cost_tracker().get_breakdown()


@router.get("/agents", response_model=list[AgentPersona])
async def get_agents():
    """Return all agent personas."""
    state = _get_state()
    return state.agents


@router.get("/agents/{agent_id}", response_model=AgentPersona)
async def get_agent(agent_id: str):
    """Return a single agent persona by ID."""
    state = _get_state()
    for agent in state.agents:
        if agent.id == agent_id:
            return agent
    raise HTTPException(status_code=404, detail=f"Agent '{agent_id}' not found")


@router.get("/events", response_model=list[ActivityEvent])
async def get_events(
    skip: int = Query(0, ge=0),
    limit: int = Query(100, ge=1, le=1000),
    phase: str | None = Query(None),
    agent_id: str | None = Query(None),
    event_type: str | None = Query(None),
):
    """Return activity events with pagination and optional filters."""
    return _get_activity_log().get_events(
        skip=skip,
        limit=limit,
        phase_filter=phase,
        agent_filter=agent_id,
        type_filter=event_type,
    )


@router.get("/progress", response_model=PhaseProgress)
async def get_progress():
    """Return current pipeline progress."""
    state = _get_state()
    return state.progress
