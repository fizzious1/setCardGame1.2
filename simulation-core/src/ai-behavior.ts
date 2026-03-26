import {
  Participant,
  ParticipantState,
  HouseLayout,
  GameEvent,
  PersonalityTraits,
} from '../../shared/types';
import { getConnectedRooms } from './house';

// Seeded PRNG (mulberry32)
export class SeededRandom {
  private state: number;

  constructor(seed: number) {
    this.state = seed;
  }

  next(): number {
    this.state |= 0;
    this.state = (this.state + 0x6d2b79f5) | 0;
    let t = Math.imul(this.state ^ (this.state >>> 15), 1 | this.state);
    t = (t + Math.imul(t ^ (t >>> 7), 61 | t)) ^ t;
    return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
  }

  range(min: number, max: number): number {
    return min + this.next() * (max - min);
  }

  int(min: number, max: number): number {
    return Math.floor(this.range(min, max + 1));
  }

  pick<T>(arr: T[]): T {
    return arr[Math.floor(this.next() * arr.length)];
  }

  chance(probability: number): boolean {
    return this.next() < probability;
  }

  shuffle<T>(arr: T[]): T[] {
    const result = [...arr];
    for (let i = result.length - 1; i > 0; i--) {
      const j = Math.floor(this.next() * (i + 1));
      [result[i], result[j]] = [result[j], result[i]];
    }
    return result;
  }
}

export interface AIDecision {
  participantId: string;
  targetRoomId: string;
  socialAction: SocialAction | null;
  moodChange: number;
  energyChange: number;
  newGoal: string | null;
}

export interface SocialAction {
  type:
    | 'form_alliance'
    | 'break_alliance'
    | 'start_conflict'
    | 'reconcile'
    | 'strategize'
    | 'confess'
    | 'chat'
    | 'betray'
    | 'comfort'
    | 'avoid';
  targetParticipantId?: string;
  description: string;
}

export interface AllianceTracker {
  alliances: Map<string, Set<string>>; // participantId -> set of allied participantIds
  conflicts: Map<string, Set<string>>; // participantId -> set of conflicting participantIds
  betrayals: Set<string>; // "betrayerId->victimId" keys
}

export function createAllianceTracker(): AllianceTracker {
  return {
    alliances: new Map(),
    conflicts: new Map(),
    betrayals: new Set(),
  };
}

function getAllies(tracker: AllianceTracker, pid: string): Set<string> {
  if (!tracker.alliances.has(pid)) {
    tracker.alliances.set(pid, new Set());
  }
  return tracker.alliances.get(pid)!;
}

function getConflicts(tracker: AllianceTracker, pid: string): Set<string> {
  if (!tracker.conflicts.has(pid)) {
    tracker.conflicts.set(pid, new Set());
  }
  return tracker.conflicts.get(pid)!;
}

export function addAlliance(tracker: AllianceTracker, p1: string, p2: string): void {
  getAllies(tracker, p1).add(p2);
  getAllies(tracker, p2).add(p1);
  // Remove from conflicts if present
  getConflicts(tracker, p1).delete(p2);
  getConflicts(tracker, p2).delete(p1);
}

export function removeAlliance(tracker: AllianceTracker, p1: string, p2: string): void {
  getAllies(tracker, p1).delete(p2);
  getAllies(tracker, p2).delete(p1);
}

export function addConflict(tracker: AllianceTracker, p1: string, p2: string): void {
  getConflicts(tracker, p1).add(p2);
  getConflicts(tracker, p2).add(p1);
}

export function removeConflict(tracker: AllianceTracker, p1: string, p2: string): void {
  getConflicts(tracker, p1).delete(p2);
  getConflicts(tracker, p2).delete(p1);
}

export function areAllies(tracker: AllianceTracker, p1: string, p2: string): boolean {
  return getAllies(tracker, p1).has(p2);
}

