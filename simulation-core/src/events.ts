import {
  GameEvent,
  EventImpact,
  EventType,
  Participant,
  ParticipantState,
} from '../../shared/types';
import {
  AIDecision,
  AllianceTracker,
  SeededRandom,
  areAllies,
  areInConflict,
} from './ai-behavior';

let eventCounter = 0;

export function resetEventCounter(): void {
  eventCounter = 0;
}

function nextEventId(): string {
  eventCounter++;
  return `evt_${String(eventCounter).padStart(4, '0')}`;
}

// Convert AI decisions into game events for a single tick
export function generateEventsFromDecisions(
  decisions: AIDecision[],
  participants: Participant[],
  states: ParticipantState[],
  tick: number,
  day: number,
  hour: number,
  tracker: AllianceTracker,
  rng: SeededRandom
): GameEvent[] {
  const events: GameEvent[] = [];
  const pMap = new Map(participants.map((p) => [p.id, p]));
  const sMap = new Map(states.map((s) => [s.participantId, s]));

  for (const decision of decisions) {
    if (!decision.socialAction) continue;
    const action = decision.socialAction;
    const participant = pMap.get(decision.participantId)!;
    const state = sMap.get(decision.participantId)!;

    const targetId = action.targetParticipantId;
    const targetParticipant = targetId ? pMap.get(targetId) : undefined;

    let eventType: EventType;
    let title: string;
    let isHighlight = false;
    let animationHint: string | undefined;

    switch (action.type) {
      case 'form_alliance':
        eventType = 'alliance_formed';
        title = `Alliance: ${participant.name} & ${targetParticipant?.name}`;
        isHighlight = true;
        animationHint = 'handshake';
        break;
      case 'break_alliance':
        eventType = 'alliance_broken';
        title = `Alliance Broken: ${participant.name} cuts ties with ${targetParticipant?.name}`;
        isHighlight = true;
        animationHint = 'argument';
        break;
      case 'start_conflict':
        eventType = 'conflict';
        title = `Conflict: ${participant.name} vs ${targetParticipant?.name}`;
        isHighlight = true;
        animationHint = 'argument';
        break;
      case 'reconcile':
        eventType = 'reconciliation';
        title = `Reconciliation: ${participant.name} & ${targetParticipant?.name} make peace`;
        isHighlight = false;
        animationHint = 'handshake';
        break;
      case 'betray':
        eventType = 'betrayal';
        title = `Betrayal! ${participant.name} turns on ${targetParticipant?.name}`;
        isHighlight = true;
        animationHint = 'whisper';
        break;
      case 'strategize':
        eventType = 'strategy_meeting';
        title = `Strategy Session: ${participant.name} & ${targetParticipant?.name}`;
        isHighlight = false;
        animationHint = 'whisper';
        break;
      case 'confess':
        eventType = 'confession';
        title = `Confessional: ${participant.name}`;
        isHighlight = rng.chance(0.3);
        animationHint = 'cry';
        break;
      case 'chat':
        eventType = 'conversation';
        title = `Chat: ${participant.name} & ${targetParticipant?.name}`;
        isHighlight = false;
        animationHint = undefined;
        break;
      case 'comfort':
        eventType = 'conversation';
        title = `${participant.name} comforts ${targetParticipant?.name}`;
        isHighlight = false;
        animationHint = 'handshake';
        break;
      case 'avoid':
        continue; // No event for avoidance
      default:
        continue;
    }

    const participantIds = [decision.participantId];
    if (targetId) participantIds.push(targetId);

    const impact: EventImpact[] = [];

    // Impact on the acting participant
    impact.push({
      participantId: decision.participantId,
      moodDelta: decision.moodChange,
      trustDeltas: targetId
        ? { [targetId]: computeTrustDelta(action.type) }
        : {},
      newGoal: decision.newGoal || undefined,
    });

    // Impact on the target participant (reciprocal)
    if (targetId) {
      const targetState = sMap.get(targetId);
      if (targetState) {
        const reciprocalMood = computeReciprocalMood(action.type, targetParticipant!);
        const reciprocalTrust = computeReciprocalTrust(action.type);
        impact.push({
          participantId: targetId,
          moodDelta: reciprocalMood,
          trustDeltas: { [decision.participantId]: reciprocalTrust },
        });
      }
    }

    events.push({
      id: nextEventId(),
      tick,
      day,
      hour,
      type: eventType,
      participantIds,
      roomId: decision.targetRoomId,
      title,
      description: action.description,
      impact,
      isHighlight,
      animationHint,
    });
  }

  return events;
}

function computeTrustDelta(actionType: string): number {
  switch (actionType) {
    case 'form_alliance':
      return 0.2;
    case 'start_conflict':
      return -0.25;
    case 'betray':
      return -0.4;
    case 'reconcile':
      return 0.15;
    case 'chat':
      return 0.05;
    case 'comfort':
      return 0.1;
    case 'strategize':
      return 0.08;
    case 'break_alliance':
      return -0.3;
    default:
      return 0;
  }
}

