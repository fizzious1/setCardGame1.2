import React, { useState, useRef, useEffect, useCallback } from 'react';
import { ActivityEvent, AgentPersona } from '../types';

const FILTERS = [
  { label: 'All', value: 'all' },
  { label: 'Phase 1', value: 'Phase 1' },
  { label: 'Phase 2', value: 'Phase 2' },
  { label: 'Phase 3', value: 'Phase 3' },
  { label: 'Phase 4', value: 'Phase 4' },
  { label: 'Errors Only', value: 'errors' },
];

function phaseBadgeClass(phase: string): string {
  if (phase.includes('1')) return 'phase1';
  if (phase.includes('2')) return 'phase2';
  if (phase.includes('3')) return 'phase3';
  if (phase.includes('4')) return 'phase4';
  return 'phase1';
}

function formatTime(iso: string): string {
  try {
    const d = new Date(iso);
    return d.toLocaleTimeString('en-US', { hour: '2-digit', minute: '2-digit', second: '2-digit', hour12: false });
  } catch {
    return iso;
  }
}

interface Props {
  events: ActivityEvent[];
  agents: AgentPersona[];
}

const ActivityFeed: React.FC<Props> = ({ events, agents }) => {
  const [filter, setFilter] = useState('all');
  const [userScrolled, setUserScrolled] = useState(false);
  const listRef = useRef<HTMLDivElement>(null);
  const prevEventsLen = useRef(events.length);

  const agentNames = React.useMemo(() => {
    const m = new Map<string, string>();
    agents.forEach((a) => m.set(a.id, a.tier1.name.split(' ')[0]));
    return m;
  }, [agents]);

  const filtered = React.useMemo(() => {
    let list = [...events];
    if (filter === 'errors') {
      list = list.filter((e) => e.event_type === 'error');
    } else if (filter !== 'all') {
      list = list.filter((e) => e.phase === filter);
    }
    // Sort newest first
    list.sort((a, b) => new Date(b.timestamp).getTime() - new Date(a.timestamp).getTime());
    return list;
  }, [events, filter]);

  const handleScroll = useCallback(() => {
    const el = listRef.current;
    if (!el) return;
    // If user scrolled away from top, set manual mode
    setUserScrolled(el.scrollTop > 20);
  }, []);

  useEffect(() => {
    // Auto-scroll to top (newest) when new events arrive
    if (!userScrolled && events.length !== prevEventsLen.current) {
      const el = listRef.current;
      if (el) el.scrollTop = 0;
    }
    prevEventsLen.current = events.length;
  }, [events.length, userScrolled]);

  return (
    <div className="card">
      <div className="card-title">Activity Feed</div>

      <div className="activity-filters">
        {FILTERS.map((f) => (
          <button
            key={f.value}
            className={`filter-btn ${filter === f.value ? 'active' : ''}`}
            onClick={() => setFilter(f.value)}
          >
            {f.label}
          </button>
        ))}
      </div>

      <div className="activity-list" ref={listRef} onScroll={handleScroll}>
        {filtered.length === 0 && (
          <div style={{ textAlign: 'center', color: 'var(--text-muted)', padding: 20, fontSize: 12 }}>
            No events matching filter
          </div>
        )}
        {filtered.map((event, i) => (
          <div key={`${event.timestamp}-${i}`} className={`activity-item ${event.event_type}`}>
            <span className="activity-time">{formatTime(event.timestamp)}</span>
            <span className={`activity-phase-badge ${phaseBadgeClass(event.phase)}`}>{event.phase}</span>
            {event.agent_id && (
              <span className="activity-agent">{agentNames.get(event.agent_id) ?? event.agent_id}</span>
            )}
            <span className="activity-message">{event.message}</span>
          </div>
        ))}
      </div>
    </div>
  );
};

export default ActivityFeed;