export function areInConflict(tracker: AllianceTracker, p1: string, p2: string): boolean {
  return getConflicts(tracker, p1).has(p2);
}

function clamp(value: number, min: number, max: number): number {
  return Math.max(min, Math.min(max, value));
}

// Determine what goal a participant should have based on game state
function determineGoal(
  participant: Participant,
  state: ParticipantState,
  day: number,
  hour: number,
  tracker: AllianceTracker,
  allStates: ParticipantState[],
  nominations: string[],
  rng: SeededRandom
): string {
  const p = participant.personality;

  // Evicted participants have no goals
  if (state.status === 'evicted') return 'watch from home';

  // If nominated, top priority is survival
  if (state.status === 'nominated') {
    if (p.strategy > 0.6) return 'campaign to stay';
    if (p.extraversion > 0.7) return 'rally support';
    return 'plead my case';
  }

  // Night hours (0-6): sleep
  if (hour >= 0 && hour < 7) return 'sleep';

  // Nomination days: strategic focus
  if (day === 3 || day === 6) {
    if (hour >= 14 && hour < 20) {
      if (p.strategy > 0.6) return 'plan nomination strategy';
      return 'worry about nominations';
    }
  }

  // Competition days
  if (day === 2 || day === 5) {
    if (hour >= 12 && hour < 16) return 'prepare for competition';
  }

  // Low on allies? Build some
  const allyCount = getAllies(tracker, participant.id).size;
  if (allyCount < 1 && p.strategy > 0.5) return 'find an alliance partner';

  // Has conflicts? Decide whether to resolve or escalate
  const conflictCount = getConflicts(tracker, participant.id).size;
  if (conflictCount > 0 && p.agreeableness > 0.6) return 'mend a broken relationship';

  // Low mood? Seek comfort
  if (state.mood < -0.3) {
    if (p.emotionality > 0.5) return 'vent in confessional';
    return 'take some alone time';
  }

  // Default goals based on personality
  const goals: string[] = [];
  if (p.extraversion > 0.7) goals.push('socialize with everyone', 'be the center of attention');
  if (p.strategy > 0.7) goals.push('gather information', 'strengthen my position');
  if (p.agreeableness > 0.7) goals.push('keep the peace', 'check on everyone');
  if (p.emotionality > 0.7) goals.push('express how I feel', 'connect on a deeper level');
  if (p.loyalty > 0.7) goals.push('protect my allies', 'stay true to my word');

  if (goals.length === 0) goals.push('lay low', 'observe others', 'enjoy the day');

  return rng.pick(goals);
}