function computeReciprocalMood(actionType: string, target: Participant): number {
  const e = target.personality.emotionality;
  switch (actionType) {
    case 'form_alliance':
      return 0.12;
    case 'start_conflict':
      return -0.2 * e;
    case 'betray':
      return -0.35 * e;
    case 'reconcile':
      return 0.1;
    case 'chat':
      return 0.03;
    case 'comfort':
      return 0.15;
    case 'strategize':
      return 0.05;
    case 'break_alliance':
      return -0.15 * e;
    default:
      return 0;
  }
}

function computeReciprocalTrust(actionType: string): number {
  switch (actionType) {
    case 'form_alliance':
      return 0.2;
    case 'start_conflict':
      return -0.2;
    case 'betray':
      return -0.5;
    case 'reconcile':
      return 0.15;
    case 'chat':
      return 0.05;
    case 'comfort':
      return 0.12;
    case 'strategize':
      return 0.08;
    case 'break_alliance':
      return -0.25;
    default:
      return 0;
  }
}

// Generate a day_start event
export function generateDayStartEvent(
  day: number,
  tick: number,
  participants: Participant[],
  rng: SeededRandom
): GameEvent {
  const activeParticipants = participants.map((p) => p.id);
  return {
    id: nextEventId(),
    tick,
    day,
    hour: 7,
    type: 'day_start',
    participantIds: activeParticipants,
    roomId: 'living_room',
    title: `Day ${day} Begins`,
    description: `The houseguests wake up to start Day ${day} in the Big Brother house. The tension is palpable as another day of strategy and social maneuvering begins.`,
    impact: [],
    isHighlight: false,
  };
}

// Generate a day_end event
export function generateDayEndEvent(
  day: number,
  tick: number,
  participants: Participant[],
  rng: SeededRandom
): GameEvent {
  return {
    id: nextEventId(),
    tick,
    day,
    hour: 23,
    type: 'day_end',
    participantIds: participants.map((p) => p.id),
    roomId: 'living_room',
    title: `Day ${day} Ends`,
    description: `Day ${day} comes to a close. The houseguests retire to their beds, some with new alliances, others with new enemies.`,
    impact: [],
    isHighlight: false,
  };
}

// Generate nomination events
export function generateNominationEvents(
  day: number,
  tick: number,
  participants: Participant[],
  states: ParticipantState[],
  tracker: AllianceTracker,
  rng: SeededRandom
): { events: GameEvent[]; nomineeIds: string[] } {
  const events: GameEvent[] = [];
  const activeStates = states.filter((s) => s.status === 'active');
  const activeParticipants = participants.filter((p) =>
    activeStates.some((s) => s.participantId === p.id)
  );

  // Each participant votes to nominate someone (can't nominate themselves or allies)
  const nominationVotes: Record<string, number> = {};
  for (const p of activeParticipants) {
    nominationVotes[p.id] = 0;
  }

  for (const voter of activeParticipants) {
    const voterState = activeStates.find((s) => s.participantId === voter.id)!;
    const allies = new Set<string>();
    if (tracker.alliances.has(voter.id)) {
      for (const a of tracker.alliances.get(voter.id)!) {
        allies.add(a);
      }
    }

    // Find candidates (not self, not allies, preferring enemies)
    const candidates = activeParticipants.filter(
      (c) => c.id !== voter.id && !allies.has(c.id)
    );

    if (candidates.length === 0) continue;

    // Score candidates: lower trust = more likely to nominate
    let bestTarget = candidates[0];
    let lowestTrust = voterState.trust[candidates[0].id] || 0;

    for (const c of candidates) {
      const trust = voterState.trust[c.id] || 0;
      const conflictPenalty = areInConflict(tracker, voter.id, c.id) ? -0.3 : 0;
      const score = trust + conflictPenalty + rng.range(-0.2, 0.2);
      if (score < lowestTrust) {
        lowestTrust = score;
        bestTarget = c;
      }
    }

    nominationVotes[bestTarget.id] = (nominationVotes[bestTarget.id] || 0) + 1;
  }

  // Top 2 vote-getters are nominated
  const sorted = Object.entries(nominationVotes)
    .sort((a, b) => b[1] - a[1]);

  const nomineeIds = sorted.slice(0, 2).map(([id]) => id);

  for (const nid of nomineeIds) {
    const nominee = participants.find((p) => p.id === nid)!;
    const voteCount = nominationVotes[nid];

    events.push({
      id: nextEventId(),
      tick,
      day,
      hour: 20,
      type: 'nomination',
      participantIds: [nid],
      roomId: 'nomination_room',
      title: `${nominee.name} Nominated`,
      description: `${nominee.name} has been nominated for eviction with ${voteCount} votes against them. The house has spoken.`,
      impact: [
        {
          participantId: nid,
          moodDelta: -0.3,
          trustDeltas: {},
          newGoal: 'survive the eviction vote',
        },
      ],
      isHighlight: true,
      animationHint: 'cry',
    });
  }

  return { events, nomineeIds };
}

