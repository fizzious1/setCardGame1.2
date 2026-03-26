import React, { useRef, useCallback, useMemo } from 'react';
import type { GameEvent, HighlightIndex } from '../types';

interface Props {
  currentTick: number;
  maxTick: number;
  ticksPerDay: number;
  totalDays: number;
  events: GameEvent[];
  highlights: HighlightIndex[];
  onScrub: (tick: number) => void;
  onEventClick: (eventId: string) => void;
}

const EVENT_TYPE_COLORS: Record<string, string> = {
  conflict: '#ef4444',
  alliance: '#22c55e',
  nomination: '#eab308',
  eviction: '#a855f7',
  competition: '#06b6d4',
  confession: '#f97316',
  conversation: '#64748b',
  betrayal: '#dc2626',
  social: '#8b5cf6',
  ceremony: '#f59e0b',
};

export const Timeline: React.FC<Props> = ({
  currentTick,
  maxTick,
  ticksPerDay,
  totalDays,
  events,
  highlights,
  onScrub,
  onEventClick,
}) => {
  const trackRef = useRef<HTMLDivElement>(null);
  const dragging = useRef(false);

  const highlightSet = useMemo(
    () => new Set(highlights.map((h) => h.eventId)),
    [highlights],
  );

  const tickToPercent = useCallback(
    (tick: number) => {
      if (maxTick === 0) return 0;
      return (tick / maxTick) * 100;
    },
    [maxTick],
  );

  const getTickFromMouseEvent = useCallback(
    (e: React.MouseEvent | MouseEvent) => {
      if (!trackRef.current) return 0;
      const rect = trackRef.current.getBoundingClientRect();
      const x = Math.max(0, Math.min(e.clientX - rect.left, rect.width));
      const pct = x / rect.width;
      return Math.round(pct * maxTick);
    },
    [maxTick],
  );

  const handleMouseDown = useCallback(
    (e: React.MouseEvent) => {
      dragging.current = true;
      const tick = getTickFromMouseEvent(e);
      onScrub(tick);

      const handleMouseMove = (ev: MouseEvent) => {
        if (!dragging.current) return;
        const t = getTickFromMouseEvent(ev);
        onScrub(t);
      };

      const handleMouseUp = () => {
        dragging.current = false;
        window.removeEventListener('mousemove', handleMouseMove);
        window.removeEventListener('mouseup', handleMouseUp);
      };

      window.addEventListener('mousemove', handleMouseMove);
      window.addEventListener('mouseup', handleMouseUp);
    },
    [getTickFromMouseEvent, onScrub],
  );

  // Day markers
  const dayMarkers = useMemo(() => {
    const markers: { day: number; pct: number }[] = [];
    for (let d = 1; d <= totalDays; d++) {
      const tick = d * ticksPerDay;
      if (tick <= maxTick) {
        markers.push({ day: d, pct: tickToPercent(tick) });
      }
    }
    return markers;
  }, [totalDays, ticksPerDay, maxTick, tickToPercent]);

  // Filter events for display (show highlights + important types)
  const displayEvents = useMemo(() => {
    return events.filter(
      (e) =>
        e.type === 'conflict' ||
        e.type === 'alliance' ||
        e.type === 'nomination' ||
        e.type === 'eviction' ||
        e.type === 'competition' ||
        e.type === 'betrayal' ||
        highlightSet.has(e.id),
    );
  }, [events, highlightSet]);

  const progressPct = tickToPercent(currentTick);

  return (
    <div className="timeline">
      <div className="timeline-label-row">
        <span>Day 1</span>
        <span>Timeline</span>
        <span>Day {totalDays}</span>
      </div>
      <div
        className="timeline-track-area"
        ref={trackRef}
        onMouseDown={handleMouseDown}
      >
        {/* Base track */}
        <div className="timeline-track">
          <div
            className="timeline-progress"
            style={{ width: `${progressPct}%` }}
          />
        </div>

        {/* Day markers */}
        {dayMarkers.map((m) => (
          <div
            key={m.day}
            className="timeline-day-marker"
            style={{ left: `${m.pct}%` }}
          >
            <span className="timeline-day-label">D{m.day}</span>
          </div>
        ))}

        {/* Event dots */}
        <div className="timeline-event-markers">
          {displayEvents.map((evt) => (
            <div
              key={evt.id}
              className={`timeline-event-dot${highlightSet.has(evt.id) ? ' highlight' : ''}`}
              style={{
                left: `${tickToPercent(evt.tick)}%`,
                backgroundColor: EVENT_TYPE_COLORS[evt.type] ?? '#64748b',
              }}
              title={evt.title}
              onClick={(e) => {
                e.stopPropagation();
                onEventClick(evt.id);
              }}
            />
          ))}
        </div>

        {/* Scrub handle */}
        <div
          className="timeline-scrub-line"
          style={{ left: `${progressPct}%` }}
        />
        <div
          className="timeline-scrub-handle"
          style={{ left: `${progressPct}%` }}
        />
      </div>
    </div>
  );
};