// Decide which room to move to
function decideRoom(
  participant: Participant,
  currentState: ParticipantState,
  house: HouseLayout,
  allStates: ParticipantState[],
  day: number,
  hour: number,
  tracker: AllianceTracker,
  rng: SeededRandom
): string {
  if (currentState.status === 'evicted') return 'hallway';

  const p = participant.personality;

  // Night: go to bedroom
  if (hour >= 0 && hour < 7) {
    if (currentState.roomId === 'bedroom1' || currentState.roomId === 'bedroom2') {
      return currentState.roomId;
    }
    return rng.chance(0.5) ? 'bedroom1' : 'bedroom2';
  }

  // Confessional urge
  if (p.emotionality > 0.6 && currentState.mood < -0.2 && rng.chance(0.15)) {
    return 'confessional';
  }

  // Strategic people occasionally visit confessional
  if (p.strategy > 0.7 && rng.chance(0.08)) {
    return 'confessional';
  }

  // Stay in current room sometimes (inertia)
  if (rng.chance(0.4)) {
    return currentState.roomId;
  }

  // Move toward allies
  const allies = getAllies(tracker, participant.id);
  if (allies.size > 0 && rng.chance(0.3 + p.loyalty * 0.2)) {
    for (const s of allStates) {
      if (allies.has(s.participantId) && s.status !== 'evicted') {
        return s.roomId;
      }
    }
  }

  // Avoid people in conflict
  const conflictPeople = getConflicts(tracker, participant.id);
  if (conflictPeople.size > 0 && rng.chance(0.3)) {
    const conflictRooms = new Set<string>();
    for (const s of allStates) {
      if (conflictPeople.has(s.participantId)) {
        conflictRooms.add(s.roomId);
      }
    }
    const connected = getConnectedRooms(house, currentState.roomId);
    const safeRooms = connected.filter((r) => !conflictRooms.has(r));
    if (safeRooms.length > 0) return rng.pick(safeRooms);
  }

  // Extroverts go to populated rooms
  if (p.extraversion > 0.6 && rng.chance(0.4)) {
    const roomPop: Record<string, number> = {};
    for (const s of allStates) {
      if (s.status !== 'evicted') {
        roomPop[s.roomId] = (roomPop[s.roomId] || 0) + 1;
      }
    }
    let bestRoom = currentState.roomId;
    let bestPop = 0;
    for (const [rid, pop] of Object.entries(roomPop)) {
      if (pop > bestPop && rid !== 'confessional' && rid !== 'nomination_room') {
        bestPop = pop;
        bestRoom = rid;
      }
    }
    return bestRoom;
  }

  // Introverts go to empty rooms
  if (p.extraversion < 0.4 && rng.chance(0.3)) {
    const roomPop: Record<string, number> = {};
    for (const s of allStates) {
      if (s.status !== 'evicted') {
        roomPop[s.roomId] = (roomPop[s.roomId] || 0) + 1;
      }
    }
    const connected = getConnectedRooms(house, currentState.roomId);
    const quietRooms = connected.filter(
      (r) => (roomPop[r] || 0) < 2 && r !== 'nomination_room' && r !== 'competition_arena'
    );
    if (quietRooms.length > 0) return rng.pick(quietRooms);
  }

  // Random adjacent room
  const connected = getConnectedRooms(house, currentState.roomId);
  const validRooms = connected.filter(
    (r) => r !== 'nomination_room' && r !== 'competition_arena'
  );
  if (validRooms.length > 0) return rng.pick(validRooms);

  return currentState.roomId;
}

