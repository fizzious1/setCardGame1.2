// ─── Shared Data Contract Types ───

export interface Personality {
  extraversion: number;   // 0-1
  agreeableness: number;  // 0-1
  emotionality: number;   // 0-1
  strategy: number;       // 0-1
  loyalty: number;        // 0-1
}

export interface Participant {
  id: string;
  name: string;
  age: number;
  occupation: string;
  bio: string;
  personality: Personality;
  avatarColor: string;
}

export type ParticipantStatus = 'active' | 'nominated' | 'evicted' | 'hoh';

export interface ParticipantState {
  participantId: string;
  mood: number;         // -1 to 1
  energy: number;       // 0 to 1
  stress: number;       // 0 to 1
  room: string;
  status: ParticipantStatus;
  currentGoal: string;
  trust: Record<string, number>; // participantId -> trust level (-1 to 1)
}

export type EventType =
  | 'alliance'
  | 'conflict'
  | 'nomination'
  | 'eviction'
  | 'competition'
  | 'confession'
  | 'conversation'
  | 'betrayal'
  | 'social'
  | 'ceremony';

export interface GameEvent {
  id: string;
  tick: number;
  day: number;
  hour: number;
  type: EventType;
  title: string;
  description: string;
  participantIds: string[];
  room: string;
  moodImpact?: Record<string, number>;
  trustImpact?: Record<string, Record<string, number>>;
  isHighlight?: boolean;
}

export interface Snapshot {
  tick: number;
  day: number;
  hour: number;
  participantStates: ParticipantState[];
}

export interface DailyRecap {
  day: number;
  summary: string;
  keyEventIds: string[];
  moodTrends: Record<string, number[]>; // participantId -> array of mood values through the day
  alliances: [string, string][];          // pairs of participant ids
  dramaScore: number;                      // 1-10
  nominations?: string[];                  // participant ids nominated
  evicted?: string;                        // participant id evicted
  hohWinner?: string;                      // head of household winner
}

export interface HighlightIndex {
  eventId: string;
  tick: number;
  label: string;
  importance: number; // 1-5
}

export interface RoomLayout {
  id: string;
  name: string;
  x: number;
  y: number;
  w: number;
  h: number;
  color: string;
  connections: string[]; // room ids this connects to
}

export interface Season {
  id: string;
  name: string;
  totalDays: number;
  ticksPerDay: number;
  participants: Participant[];
  houseLayout: RoomLayout[];
  snapshots: Snapshot[];
  events: GameEvent[];
  dailyRecaps: DailyRecap[];
  highlights: HighlightIndex[];
}

export interface PlaybackState {
  isPlaying: boolean;
  speed: number;       // 1, 2, 4, 8
  currentTick: number;
  maxTick: number;
}
