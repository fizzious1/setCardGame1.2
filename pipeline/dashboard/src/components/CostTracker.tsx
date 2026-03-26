import React from 'react';
import { CostBreakdown } from '../types';

const BUDGET = 75;

function fmt(n: number): string {
  return `$${n.toFixed(2)}`;
}

function fmtK(n: number): string {
  if (n >= 1_000_000) return `${(n / 1_000_000).toFixed(1)}M`;
  if (n >= 1_000) return `${(n / 1_000).toFixed(1)}K`;
  return String(n);
}

interface Props {
  costs: CostBreakdown;
}

const CostTracker: React.FC<Props> = ({ costs }) => {
  const {
    total_cost, opus_cost, sonnet_cost, embedding_cost,
    cost_by_phase, cost_by_agent,
    total_input_tokens, total_output_tokens,
    total_api_calls, opus_calls, sonnet_calls,
    avg_cost_per_opus_call, avg_cost_per_sonnet_call,
    estimated_total_cost, budget_remaining,
    cost_over_time,
  } = costs;

  const costColor = total_cost < 37.5 ? 'green' : total_cost < 60 ? 'yellow' : 'red';
  const budgetPercent = Math.min(100, (total_cost / BUDGET) * 100);

  const phaseEntries = Object.entries(cost_by_phase);
  const maxPhaseCost = Math.max(...phaseEntries.map(([, v]) => v), 1);

  const agentEntries = Object.entries(cost_by_agent).filter(([, v]) => v > 0);
  const maxAgentCost = Math.max(...agentEntries.map(([, v]) => v), 1);

  // Chart data
  const opusPoints = cost_over_time.filter(d => d.model === 'opus');
  const sonnetPoints = cost_over_time.filter(d => d.model === 'sonnet');
  const allCosts = cost_over_time.map(d => d.cumulative_cost);
  const maxChartCost = Math.max(...allCosts, 1);
  const chartW = 340;
  const chartH = 80;
  const chartPadL = 30;
  const chartPadB = 14;

  function toChartPath(points: typeof cost_over_time): string {
    if (points.length === 0) return '';
    const xStep = (chartW - chartPadL) / Math.max(cost_over_time.length - 1, 1);
    return points
      .map((p, i) => {
        const idx = cost_over_time.indexOf(p);
        const x = chartPadL + idx * xStep;
        const y = chartH - chartPadB - (p.cumulative_cost / maxChartCost) * (chartH - chartPadB - 4);
        return `${i === 0 ? 'M' : 'L'}${x.toFixed(1)},${y.toFixed(1)}`;
      })
      .join(' ');
  }

  function toChartArea(points: typeof cost_over_time): string {
    if (points.length === 0) return '';
    const xStep = (chartW - chartPadL) / Math.max(cost_over_time.length - 1, 1);
    const baseline = chartH - chartPadB;
    const firstIdx = cost_over_time.indexOf(points[0]);
    const lastIdx = cost_over_time.indexOf(points[points.length - 1]);
    const firstX = chartPadL + firstIdx * xStep;
    const lastX = chartPadL + lastIdx * xStep;
    return `${toChartPath(points)} L${lastX.toFixed(1)},${baseline} L${firstX.toFixed(1)},${baseline} Z`;
  }

  return (
    <div className="card">
      <div className="card-title">Cost Tracker</div>

      <div className="cost-hero">
        <div className={`cost-total ${costColor}`}>{fmt(total_cost)}</div>
        <div className="cost-label">of {fmt(BUDGET)} budget</div>
      </div>

      {/* Budget Bar */}
      <div className="budget-bar">
        <div className="budget-bar-track">
          <div
            className="budget-bar-fill"
            style={{
              width: `${budgetPercent}%`,
              background: `var(--${costColor})`,
            }}
          />
          <div className="budget-marker" style={{ left: '50%' }}>
            <div className="budget-marker-label">$37.50</div>
          </div>
          <div className="budget-marker" style={{ left: '80%' }}>
            <div className="budget-marker-label">$60</div>
          </div>
        </div>
        <div className="budget-row">
          <span>$0</span>
          <span>{fmt(BUDGET)}</span>
        </div>
      </div>

      {/* Model Split */}
      <div className="model-split">
        <div className="model-split-bar">
          <div className="model-split-opus" style={{ width: `${(opus_cost / Math.max(total_cost, 0.01)) * 100}%` }} />
          <div className="model-split-sonnet" style={{ width: `${(sonnet_cost / Math.max(total_cost, 0.01)) * 100}%` }} />
        </div>
        <div className="model-split-labels">
          <span><span className="model-dot opus" /> Opus {fmt(opus_cost)}</span>
          <span><span className="model-dot sonnet" /> Sonnet {fmt(sonnet_cost)}</span>
          <span><span className="model-dot embedding" /> Embed {fmt(embedding_cost)}</span>
        </div>
      </div>

      {/* Cost by Phase */}
      <div className="cost-section">
        <div className="cost-section-title">Cost by Phase</div>
        {phaseEntries.map(([phase, cost], i) => (
          <div className="cost-bar-row" key={phase}>
            <div className="cost-bar-label">{phase}</div>
            <div className="cost-bar-track">
              <div
                className={`cost-bar-fill phase${i + 1}`}
                style={{ width: `${(cost / maxPhaseCost) * 100}%` }}
              />
            </div>
            <div className="cost-bar-value">{fmt(cost)}</div>
          </div>
        ))}
      </div>

      {/* Cost by Agent */}
      {agentEntries.length > 0 && (
        <div className="cost-section">
          <div className="cost-section-title">Cost by Agent</div>
          <div className="agent-cost-grid">
            {agentEntries.map(([id, cost]) => {
              const num = id.replace('agent-', '');
              return (
                <div className="agent-cost-mini" key={id}>
                  <div className="agent-cost-mini-bar">
                    <div className="agent-cost-mini-fill" style={{ width: `${(cost / maxAgentCost) * 100}%` }} />
                  </div>
                  <div className="agent-cost-mini-label">#{num}</div>
                </div>
              );
            })}
          </div>
        </div>
      )}

      {/* Stats Grid */}
      <div className="cost-stats-grid">
        <div className="cost-stat">
          <div className="cost-stat-value">{fmtK(total_input_tokens)}</div>
          <div className="cost-stat-label">Input Tokens</div>
        </div>
        <div className="cost-stat">
          <div className="cost-stat-value">{fmtK(total_output_tokens)}</div>
          <div className="cost-stat-label">Output Tokens</div>
        </div>
        <div className="cost-stat">
          <div className="cost-stat-value">{total_api_calls}</div>
          <div className="cost-stat-label">API Calls</div>
        </div>
        <div className="cost-stat">
          <div className="cost-stat-value">{opus_calls}</div>
          <div className="cost-stat-label">Opus Calls</div>
        </div>
        <div className="cost-stat">
          <div className="cost-stat-value">{sonnet_calls}</div>
          <div className="cost-stat-label">Sonnet Calls</div>
        </div>
        <div className="cost-stat">
          <div className="cost-stat-value">{fmt(estimated_total_cost)}</div>
          <div className="cost-stat-label">Est. Total</div>
        </div>
        <div className="cost-stat">
          <div className="cost-stat-value">{fmt(avg_cost_per_opus_call)}</div>
          <div className="cost-stat-label">Avg/Opus Call</div>
        </div>
        <div className="cost-stat">
          <div className="cost-stat-value">{fmt(avg_cost_per_sonnet_call)}</div>
          <div className="cost-stat-label">Avg/Sonnet Call</div>
        </div>
        <div className="cost-stat">
          <div className="cost-stat-value" style={{ color: budget_remaining > 20 ? 'var(--green)' : 'var(--yellow)' }}>
            {fmt(budget_remaining)}
          </div>
          <div className="cost-stat-label">Remaining</div>
        </div>
      </div>

      {/* Cost Over Time Chart */}
      <div className="cost-chart">
        <div className="cost-section-title">Cost Over Time</div>
        <svg viewBox={`0 0 ${chartW} ${chartH}`} preserveAspectRatio="xMidYMid meet">
          {/* Grid lines */}
          {[0, 0.25, 0.5, 0.75, 1].map((f) => {
            const y = chartH - chartPadB - f * (chartH - chartPadB - 4);
            return (
              <g key={f}>
                <line x1={chartPadL} y1={y} x2={chartW} y2={y} className="chart-grid-line" />
                <text x={chartPadL - 3} y={y + 3} textAnchor="end" className="chart-label">
                  {fmt(maxChartCost * f)}
                </text>
              </g>
            );
          })}
          {/* Areas */}
          <path d={toChartArea(opusPoints)} className="chart-area opus" />
          <path d={toChartArea(sonnetPoints)} className="chart-area sonnet" />
          {/* Lines */}
          <path d={toChartPath(opusPoints)} className="chart-line opus" />
          <path d={toChartPath(sonnetPoints)} className="chart-line sonnet" />
        </svg>
      </div>
    </div>
  );
};

export default CostTracker;