// Decide a social action this tick
function decideSocialAction(
  participant: Participant,
  currentState: ParticipantState,
  allStates: ParticipantState[],
  allParticipants: Participant[],
  day: number,
  hour: number,
  tracker: AllianceTracker,
  rng: SeededRandom
): SocialAction | null {
  if (currentState.status === 'evicted') return null;

  // No actions while sleeping
  if (hour >= 0 && hour < 7) return null;

  const p = participant.personality;
  const roommates = allStates.filter(
    (s) =>
      s.roomId === currentState.roomId &&
      s.participantId !== participant.id &&
      s.status !== 'evicted'
  );

  // Alone in room
  if (roommates.length === 0) {
    if (currentState.roomId === 'confessional' && rng.chance(0.7)) {
      return {
        type: 'confess',
        description: generateConfession(participant, currentState, tracker, rng),
      };
    }
    return null;
  }

  const target = rng.pick(roommates);
  const targetParticipant = allParticipants.find((pp) => pp.id === target.participantId)!;
  const trustLevel = currentState.trust[target.participantId] || 0;
  const isAlly = areAllies(tracker, participant.id, target.participantId);
  const inConflict = areInConflict(tracker, participant.id, target.participantId);

  // Alliance formation: strategic players with moderate-high trust who aren't already allies
  if (
    !isAlly &&
    !inConflict &&
    trustLevel > 0.2 &&
    p.strategy > 0.5 &&
    rng.chance(0.08 + p.strategy * 0.05)
  ) {
    return {
      type: 'form_alliance',
      targetParticipantId: target.participantId,
      description: `${participant.name} proposes a secret alliance to ${targetParticipant.name}, suggesting they watch each other's backs.`,
    };
  }

  // Betrayal: low loyalty ally in a strategic moment
  if (
    isAlly &&
    p.loyalty < 0.5 &&
    p.strategy > 0.6 &&
    (day >= 3) &&
    rng.chance(0.04)
  ) {
    return {
      type: 'betray',
      targetParticipantId: target.participantId,
      description: `${participant.name} secretly plans to turn against ${targetParticipant.name}, their supposed ally, to gain a strategic advantage.`,
    };
  }

  // Conflict: low agreeableness or existing tension
  if (
    (p.agreeableness < 0.4 || inConflict || trustLevel < -0.3) &&
    rng.chance(0.06 + (1 - p.agreeableness) * 0.05)
  ) {
    return {
      type: 'start_conflict',
      targetParticipantId: target.participantId,
      description: generateConflictDescription(participant, targetParticipant, rng),
    };
  }

  // Reconciliation: agreeable people try to fix conflicts
  if (inConflict && p.agreeableness > 0.5 && rng.chance(0.1)) {
    return {
      type: 'reconcile',
      targetParticipantId: target.participantId,
      description: `${participant.name} approaches ${targetParticipant.name} to clear the air and patch things up.`,
    };
  }

  // Strategy meeting with ally
  if (isAlly && p.strategy > 0.5 && rng.chance(0.1)) {
    return {
      type: 'strategize',
      targetParticipantId: target.participantId,
      description: `${participant.name} and ${targetParticipant.name} huddle together to discuss their game plan and share information about the other houseguests.`,
    };
  }

  // Comfort someone who is sad
  if (target.mood < -0.3 && p.agreeableness > 0.5 && rng.chance(0.15)) {
    return {
      type: 'comfort',
      targetParticipantId: target.participantId,
      description: `${participant.name} notices ${targetParticipant.name} is down and sits with them to offer support and encouragement.`,
    };
  }

  // General chat
  if (rng.chance(0.12 + p.extraversion * 0.1)) {
    return {
      type: 'chat',
      targetParticipantId: target.participantId,
      description: generateChatDescription(participant, targetParticipant, rng),
    };
  }

  return null;
}

function generateConfession(
  participant: Participant,
  state: ParticipantState,
  tracker: AllianceTracker,
  rng: SeededRandom
): string {
  const p = participant.personality;
  const confessions = [
    `${participant.name} sits in the confessional and admits they're feeling the pressure of the game.`,
    `${participant.name} reveals their true feelings about the house dynamics and who they trust.`,
    `${participant.name} confesses that their strategy might need to change if they want to survive.`,
    `${participant.name} breaks down about how much they miss home but says they're not giving up.`,
    `${participant.name} shares their secret observations about who's really running the house.`,
  ];

  if (state.mood < -0.3) {
    confessions.push(
      `${participant.name} gets emotional in the confessional, feeling isolated and unsure who to trust.`
    );
  }
  if (p.strategy > 0.7) {
    confessions.push(
      `${participant.name} lays out their master plan in the confessional, detailing exactly who needs to go next.`
    );
  }

  return rng.pick(confessions);
}

function generateConflictDescription(
  p1: Participant,
  p2: Participant,
  rng: SeededRandom
): string {
  const conflicts = [
    `${p1.name} and ${p2.name} get into a heated argument about who's really pulling the strings in the house.`,
    `Tensions boil over between ${p1.name} and ${p2.name} over perceived disloyalty and broken promises.`,
    `${p1.name} confronts ${p2.name} about talking behind their back, and the conversation quickly escalates.`,
    `${p1.name} accuses ${p2.name} of playing both sides, leading to a loud and dramatic showdown.`,
    `A disagreement about chores spirals into a full-blown argument between ${p1.name} and ${p2.name}, revealing deeper tensions.`,
  ];
  return rng.pick(conflicts);
}

