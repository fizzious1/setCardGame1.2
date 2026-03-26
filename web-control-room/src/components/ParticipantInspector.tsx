import React, { useMemo } from 'react';
import type {
  Participant,
  ParticipantState,
  Personality,
  GameEvent,
} from '../types';

interface Props {
  participant: Participant;
  state: ParticipantState;
  allParticipants: Participant[];
  recentEvents: GameEvent[];
  onEventClick: (eventId: string) => void;
}

const PERSONALITY_AXES: { key: keyof Personality; label: string }[] = [
  { key: 'extraversion', label: 'EXT' },
  { key: 'agreeableness', label: 'AGR' },
  { key: 'emotionality', label: 'EMO' },
  { key: 'strategy', label: 'STR' },
  { key: 'loyalty', label: 'LOY' },
];

function RadarChart({ personality }: { personality: Personality }) {
  const cx = 70;
  const cy = 70;
  const r = 55;
  const n = PERSONALITY_AXES.length;
  const angleStep = (2 * Math.PI) / n;
  const startAngle = -Math.PI / 2;

  // Grid rings
  const rings = [0.25, 0.5, 0.75, 1.0];

  // Compute polygon points for the data
  const dataPoints = PERSONALITY_AXES.map((axis, i) => {
    const angle = startAngle + i * angleStep;
    const value = personality[axis.key];
    return {
      x: cx + Math.cos(angle) * r * value,
      y: cy + Math.sin(angle) * r * value,
    };
  });
  const dataPolygon = dataPoints.map((p) => `${p.x},${p.y}`).join(' ');

  // Label positions
  const labels = PERSONALITY_AXES.map((axis, i) => {
    const angle = startAngle + i * angleStep;
    return {
      x: cx + Math.cos(angle) * (r + 14),
      y: cy + Math.sin(angle) * (r + 14),
      text: axis.label,
    };
  });

  return (
    <div className="radar-chart">
      <svg width="140" height="140" viewBox="0 0 140 140">
        {/* Grid */}
        {rings.map((ringVal) => {
          const pts = PERSONALITY_AXES.map((_, i) => {
            const angle = startAngle + i * angleStep;
            return `${cx + Math.cos(angle) * r * ringVal},${cy + Math.sin(angle) * r * ringVal}`;
          }).join(' ');
          return (
            <polygon key={ringVal} className="radar-grid" points={pts} />
          );
        })}
        {/* Axes */}
        {PERSONALITY_AXES.map((_, i) => {
          const angle = startAngle + i * angleStep;
          return (
            <line
              key={i}
              x1={cx}
              y1={cy}
              x2={cx + Math.cos(angle) * r}
              y2={cy + Math.sin(angle) * r}
              className="radar-grid"
            />
          );
        })}
        {/* Data polygon */}
        <polygon className="radar-polygon" points={dataPolygon} />
        {/* Labels */}
        {labels.map((l, i) => (
          <text
            key={i}
            className="radar-label"
            x={l.x}
            y={l.y}
            textAnchor="middle"
            dominantBaseline="middle"
          >
            {l.text}
          </text>
        ))}
      </svg>
    </div>
  );
}

const EVENT_TYPE_COLORS: Record<string, string> = {
  conflict: 'var(--event-conflict)',
  alliance: 'var(--event-alliance)',
  nomination: 'var(--event-nomination)',
  eviction: 'var(--event-eviction)',
  competition: 'var(--event-competition)',
  confession: 'var(--event-confession)',
  conversation: 'var(--event-conversation)',
  betrayal: 'var(--event-betrayal)',
  social: 'var(--event-social)',
  ceremony: 'var(--event-ceremony)',
};