// Generate competition event
export function generateCompetitionEvent(
  day: number,
  tick: number,
  participants: Participant[],
  states: ParticipantState[],
  rng: SeededRandom
): { event: GameEvent; winnerId: string } {
  const activeParticipants = participants.filter((p) =>
    states.some((s) => s.participantId === p.id && s.status !== 'evicted')
  );

  // Competition score based on personality + randomness
  let bestScore = -1;
  let winner = activeParticipants[0];

  for (const p of activeParticipants) {
    const state = states.find((s) => s.participantId === p.id)!;
    // Score based on strategy, energy, and randomness
    const score =
      p.personality.strategy * 0.3 +
      state.energy * 0.3 +
      rng.next() * 0.4;
    if (score > bestScore) {
      bestScore = score;
      winner = p;
    }
  }

  const competitionNames = [
    'Wall of Endurance',
    'Puzzle Rush',
    'Memory Lane',
    'Balance Beam Blitz',
    'Trivia Challenge',
  ];
  const compName = rng.pick(competitionNames);

  const event: GameEvent = {
    id: nextEventId(),
    tick,
    day,
    hour: 14,
    type: 'competition_won',
    participantIds: [winner.id, ...activeParticipants.filter((p) => p.id !== winner.id).map((p) => p.id)],
    roomId: 'competition_arena',
    title: `${winner.name} Wins ${compName}!`,
    description: `In a fierce ${compName} competition, ${winner.name} outlasts all other houseguests to claim victory and gain safety for the week.`,
    impact: [
      {
        participantId: winner.id,
        moodDelta: 0.4,
        trustDeltas: {},
        newGoal: 'leverage competition win',
      },
      ...activeParticipants
        .filter((p) => p.id !== winner.id)
        .map((p) => ({
          participantId: p.id,
          moodDelta: -0.1,
          trustDeltas: {},
        })),
    ],
    isHighlight: true,
    animationHint: 'celebrate',
  };

  return { event, winnerId: winner.id };
}

// Generate eviction event
export function generateEvictionEvent(
  day: number,
  tick: number,
  participants: Participant[],
  states: ParticipantState[],
  nomineeIds: string[],
  tracker: AllianceTracker,
  rng: SeededRandom
): { event: GameEvent; evictedId: string } {
  const activeStates = states.filter(
    (s) => s.status !== 'evicted' && !nomineeIds.includes(s.participantId)
  );
  const activeVoters = participants.filter((p) =>
    activeStates.some((s) => s.participantId === p.id)
  );

  // Vote to evict: each non-nominee votes for one nominee
  const evictionVotes: Record<string, number> = {};
  for (const nid of nomineeIds) {
    evictionVotes[nid] = 0;
  }

  for (const voter of activeVoters) {
    const voterState = activeStates.find((s) => s.participantId === voter.id)!;
    let chosenNominee = nomineeIds[0];
    let lowestTrust = Infinity;

    for (const nid of nomineeIds) {
      const trust = voterState.trust[nid] || 0;
      const allyBonus = areAllies(tracker, voter.id, nid) ? 0.5 : 0;
      const score = trust + allyBonus + rng.range(-0.15, 0.15);
      if (score < lowestTrust) {
        lowestTrust = score;
        chosenNominee = nid;
      }
    }

    evictionVotes[chosenNominee] = (evictionVotes[chosenNominee] || 0) + 1;
  }

  // The nominee with the most votes is evicted
  const sorted = Object.entries(evictionVotes).sort((a, b) => b[1] - a[1]);
  const evictedId = sorted[0][0];
  const evictedParticipant = participants.find((p) => p.id === evictedId)!;
  const voteCount = sorted[0][1];
  const totalVotes = activeVoters.length;

  const event: GameEvent = {
    id: nextEventId(),
    tick,
    day,
    hour: 21,
    type: 'eviction',
    participantIds: [evictedId, ...nomineeIds.filter((n) => n !== evictedId)],
    roomId: 'living_room',
    title: `${evictedParticipant.name} Evicted!`,
    description: `By a vote of ${voteCount} to ${totalVotes - voteCount}, ${evictedParticipant.name} has been evicted from the Big Brother house. They gather their belongings and say their goodbyes.`,
    impact: [
      {
        participantId: evictedId,
        moodDelta: -0.5,
        trustDeltas: {},
        newGoal: 'leave the house with dignity',
      },
      ...nomineeIds
        .filter((n) => n !== evictedId)
        .map((n) => ({
          participantId: n,
          moodDelta: 0.3,
          trustDeltas: {},
          newGoal: 'rebuild after surviving eviction',
        })),
    ],
    isHighlight: true,
    animationHint: 'cry',
  };

  return { event, evictedId };
}
