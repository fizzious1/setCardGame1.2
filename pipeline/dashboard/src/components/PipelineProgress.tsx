import React from 'react';
import { PhaseProgress } from '../types';

const PHASE_LABELS = ['Knowledge Base', 'Persona Generation', 'Conditioning & Validation', 'Export'];

function formatTime(seconds: number): string {
  if (seconds <= 0) return '0s';
  const h = Math.floor(seconds / 3600);
  const m = Math.floor((seconds % 3600) / 60);
  const s = Math.floor(seconds % 60);
  const parts: string[] = [];
  if (h > 0) parts.push(`${h}h`);
  if (m > 0) parts.push(`${m}m`);
  if (s > 0 || parts.length === 0) parts.push(`${s}s`);
  return parts.join(' ');
}

function formatTimestamp(iso: string): string {
  try {
    const d = new Date(iso);
    return d.toLocaleTimeString('en-US', { hour: '2-digit', minute: '2-digit', second: '2-digit', hour12: false });
  } catch {
    return iso;
  }
}

interface Props {
  progress: PhaseProgress;
}

const PipelineProgress: React.FC<Props> = ({ progress }) => {
  const { current_phase, phase_name, steps_completed, total_steps, progress_percent, estimated_remaining_seconds, last_action, last_action_timestamp, current_step } = progress;

  const phaseInternalPercent = (() => {
    if (total_steps === 0) return 0;
    const phasesCompleted = current_phase - 1;
    const stepsPerPhase = total_steps / 4;
    const stepsInCurrentPhase = steps_completed - phasesCompleted * stepsPerPhase;
    return Math.min(100, Math.max(0, (stepsInCurrentPhase / stepsPerPhase) * 100));
  })();

  return (
    <div className="card">
      <div className="card-title">Pipeline Progress</div>

      <div className="phase-header">
        <div className="phase-number">{current_phase}</div>
        <div>
          <div className="phase-name">{phase_name}</div>
          <div style={{ fontSize: 12, color: 'var(--text-secondary)', marginTop: 2 }}>{current_step}</div>
        </div>
      </div>

      <div className="phase-bar-container">
        {[1, 2, 3, 4].map((phase) => (
          <div
            key={phase}
            className={`phase-segment ${phase < current_phase ? 'completed' : phase === current_phase ? 'current' : ''}`}
          >
            {phase === current_phase && (
              <div className="phase-segment-fill" style={{ width: `${phaseInternalPercent}%` }} />
            )}
          </div>
        ))}
      </div>

      <div className="phase-labels">
        {PHASE_LABELS.map((label, i) => (
          <div
            key={label}
            className={`phase-label ${i + 1 === current_phase ? 'active' : i + 1 < current_phase ? 'done' : ''}`}
          >
            {label}
          </div>
        ))}
      </div>

      <div className="progress-stats">
        <div className="stat-item">
          <div className="stat-label">Overall Progress</div>
          <div className="stat-value">{progress_percent.toFixed(1)}%</div>
        </div>
        <div className="stat-item">
          <div className="stat-label">Steps</div>
          <div className="stat-value">{steps_completed} / {total_steps}</div>
        </div>
        <div className="stat-item">
          <div className="stat-label">Est. Remaining</div>
          <div className="stat-value">{formatTime(estimated_remaining_seconds)}</div>
        </div>
        <div className="stat-item">
          <div className="stat-label">Current Phase</div>
          <div className="stat-value">Phase {current_phase} of 4</div>
        </div>
      </div>

      <div className="last-action">
        <div className="last-action-text">{last_action}</div>
        <div className="last-action-time">{formatTimestamp(last_action_timestamp)}</div>
      </div>
    </div>
  );
};

export default PipelineProgress;