function generateChatDescription(
  p1: Participant,
  p2: Participant,
  rng: SeededRandom
): string {
  const chats = [
    `${p1.name} and ${p2.name} share a laugh over breakfast and bond over their similar backgrounds.`,
    `${p1.name} and ${p2.name} have a deep conversation about life outside the house.`,
    `${p1.name} and ${p2.name} chat casually while lounging, carefully gauging each other's true intentions.`,
    `${p1.name} and ${p2.name} swap stories about their families and why they came on the show.`,
    `${p1.name} and ${p2.name} gossip about the other houseguests while trying to read each other's loyalties.`,
  ];
  return rng.pick(chats);
}

// Compute mood change based on interactions and environment
function computeMoodChange(
  participant: Participant,
  currentState: ParticipantState,
  action: SocialAction | null,
  roommates: ParticipantState[],
  hour: number,
  rng: SeededRandom
): number {
  let delta = 0;
  const p = participant.personality;

  // Base decay toward neutral
  if (currentState.mood > 0.1) delta -= 0.02;
  if (currentState.mood < -0.1) delta += 0.02;

  // Social interaction effects
  if (action) {
    switch (action.type) {
      case 'form_alliance':
        delta += 0.15;
        break;
      case 'start_conflict':
        delta -= 0.2 * p.emotionality;
        break;
      case 'betray':
        delta -= 0.1 * p.emotionality;
        delta += 0.05 * p.strategy; // strategic satisfaction
        break;
      case 'reconcile':
        delta += 0.1;
        break;
      case 'confess':
        delta += 0.05; // cathartic
        break;
      case 'chat':
        delta += 0.05 * p.extraversion;
        break;
      case 'comfort':
        delta += 0.08;
        break;
      case 'strategize':
        delta += 0.03;
        break;
      case 'break_alliance':
        delta -= 0.15 * p.emotionality;
        break;
      case 'avoid':
        break;
    }
  }

  // Being around many people: extroverts like it, introverts don't
  if (roommates.length > 3) {
    delta += (p.extraversion - 0.5) * 0.05;
  }
  if (roommates.length === 0 && p.extraversion < 0.4) {
    delta += 0.03; // introverts enjoy solitude
  }

  // Random fluctuation
  delta += rng.range(-0.02, 0.02);

  return delta;
}

// Compute energy change
function computeEnergyChange(hour: number, action: SocialAction | null): number {
  // Sleeping hours: recover energy
  if (hour >= 0 && hour < 7) return 0.08;

  // Base drain
  let delta = -0.02;

  // Actions cost extra energy
  if (action) {
    switch (action.type) {
      case 'start_conflict':
        delta -= 0.05;
        break;
      case 'strategize':
        delta -= 0.03;
        break;
      case 'form_alliance':
        delta -= 0.02;
        break;
      default:
        delta -= 0.01;
        break;
    }
  }

  return delta;
}

// Compute trust changes from a social action
function computeTrustChanges(
  participant: Participant,
  action: SocialAction | null,
  rng: SeededRandom
): Record<string, number> {
  const deltas: Record<string, number> = {};
  if (!action || !action.targetParticipantId) return deltas;

  const targetId = action.targetParticipantId;

  switch (action.type) {
    case 'form_alliance':
      deltas[targetId] = 0.2;
      break;
    case 'start_conflict':
      deltas[targetId] = -0.25;
      break;
    case 'betray':
      deltas[targetId] = -0.4;
      break;
    case 'reconcile':
      deltas[targetId] = 0.15;
      break;
    case 'chat':
      deltas[targetId] = 0.05 + rng.range(0, 0.05);
      break;
    case 'comfort':
      deltas[targetId] = 0.1;
      break;
    case 'strategize':
      deltas[targetId] = 0.08;
      break;
    case 'break_alliance':
      deltas[targetId] = -0.3;
      break;
    case 'confess':
      break;
    case 'avoid':
      deltas[targetId] = -0.05;
      break;
  }

  return deltas;
}

