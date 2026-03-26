"""FastAPI application entry point for the Big Brother AI Simulator pipeline."""

import asyncio
import uvicorn
from fastapi import FastAPI, BackgroundTasks
from fastapi.middleware.cors import CORSMiddleware

from .config import PipelineConfig
from .models import PipelineState
from .cost_tracker import CostTracker
from .health_monitor import HealthMonitor
from .activity_log import ActivityLog
from .api_client import APIClient
from .api.routes import router as api_router

# ---------------------------------------------------------------------------
# Global singletons
# ---------------------------------------------------------------------------
_config = PipelineConfig()
_cost_tracker = CostTracker(_config)
_health_monitor = HealthMonitor(_cost_tracker, _config)
_activity_log = ActivityLog()
_api_client = APIClient(_config, _cost_tracker, _health_monitor, _activity_log)
_pipeline_state = PipelineState()
_pipeline_running = False


def get_pipeline_state() -> PipelineState:
    return _pipeline_state


def get_cost_tracker() -> CostTracker:
    return _cost_tracker


def get_health_monitor() -> HealthMonitor:
    return _health_monitor


def get_activity_log() -> ActivityLog:
    return _activity_log


def get_api_client() -> APIClient:
    return _api_client


def get_config() -> PipelineConfig:
    return _config


# ---------------------------------------------------------------------------
# FastAPI application
# ---------------------------------------------------------------------------
app = FastAPI(
    title="Big Brother AI Simulator — Conditioning Pipeline",
    description="Backend API for the AI agent conditioning pipeline dashboard.",
    version="1.0.0",
)

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

app.include_router(api_router)


@app.get("/")
async def root():
    return {
        "service": "Big Brother AI Simulator — Conditioning Pipeline",
        "status": "running" if _pipeline_running else "idle",
        "version": "1.0.0",
    }


@app.post("/api/start")
async def start_pipeline(background_tasks: BackgroundTasks):
    """Start the full conditioning pipeline as a background task."""
    global _pipeline_running

    if _pipeline_running:
        return {"status": "already_running", "message": "Pipeline is already in progress."}

    _pipeline_running = True
    _activity_log.add_event(
        phase="startup",
        agent_id=None,
        event_type="info",
        message="Pipeline start requested.",
    )

    background_tasks.add_task(_run_pipeline)
    return {"status": "started", "message": "Pipeline launched as background task."}


async def _run_pipeline():
    """Execute the full pipeline orchestration."""
    global _pipeline_running
    try:
        from .orchestrator import PipelineOrchestrator

        orchestrator = PipelineOrchestrator(
            config=_config,
            cost_tracker=_cost_tracker,
            health_monitor=_health_monitor,
            activity_log=_activity_log,
            api_client=_api_client,
            state=_pipeline_state,
        )
        _health_monitor.set_pipeline_running(True)
        await orchestrator.run()
    except Exception as e:
        _activity_log.add_event(
            phase="pipeline",
            agent_id=None,
            event_type="error",
            message=f"Pipeline crashed: {type(e).__name__}: {e}",
        )
        _health_monitor.record_error("PipelineCrash", str(e), phase="pipeline")
    finally:
        _pipeline_running = False
        _health_monitor.set_pipeline_running(False)
        _activity_log.add_event(
            phase="pipeline",
            agent_id=None,
            event_type="info",
            message="Pipeline execution finished.",
        )


def main():
    """Run the application with uvicorn."""
    uvicorn.run(
        "pipeline.orchestrator.main:app",
        host="0.0.0.0",
        port=8420,
        reload=False,
    )


if __name__ == "__main__":
    main()
