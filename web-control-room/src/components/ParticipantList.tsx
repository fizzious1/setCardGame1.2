import React from 'react';
import type { Participant, ParticipantState } from '../types';

interface Props {
  participants: Participant[];
  states: ParticipantState[];
  selectedId: string | null;
  onSelect: (id: string) => void;
}

export const ParticipantList: React.FC<Props> = ({
  participants,
  states,
  selectedId,
  onSelect,
}) => {
  const stateMap = new Map(states.map((s) => [s.participantId, s]));

  return (
    <div className="left-sidebar">
      <div className="sidebar-header">Houseguests</div>
      <div className="participant-list">
        {participants.map((p) => {
          const st = stateMap.get(p.id);
          const status = st?.status ?? 'active';
          const mood = st?.mood ?? 0;
          const energy = st?.energy ?? 0.5;
          const isEvicted = status === 'evicted';
          const initials = p.name
            .split(' ')
            .map((w) => w[0])
            .join('');

          return (
            <div
              key={p.id}
              className={`participant-card${selectedId === p.id ? ' selected' : ''}${isEvicted ? ' evicted' : ''}`}
              onClick={() => onSelect(p.id)}
            >
              <div
                className="participant-avatar"
                style={{ backgroundColor: p.avatarColor }}
              >
                {initials}
                <div
                  className={`status-ring${status === 'hoh' ? ' hoh' : ''}${status === 'nominated' ? ' nominated' : ''}`}
                />
              </div>
              <div className="participant-info">
                <div className="participant-name">{p.name}</div>
                <div className={`participant-status ${status}`}>
                  {status === 'hoh' ? 'Head of Household' : status}
                </div>
              </div>
              <div className="participant-bars">
                <div className="mini-bar" title={`Mood: ${Math.round(mood * 100)}%`}>
                  <div
                    className={`mini-bar-fill mood${mood < 0 ? ' negative' : ''}`}
                    style={{ width: `${Math.abs(mood) * 100}%` }}
                  />
                </div>
                <div className="mini-bar" title={`Energy: ${Math.round(energy * 100)}%`}>
                  <div
                    className="mini-bar-fill energy"
                    style={{ width: `${energy * 100}%` }}
                  />
                </div>
              </div>
            </div>
          );
        })}
      </div>
    </div>
  );
};