// Main AI decision function for a single participant in a single tick
export function makeDecision(
  participant: Participant,
  currentState: ParticipantState,
  allStates: ParticipantState[],
  allParticipants: Participant[],
  house: HouseLayout,
  day: number,
  hour: number,
  tracker: AllianceTracker,
  nominations: string[],
  rng: SeededRandom
): AIDecision {
  // Decide room
  const targetRoomId = decideRoom(
    participant,
    currentState,
    house,
    allStates,
    day,
    hour,
    tracker,
    rng
  );

  // Decide social action
  const socialAction = decideSocialAction(
    participant,
    { ...currentState, roomId: targetRoomId },
    allStates,
    allParticipants,
    day,
    hour,
    tracker,
    rng
  );

  // Compute mood and energy changes
  const roommates = allStates.filter(
    (s) =>
      s.roomId === targetRoomId &&
      s.participantId !== participant.id &&
      s.status !== 'evicted'
  );

  const moodChange = computeMoodChange(participant, currentState, socialAction, roommates, hour, rng);
  const energyChange = computeEnergyChange(hour, socialAction);

  // Determine new goal
  const newGoal = determineGoal(
    participant,
    currentState,
    day,
    hour,
    tracker,
    allStates,
    nominations,
    rng
  );

  return {
    participantId: participant.id,
    targetRoomId,
    socialAction,
    moodChange,
    energyChange,
    newGoal,
  };
}

// Apply an AI decision to update the participant state
export function applyDecision(
  state: ParticipantState,
  decision: AIDecision,
  rng: SeededRandom
): ParticipantState {
  const trustChanges = computeTrustChanges(
    {} as Participant, // not needed for trust computation
    decision.socialAction,
    rng
  );

  const newTrust = { ...state.trust };
  for (const [pid, delta] of Object.entries(trustChanges)) {
    newTrust[pid] = clamp((newTrust[pid] || 0) + delta, -1, 1);
  }

  const actionString = decision.socialAction
    ? socialActionToAnimationHint(decision.socialAction.type)
    : undefined;

  return {
    participantId: state.participantId,
    roomId: decision.targetRoomId,
    mood: clamp(state.mood + decision.moodChange, -1, 1),
    energy: clamp(state.energy + decision.energyChange, 0, 1),
    trust: newTrust,
    currentGoal: decision.newGoal || state.currentGoal,
    status: state.status,
    socialAction: actionString,
    positionInRoom: {
      x: rng.range(0.1, 0.9),
      y: rng.range(0.1, 0.9),
    },
  };
}

function socialActionToAnimationHint(
  type: SocialAction['type']
): string | undefined {
  switch (type) {
    case 'form_alliance':
      return 'handshake';
    case 'start_conflict':
      return 'argument';
    case 'strategize':
      return 'whisper';
    case 'reconcile':
      return 'handshake';
    case 'confess':
      return 'cry';
    case 'comfort':
      return 'handshake';
    case 'betray':
      return 'whisper';
    case 'chat':
      return 'idle_talk';
    default:
      return undefined;
  }
}

// Update the alliance tracker based on social actions
export function updateTracker(
  tracker: AllianceTracker,
  decision: AIDecision
): void {
  if (!decision.socialAction) return;

  const action = decision.socialAction;
  const pid = decision.participantId;
  const tid = action.targetParticipantId;

  if (!tid) return;

  switch (action.type) {
    case 'form_alliance':
      addAlliance(tracker, pid, tid);
      break;
    case 'break_alliance':
      removeAlliance(tracker, pid, tid);
      break;
    case 'start_conflict':
      addConflict(tracker, pid, tid);
      break;
    case 'reconcile':
      removeConflict(tracker, pid, tid);
      break;
    case 'betray':
      tracker.betrayals.add(`${pid}->${tid}`);
      removeAlliance(tracker, pid, tid);
      addConflict(tracker, pid, tid);
      break;
  }
}
