import React, { useState, useMemo } from 'react';
import type { DailyRecap as DailyRecapType, GameEvent, Participant } from '../types';

interface Props {
  recap: DailyRecapType | undefined;
  currentDay: number;
  totalDays: number;
  participants: Participant[];
  getEventById: (id: string) => GameEvent | undefined;
  onDayChange: (day: number) => void;
  onEventClick: (eventId: string) => void;
}

export const DailyRecap: React.FC<Props> = ({
  recap,
  currentDay,
  totalDays,
  participants,
  getEventById,
  onDayChange,
  onEventClick,
}) => {
  const [viewDay, setViewDay] = useState(currentDay);

  // Keep viewDay in sync with currentDay when it changes
  React.useEffect(() => {
    setViewDay(currentDay);
  }, [currentDay]);

  const participantNameMap = useMemo(() => {
    const m = new Map<string, string>();
    for (const p of participants) m.set(p.id, p.name);
    return m;
  }, [participants]);

  const handlePrev = () => {
    const d = Math.max(1, viewDay - 1);
    setViewDay(d);
    onDayChange(d);
  };

  const handleNext = () => {
    const d = Math.min(totalDays, viewDay + 1);
    setViewDay(d);
    onDayChange(d);
  };

  if (!recap) {
    return (
      <div className="daily-recap">
        <div className="recap-day-nav">
          <button onClick={handlePrev} disabled={viewDay <= 1}>
            <svg width="12" height="12" viewBox="0 0 12 12" fill="currentColor">
              <polygon points="8,1 3,6 8,11" />
            </svg>
          </button>
          <span className="recap-day-title">Day {viewDay}</span>
          <button onClick={handleNext} disabled={viewDay >= totalDays}>
            <svg width="12" height="12" viewBox="0 0 12 12" fill="currentColor">
              <polygon points="4,1 9,6 4,11" />
            </svg>
          </button>
        </div>
        <div className="recap-empty">No recap available for this day.</div>
      </div>
    );
  }

  const keyEvents = recap.keyEventIds
    .map((id) => getEventById(id))
    .filter((e): e is GameEvent => e !== undefined);

  return (
    <div className="daily-recap">
      {/* Day Navigation */}
      <div className="recap-day-nav">
        <button onClick={handlePrev} disabled={viewDay <= 1}>
          <svg width="12" height="12" viewBox="0 0 12 12" fill="currentColor">
            <polygon points="8,1 3,6 8,11" />
          </svg>
        </button>
        <span className="recap-day-title">Day {recap.day}</span>
        <button onClick={handleNext} disabled={viewDay >= totalDays}>
          <svg width="12" height="12" viewBox="0 0 12 12" fill="currentColor">
            <polygon points="4,1 9,6 4,11" />
          </svg>
        </button>
      </div>

      {/* Summary */}
      <div className="recap-summary">{recap.summary}</div>

      {/* Drama Score */}
      <div className="recap-section">
        <div className="recap-section-title">Drama Score</div>
        <div className="recap-drama-score">
          <div className="drama-bar">
            <div
              className="drama-bar-fill"
              style={{ width: `${(recap.dramaScore / 10) * 100}%` }}
            />
          </div>
          <span className="drama-score-value">{recap.dramaScore}/10</span>
        </div>
      </div>

      {/* HoH */}
      {recap.hohWinner && (
        <div className="recap-section">
          <div className="recap-section-title">Head of Household</div>
          <span className="recap-hoh">
            {participantNameMap.get(recap.hohWinner) ?? recap.hohWinner}
          </span>
        </div>
      )}

      {/* Nominations */}
      {recap.nominations && recap.nominations.length > 0 && (
        <div className="recap-section">
          <div className="recap-section-title">Nominations</div>
          <div className="recap-nominations">
            {recap.nominations.map((pid) => (
              <span key={pid} className="recap-nominee">
                {participantNameMap.get(pid) ?? pid}
              </span>
            ))}
          </div>
        </div>
      )}

      {/* Eviction */}
      {recap.evicted && (
        <div className="recap-section">
          <div className="recap-section-title">Evicted</div>
          <span className="recap-evicted">
            {participantNameMap.get(recap.evicted) ?? recap.evicted}
          </span>
        </div>
      )}

      {/* Key Events */}
      <div className="recap-section">
        <div className="recap-section-title">Key Events</div>
        {keyEvents.map((evt) => (
          <div
            key={evt.id}
            className={`recap-event-item event-border-${evt.type}`}
            onClick={() => onEventClick(evt.id)}
          >
            <strong>{evt.title}</strong>
            <div style={{ fontSize: 10, color: 'var(--text-muted)', marginTop: 2 }}>
              {evt.hour}:00 &middot; {evt.type}
            </div>
          </div>
        ))}
        {keyEvents.length === 0 && (
          <div className="recap-empty">No key events recorded.</div>
        )}
      </div>

      {/* Alliances */}
      {recap.alliances.length > 0 && (
        <div className="recap-section">
          <div className="recap-section-title">Active Alliances</div>
          <div className="recap-alliance-list">
            {recap.alliances.map(([a, b], i) => (
              <div key={i} className="recap-alliance-pair">
                <span>{participantNameMap.get(a) ?? a}</span>
                <span className="recap-alliance-arrow">&harr;</span>
                <span>{participantNameMap.get(b) ?? b}</span>
              </div>
            ))}
          </div>
        </div>
      )}

      {/* Mood Trends */}
      {Object.keys(recap.moodTrends).length > 0 && (
        <div className="recap-section">
          <div className="recap-section-title">End-of-Day Moods</div>
          {Object.entries(recap.moodTrends).map(([pid, moods]) => {
            const lastMood = moods[moods.length - 1] ?? 0;
            const pct = ((lastMood + 1) / 2) * 100;
            const color =
              lastMood > 0.3
                ? 'var(--success)'
                : lastMood < -0.3
                  ? 'var(--danger)'
                  : 'var(--warning)';
            return (
              <div key={pid} className="inspector-stat-row">
                <span className="inspector-stat-label" style={{ width: 80, fontSize: 11 }}>
                  {participantNameMap.get(pid) ?? pid}
                </span>
                <div className="inspector-stat-bar">
                  <div
                    className="inspector-stat-fill"
                    style={{ width: `${pct}%`, backgroundColor: color }}
                  />
                </div>
                <span className="inspector-stat-value">
                  {lastMood > 0 ? '+' : ''}
                  {lastMood.toFixed(1)}
                </span>
              </div>
            );
          })}
        </div>
      )}
    </div>
  );
};
