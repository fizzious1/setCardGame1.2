import React, { useMemo } from 'react';
import type { RoomLayout, Participant, ParticipantState } from '../types';

interface Props {
  rooms: RoomLayout[];
  participants: Participant[];
  states: ParticipantState[];
  selectedParticipantId: string | null;
  onSelectParticipant: (id: string) => void;
}

// Position participant dots within a room, avoiding overlap
function layoutDotsInRoom(
  room: RoomLayout,
  pIds: string[],
): Map<string, { cx: number; cy: number }> {
  const result = new Map<string, { cx: number; cy: number }>();
  const cols = Math.ceil(Math.sqrt(pIds.length));
  const rows = Math.ceil(pIds.length / cols);
  const padX = 20;
  const padY = 24;
  const availW = room.w - padX * 2;
  const availH = room.h - padY * 2;
  const spacingX = cols > 1 ? availW / (cols - 1) : 0;
  const spacingY = rows > 1 ? availH / (rows - 1) : 0;

  pIds.forEach((id, idx) => {
    const col = idx % cols;
    const row = Math.floor(idx / cols);
    result.set(id, {
      cx: room.x + padX + (cols > 1 ? col * spacingX : availW / 2),
      cy: room.y + padY + (rows > 1 ? row * spacingY : availH / 2),
    });
  });
  return result;
}

export const HouseView: React.FC<Props> = ({
  rooms,
  participants,
  states,
  selectedParticipantId,
  onSelectParticipant,
}) => {
  const participantMap = useMemo(
    () => new Map(participants.map((p) => [p.id, p])),
    [participants],
  );

  // Group states by room
  const roomOccupants = useMemo(() => {
    const map = new Map<string, string[]>();
    for (const st of states) {
      if (st.status === 'evicted') continue;
      const list = map.get(st.room) ?? [];
      list.push(st.participantId);
      map.set(st.room, list);
    }
    return map;
  }, [states]);

  // Calculate positions for all dots
  const dotPositions = useMemo(() => {
    const all = new Map<string, { cx: number; cy: number }>();
    for (const room of rooms) {
      const ids = roomOccupants.get(room.id) ?? [];
      if (ids.length > 0) {
        const positions = layoutDotsInRoom(room, ids);
        for (const [id, pos] of positions) {
          all.set(id, pos);
        }
      }
    }
    return all;
  }, [rooms, roomOccupants]);

  // Compute SVG viewBox from room positions
  const viewBox = useMemo(() => {
    if (rooms.length === 0) return '0 0 800 600';
    let minX = Infinity, minY = Infinity, maxX = -Infinity, maxY = -Infinity;
    for (const r of rooms) {
      minX = Math.min(minX, r.x);
      minY = Math.min(minY, r.y);
      maxX = Math.max(maxX, r.x + r.w);
      maxY = Math.max(maxY, r.y + r.h);
    }
    const pad = 30;
    return `${minX - pad} ${minY - pad} ${maxX - minX + pad * 2} ${maxY - minY + pad * 2}`;
  }, [rooms]);

  // Draw connections between rooms
  const connections = useMemo(() => {
    const lines: { x1: number; y1: number; x2: number; y2: number; key: string }[] = [];
    const seen = new Set<string>();
    for (const room of rooms) {
      for (const connId of room.connections) {
        const key = [room.id, connId].sort().join('-');
        if (seen.has(key)) continue;
        seen.add(key);
        const other = rooms.find((r) => r.id === connId);
        if (!other) continue;
        lines.push({
          x1: room.x + room.w / 2,
          y1: room.y + room.h / 2,
          x2: other.x + other.w / 2,
          y2: other.y + other.h / 2,
          key,
        });
      }
    }
    return lines;
  }, [rooms]);

  return (
    <svg className="house-view" viewBox={viewBox} preserveAspectRatio="xMidYMid meet">
      {/* Connection lines */}
      {connections.map((c) => (
        <line
          key={c.key}
          className="house-connection"
          x1={c.x1}
          y1={c.y1}
          x2={c.x2}
          y2={c.y2}
        />
      ))}

      {/* Rooms */}
      {rooms.map((room) => (
        <g key={room.id} className="house-room">
          <rect
            className="house-room-rect"
            x={room.x}
            y={room.y}
            width={room.w}
            height={room.h}
            fill={room.color}
          />
          <text
            className="house-room-label"
            x={room.x + room.w / 2}
            y={room.y + 14}
            textAnchor="middle"
          >
            {room.name}
          </text>
        </g>
      ))}

      {/* Participant dots */}
      {states
        .filter((st) => st.status !== 'evicted')
        .map((st) => {
          const pos = dotPositions.get(st.participantId);
          const p = participantMap.get(st.participantId);
          if (!pos || !p) return null;
          const isSelected = st.participantId === selectedParticipantId;
          return (
            <g key={st.participantId}>
              <circle
                className={`house-participant-dot${isSelected ? ' selected' : ''}`}
                cx={pos.cx}
                cy={pos.cy}
                r={isSelected ? 9 : 7}
                fill={p.avatarColor}
                stroke={isSelected ? 'white' : 'rgba(0,0,0,0.4)'}
                strokeWidth={isSelected ? 2 : 1}
                onClick={() => onSelectParticipant(st.participantId)}
              />
              <text
                x={pos.cx}
                y={pos.cy + 16}
                textAnchor="middle"
                fill="#94a3b8"
                fontSize="8"
                fontWeight="600"
                pointerEvents="none"
              >
                {p.name.split(' ')[0]}
              </text>
            </g>
          );
        })}
    </svg>
  );
};
