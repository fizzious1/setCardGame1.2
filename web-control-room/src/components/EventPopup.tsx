import React, { useMemo } from 'react';
import type { GameEvent, Participant } from '../types';

interface Props {
  event: GameEvent;
  participants: Participant[];
  onClose: () => void;
}

const TYPE_STYLES: Record<string, { bg: string; color: string; label: string }> = {
  conflict: { bg: 'rgba(239,68,68,0.15)', color: '#ef4444', label: 'Conflict' },
  alliance: { bg: 'rgba(34,197,94,0.15)', color: '#22c55e', label: 'Alliance' },
  nomination: { bg: 'rgba(234,179,8,0.15)', color: '#eab308', label: 'Nomination' },
  eviction: { bg: 'rgba(168,85,247,0.15)', color: '#a855f7', label: 'Eviction' },
  competition: { bg: 'rgba(6,182,212,0.15)', color: '#06b6d4', label: 'Competition' },
  confession: { bg: 'rgba(249,115,22,0.15)', color: '#f97316', label: 'Confession' },
  conversation: { bg: 'rgba(100,116,139,0.15)', color: '#64748b', label: 'Conversation' },
  betrayal: { bg: 'rgba(220,38,38,0.15)', color: '#dc2626', label: 'Betrayal' },
  social: { bg: 'rgba(139,92,246,0.15)', color: '#8b5cf6', label: 'Social' },
  ceremony: { bg: 'rgba(245,158,11,0.15)', color: '#f59e0b', label: 'Ceremony' },
};

export const EventPopup: React.FC<Props> = ({ event, participants, onClose }) => {
  const pMap = useMemo(
    () => new Map(participants.map((p) => [p.id, p])),
    [participants],
  );

  const typeStyle = TYPE_STYLES[event.type] ?? TYPE_STYLES.social;

  const involvedParticipants = event.participantIds
    .map((id) => pMap.get(id))
    .filter((p): p is Participant => p !== undefined);

  // Mood impacts
  const moodImpacts = event.moodImpact
    ? Object.entries(event.moodImpact).map(([pid, val]) => ({
        name: pMap.get(pid)?.name ?? pid,
        value: val,
      }))
    : [];

  // Trust impacts
  const trustImpacts: { from: string; to: string; value: number }[] = [];
  if (event.trustImpact) {
    for (const [fromId, targets] of Object.entries(event.trustImpact)) {
      for (const [toId, val] of Object.entries(targets)) {
        trustImpacts.push({
          from: pMap.get(fromId)?.name ?? fromId,
          to: pMap.get(toId)?.name ?? toId,
          value: val,
        });
      }
    }
  }

  return (
    <div className="event-popup-overlay" onClick={onClose}>
      <div className="event-popup" onClick={(e) => e.stopPropagation()}>
        <div className="event-popup-header">
          <span
            className="event-popup-type"
            style={{
              backgroundColor: typeStyle.bg,
              color: typeStyle.color,
              border: `1px solid ${typeStyle.color}30`,
            }}
          >
            {typeStyle.label}
          </span>
          <button className="event-popup-close" onClick={onClose}>
            &times;
          </button>
        </div>

        <div className="event-popup-body">
          <div className="event-popup-title">{event.title}</div>

          <div className="event-popup-meta">
            <div className="event-popup-meta-item">
              Day {event.day}, {event.hour}:00
            </div>
            <div className="event-popup-meta-item">
              Tick {event.tick}
            </div>
            {event.room && (
              <div className="event-popup-meta-item">
                {event.room.replace(/_/g, ' ')}
              </div>
            )}
          </div>

          <div className="event-popup-description">{event.description}</div>

          {/* Participants */}
          {involvedParticipants.length > 0 && (
            <div>
              <div
                style={{
                  fontSize: 10,
                  fontWeight: 700,
                  textTransform: 'uppercase',
                  letterSpacing: 1,
                  color: 'var(--text-muted)',
                  marginBottom: 6,
                }}
              >
                Participants
              </div>
              <div className="event-popup-participants">
                {involvedParticipants.map((p) => (
                  <div key={p.id} className="event-popup-participant">
                    <div
                      className="event-popup-participant-dot"
                      style={{ backgroundColor: p.avatarColor }}
                    />
                    {p.name}
                  </div>
                ))}
              </div>
            </div>
          )}

          {/* Mood Impact */}
          {moodImpacts.length > 0 && (
            <div className="event-popup-impacts">
              <div className="event-popup-impact-title">Mood Impact</div>
              {moodImpacts.map((mi, i) => (
                <div key={i} className="event-popup-impact-row">
                  <span>{mi.name}:</span>
                  <span className={mi.value >= 0 ? 'impact-positive' : 'impact-negative'}>
                    {mi.value > 0 ? '+' : ''}
                    {mi.value.toFixed(2)}
                  </span>
                </div>
              ))}
            </div>
          )}

          {/* Trust Impact */}
          {trustImpacts.length > 0 && (
            <div className="event-popup-impacts">
              <div className="event-popup-impact-title">Trust Impact</div>
              {trustImpacts.map((ti, i) => (
                <div key={i} className="event-popup-impact-row">
                  <span>
                    {ti.from} &rarr; {ti.to}:
                  </span>
                  <span className={ti.value >= 0 ? 'impact-positive' : 'impact-negative'}>
                    {ti.value > 0 ? '+' : ''}
                    {ti.value.toFixed(2)}
                  </span>
                </div>
              ))}
            </div>
          )}
        </div>
      </div>
    </div>
  );
};
