// Every type both the web app and desktop app consume

export interface Season {
  id: string;
  name: string;
  participants: Participant[];
  house: HouseLayout;
  totalDays: number;
  currentTick: number;
  ticksPerDay: number; // 24 ticks = 24 hours
  snapshots: Snapshot[];
  events: GameEvent[];
  dailyRecaps: DailyRecap[];
  highlights: HighlightIndex[];
}

export interface Participant {
  id: string;
  name: string;
  age: number;
  bio: string;
  personality: PersonalityTraits;
  avatarColor: string;
}

export interface PersonalityTraits {
  extraversion: number;    // 0-1
  agreeableness: number;   // 0-1
  strategy: number;        // 0-1
  emotionality: number;    // 0-1
  loyalty: number;         // 0-1
}

export interface HouseLayout {
  rooms: Room[];
  connections: RoomConnection[];
}

export interface Room {
  id: string;
  name: string;
  type: 'bedroom' | 'kitchen' | 'living_room' | 'garden' | 'confessional' | 'nomination_room' | 'competition_arena' | 'hallway';
  x: number;
  y: number;
  width: number;
  height: number;
  capacity: number;
}

export interface RoomConnection {
  from: string;
  to: string;
}

export interface Snapshot {
  tick: number;
  day: number;
  hour: number;
  participantStates: ParticipantState[];
}

export interface ParticipantState {
  participantId: string;
  roomId: string;
  mood: number;           // -1 to 1
  energy: number;         // 0 to 1
  trust: Record<string, number>;  // participantId -> trust level (-1 to 1)
  currentGoal: string;
  status: 'active' | 'nominated' | 'evicted' | 'winner';
  socialAction?: string;  // current animation/action hint for Unreal
  positionInRoom: { x: number; y: number }; // normalized 0-1 within room
}

export type EventType =
  | 'alliance_formed'
  | 'alliance_broken'
  | 'conflict'
  | 'nomination'
  | 'eviction'
  | 'competition_won'
  | 'confession'
  | 'conversation'
  | 'betrayal'
  | 'reconciliation'
  | 'strategy_meeting'
  | 'day_start'
  | 'day_end';

export interface GameEvent {
  id: string;
  tick: number;
  day: number;
  hour: number;
  type: EventType;
  participantIds: string[];
  roomId: string;
  title: string;
  description: string;
  impact: EventImpact[];
  isHighlight: boolean;
  animationHint?: string; // for Unreal: 'handshake' | 'argument' | 'whisper' | 'celebrate' | 'cry'
}

export interface EventImpact {
  participantId: string;
  moodDelta: number;
  trustDeltas: Record<string, number>;
  newGoal?: string;
}

export interface DailyRecap {
  day: number;
  summary: string;
  keyEventIds: string[];
  participantMoodSummary: Record<string, number>;
  nominations: string[];
  evictedId?: string;
  allianceMap: Record<string, string[]>;
  dramaScore: number; // 1-10
}

export interface HighlightIndex {
  eventId: string;
  tick: number;
  day: number;
  label: string;
  importance: number; // 1-10
  category: 'drama' | 'strategy' | 'social' | 'competition' | 'eviction';
}

export interface PlaybackState {
  isPlaying: boolean;
  speed: number;        // 1, 2, 4, 8
  currentTick: number;
  maxTick: number;
}

export interface CameraMode {
  type: 'director' | 'follow' | 'room' | 'overview';
  targetId?: string;    // participant or room ID
}