export const ParticipantInspector: React.FC<Props> = ({
  participant,
  state,
  allParticipants,
  recentEvents,
  onEventClick,
}) => {
  const participantNameMap = useMemo(() => {
    const m = new Map<string, string>();
    for (const p of allParticipants) m.set(p.id, p.name);
    return m;
  }, [allParticipants]);

  const initials = participant.name
    .split(' ')
    .map((w) => w[0])
    .join('');

  // Sort trust entries by absolute value descending
  const trustEntries = useMemo(() => {
    return Object.entries(state.trust)
      .filter(([pid]) => pid !== participant.id)
      .sort((a, b) => Math.abs(b[1]) - Math.abs(a[1]));
  }, [state.trust, participant.id]);

  const moodPct = ((state.mood + 1) / 2) * 100; // -1..1 -> 0..100
  const moodColor =
    state.mood > 0.3
      ? 'var(--success)'
      : state.mood < -0.3
        ? 'var(--danger)'
        : 'var(--warning)';

  return (
    <div className="inspector">
      {/* Header */}
      <div className="inspector-header">
        <div
          className="inspector-avatar"
          style={{ backgroundColor: participant.avatarColor }}
        >
          {initials}
        </div>
        <div>
          <div className="inspector-name">{participant.name}</div>
          <div className="inspector-meta">
            Age {participant.age} &middot; {participant.occupation || 'Houseguest'} &middot;{' '}
            <span style={{ textTransform: 'capitalize' }}>{state.status}</span>
          </div>
        </div>
      </div>

      {/* Bio */}
      <div className="inspector-bio">{participant.bio}</div>

      {/* Current Info */}
      <div className="inspector-section">
        <div className="inspector-section-title">Current Status</div>
        <div className="inspector-current-info">
          <div className="inspector-info-item">
            <span className="inspector-info-label">Room:</span>
            <span className="inspector-info-value">
              {state.room.replace(/_/g, ' ')}
            </span>
          </div>
          <div className="inspector-info-item">
            <span className="inspector-info-label">Goal:</span>
            <span className="inspector-info-value">{state.currentGoal}</span>
          </div>
        </div>
      </div>

      {/* Mood & Energy */}
      <div className="inspector-section">
        <div className="inspector-section-title">Vitals</div>
        <div className="inspector-stat-row">
          <span className="inspector-stat-label">Mood</span>
          <div className="inspector-stat-bar">
            <div
              className="inspector-stat-fill"
              style={{
                width: `${moodPct}%`,
                backgroundColor: moodColor,
              }}
            />
          </div>
          <span className="inspector-stat-value">
            {state.mood > 0 ? '+' : ''}
            {state.mood.toFixed(2)}
          </span>
        </div>
        <div className="inspector-stat-row">
          <span className="inspector-stat-label">Energy</span>
          <div className="inspector-stat-bar">
            <div
              className="inspector-stat-fill"
              style={{
                width: `${state.energy * 100}%`,
                backgroundColor: 'var(--cyan)',
              }}
            />
          </div>
          <span className="inspector-stat-value">
            {(state.energy * 100).toFixed(0)}%
          </span>
        </div>
        <div className="inspector-stat-row">
          <span className="inspector-stat-label">Stress</span>
          <div className="inspector-stat-bar">
            <div
              className="inspector-stat-fill"
              style={{
                width: `${state.stress * 100}%`,
                backgroundColor: 'var(--danger)',
              }}
            />
          </div>
          <span className="inspector-stat-value">
            {(state.stress * 100).toFixed(0)}%
          </span>
        </div>
      </div>

      {/* Personality Radar */}
      <div className="inspector-section">
        <div className="inspector-section-title">Personality</div>
        <RadarChart personality={participant.personality} />
      </div>

      {/* Trust */}
      <div className="inspector-section">
        <div className="inspector-section-title">Trust Levels</div>
        {trustEntries.map(([pid, trust]) => {
          const name = participantNameMap.get(pid) ?? pid;
          const absPct = Math.abs(trust) * 50; // 50% width max
          return (
            <div key={pid} className="trust-bar-row">
              <span className="trust-bar-name">{name}</span>
              <div className="trust-bar-track">
                <div className="trust-bar-center-line" />
                <div
                  className={`trust-bar-fill ${trust >= 0 ? 'positive' : 'negative'}`}
                  style={{
                    width: `${absPct}%`,
                    ...(trust < 0
                      ? { right: '50%', left: 'auto' }
                      : { left: '50%' }),
                  }}
                />
              </div>
              <span className="trust-value">
                {trust > 0 ? '+' : ''}
                {trust.toFixed(1)}
              </span>
            </div>
          );
        })}
      </div>

      {/* Recent Events */}
      <div className="inspector-section">
        <div className="inspector-section-title">Recent Events</div>
        <div className="inspector-events-list">
          {recentEvents.length === 0 && (
            <div style={{ fontSize: 12, color: 'var(--text-muted)' }}>
              No recent events
            </div>
          )}
          {recentEvents.slice(0, 8).map((evt) => (
            <div
              key={evt.id}
              className="inspector-event-item"
              style={{ borderLeftColor: EVENT_TYPE_COLORS[evt.type] ?? 'var(--border)' }}
              onClick={() => onEventClick(evt.id)}
            >
              <strong>{evt.title}</strong> &mdash; Day {evt.day}, {evt.hour}:00
            </div>
          ))}
        </div>
      </div>
    </div>
  );
};
