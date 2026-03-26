import React from 'react';
import { HealthStatus } from '../types';

function formatTimestamp(iso: string): string {
  try {
    const d = new Date(iso);
    return d.toLocaleTimeString('en-US', { hour: '2-digit', minute: '2-digit', second: '2-digit', hour12: false });
  } catch {
    return iso;
  }
}

function formatDuration(seconds: number): string {
  if (seconds < 60) return `${Math.round(seconds)}s ago`;
  if (seconds < 3600) return `${Math.floor(seconds / 60)}m ${Math.round(seconds % 60)}s ago`;
  return `${Math.floor(seconds / 3600)}h ${Math.floor((seconds % 3600) / 60)}m ago`;
}

interface Props {
  health: HealthStatus;
}

const HealthMonitor: React.FC<Props> = ({ health }) => {
  const {
    api_status, api_latency_ms, rate_limit_remaining, rate_limit_total,
    last_health_check, seconds_since_last_call,
    error_log, is_stuck,
  } = health;

  const statusIndicator = api_status === 'healthy'
    ? 'healthy'
    : api_status === 'slow'
      ? 'slow'
      : 'error';

  const statusText = api_status === 'healthy'
    ? 'API Responding'
    : api_status === 'slow'
      ? 'API Slow'
      : 'API Error';

  const ratePct = rate_limit_total > 0 ? (rate_limit_remaining / rate_limit_total) * 100 : 0;
  const rateClass = ratePct > 50 ? '' : ratePct > 20 ? 'warning' : 'critical';

  const lastCallDanger = seconds_since_last_call > 300;

  return (
    <div className="card">
      <div className="card-title">Health Monitor</div>

      <div className="health-status-row">
        <div className={`health-indicator ${statusIndicator}`} />
        <div className="health-status-text">{statusText}</div>
        <div className="health-latency">{api_latency_ms}ms</div>
      </div>

      {is_stuck && (
        <div className="stuck-alert">
          PIPELINE STUCK -- No progress detected for extended period
        </div>
      )}

      <div className="rate-limit-section">
        <div style={{ display: 'flex', justifyContent: 'space-between' }}>
          <span className="stat-label">Rate Limit</span>
          <span style={{ fontSize: 10, fontFamily: 'var(--font-mono)', color: 'var(--text-muted)' }}>
            {rate_limit_remaining} / {rate_limit_total}
          </span>
        </div>
        <div className="rate-limit-bar">
          <div className={`rate-limit-fill ${rateClass}`} style={{ width: `${ratePct}%` }} />
        </div>
      </div>

      <div className="health-stats">
        <div className="health-stat-row">
          <span className="health-stat-label">Last Health Check</span>
          <span className="health-stat-value">{formatTimestamp(last_health_check)}</span>
        </div>
        <div className="health-stat-row">
          <span className="health-stat-label">Time Since Last API Call</span>
          <span className={`health-stat-value ${lastCallDanger ? 'danger' : ''}`}>
            {formatDuration(seconds_since_last_call)}
          </span>
        </div>
      </div>

      {error_log.length > 0 && (
        <>
          <div className="error-log-title">Error Log ({error_log.length})</div>
          <div className="error-log">
            {error_log.slice(-10).reverse().map((entry, i) => (
              <div className="error-log-item" key={i}>
                <div className="error-log-time">{formatTimestamp(entry.timestamp)}</div>
                <div className="error-log-msg">{entry.error}</div>
              </div>
            ))}
          </div>
        </>
      )}
    </div>
  );
};

export default HealthMonitor;
