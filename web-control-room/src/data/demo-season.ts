import rawData from '../../../shared/demo-season.json';
import type {
  Season,
  Participant,
  Snapshot,
  GameEvent,
  DailyRecap,
  HighlightIndex,
  RoomLayout,
  ParticipantState,
  EventType,
} from '../types';

// The JSON shape differs slightly from our Season type.
// We adapt it here so the rest of the app only sees typed data.

const raw = rawData as any;

function adaptParticipant(p: any): Participant {
  return {
    id: p.id,
    name: p.name,
    age: p.age,
    occupation: p.occupation ?? '',
    bio: p.bio,
    personality: p.personality,
    avatarColor: p.avatarColor,
  };
}

function adaptParticipantState(ps: any): ParticipantState {
  return {
    participantId: ps.participantId,
    mood: ps.mood,
    energy: ps.energy,
    stress: ps.stress ?? 0,
    room: ps.roomId ?? ps.room,
    status: ps.status,
    currentGoal: ps.currentGoal,
    trust: ps.trust ?? {},
  };
}

function adaptSnapshot(s: any): Snapshot {
  return {
    tick: s.tick,
    day: s.day,
    hour: s.hour,
    participantStates: s.participantStates.map(adaptParticipantState),
  };
}

const eventTypeMap: Record<string, EventType> = {
  alliance_formed: 'alliance',
  conflict: 'conflict',
  nomination: 'nomination',
  eviction: 'eviction',
  competition_won: 'competition',
  confession: 'confession',
  conversation: 'conversation',
  betrayal: 'betrayal',
  reconciliation: 'social',
  strategy_meeting: 'social',
  day_start: 'ceremony',
  day_end: 'ceremony',
};

function adaptEvent(e: any): GameEvent {
  return {
    id: e.id,
    tick: e.tick,
    day: e.day,
    hour: e.hour,
    type: eventTypeMap[e.type] ?? 'social',
    title: e.title,
    description: e.description,
    participantIds: e.participantIds,
    room: e.roomId ?? e.room ?? '',
    moodImpact: e.moodImpact,
    trustImpact: e.trustImpact,
    isHighlight: e.isHighlight ?? false,
  };
}

function adaptRecap(r: any): DailyRecap {
  const moodTrends: Record<string, number[]> = {};
  if (r.participantMoodSummary) {
    for (const [pid, val] of Object.entries(r.participantMoodSummary)) {
      moodTrends[pid] = [val as number];
    }
  } else if (r.moodTrends) {
    Object.assign(moodTrends, r.moodTrends);
  }

  let alliances: [string, string][] = [];
  if (r.allianceMap && typeof r.allianceMap === 'object') {
    for (const [pid, targets] of Object.entries(r.allianceMap)) {
      if (Array.isArray(targets)) {
        for (const t of targets) {
          alliances.push([pid, t as string]);
        }
      }
    }
  } else if (Array.isArray(r.alliances)) {
    alliances = r.alliances;
  }

  return {
    day: r.day,
    summary: r.summary,
    keyEventIds: r.keyEventIds,
    moodTrends,
    alliances,
    dramaScore: r.dramaScore,
    nominations: r.nominations,
    evicted: r.evicted,
    hohWinner: r.hohWinner,
  };
}

function adaptRoom(r: any): RoomLayout {
  return {
    id: r.id,
    name: r.name,
    x: r.x,
    y: r.y,
    w: r.width ?? r.w,
    h: r.height ?? r.h,
    color: r.color ?? roomColor(r.type ?? r.id),
    connections: [],
  };
}

function roomColor(type: string): string {
  const map: Record<string, string> = {
    living_room: '#1e3a5f',
    kitchen: '#3b2f1e',
    bedroom: '#2d1b4e',
    garden: '#1a3c2a',
    confessional: '#4a1a2e',
    nomination_room: '#4a3a0e',
    competition_arena: '#0e3a4a',
    hallway: '#2a2a3a',
  };
  return map[type] ?? '#2a2a3a';
}

const rooms = raw.house.rooms.map(adaptRoom);
// Populate connections
if (raw.house.connections) {
  for (const conn of raw.house.connections) {
    const fromRoom = rooms.find((r: RoomLayout) => r.id === conn.from);
    const toRoom = rooms.find((r: RoomLayout) => r.id === conn.to);
    if (fromRoom && !fromRoom.connections.includes(conn.to)) {
      fromRoom.connections.push(conn.to);
    }
    if (toRoom && !toRoom.connections.includes(conn.from)) {
      toRoom.connections.push(conn.from);
    }
  }
}

export const demoSeason: Season = {
  id: raw.id,
  name: raw.name,
  totalDays: raw.totalDays,
  ticksPerDay: raw.ticksPerDay,
  participants: raw.participants.map(adaptParticipant),
  houseLayout: rooms,
  snapshots: raw.snapshots.map(adaptSnapshot),
  events: raw.events.map(adaptEvent),
  dailyRecaps: raw.dailyRecaps.map(adaptRecap),
  highlights: raw.highlights.map((h: any): HighlightIndex => ({
    eventId: h.eventId,
    tick: h.tick,
    label: h.label,
    importance: h.importance,
  })),
};
