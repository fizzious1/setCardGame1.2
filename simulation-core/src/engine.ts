import {
  Season,
  Participant,
  HouseLayout,
  Snapshot,
  ParticipantState,
  GameEvent,
  DailyRecap,
  HighlightIndex,
} from '../../shared/types';
import { createParticipants } from './participants';
import { createHouseLayout } from './house';
import {
  SeededRandom,
  AIDecision,
  AllianceTracker,
  createAllianceTracker,
  makeDecision,
  applyDecision,
  updateTracker,
} from './ai-behavior';
import {
  generateEventsFromDecisions,
  generateDayStartEvent,
  generateDayEndEvent,
  generateNominationEvents,
  generateCompetitionEvent,
  generateEvictionEvent,
  resetEventCounter,
} from './events';
import { generateDailyRecap } from './recap';
import { generateHighlights, ensureMinimumHighlights } from './highlights';

export interface SimulationConfig {
  totalDays: number;
  ticksPerDay: number;
  seed: number;
  seasonName?: string;
}

const DEFAULT_CONFIG: SimulationConfig = {
  totalDays: 7,
  ticksPerDay: 24,
  seed: 42,
  seasonName: 'Season 1: House of Rivals',
};

export function runSimulation(config: Partial<SimulationConfig> = {}): Season {
  const cfg: SimulationConfig = { ...DEFAULT_CONFIG, ...config };
  const rng = new SeededRandom(cfg.seed);
  resetEventCounter();

  const participants = createParticipants();
  const house = createHouseLayout();
  const tracker = createAllianceTracker();

  // Initialize participant states
  let currentStates: ParticipantState[] = participants.map((p) =>
    initializeState(p, house, rng)
  );

  const allSnapshots: Snapshot[] = [];
  const allEvents: GameEvent[] = [];
  const dailyRecaps: DailyRecap[] = [];
  let currentNominees: string[] = [];
  let competitionWinners: Set<string> = new Set();

  const totalTicks = cfg.totalDays * cfg.ticksPerDay;

  for (let tick = 0; tick < totalTicks; tick++) {
    const day = Math.floor(tick / cfg.ticksPerDay) + 1;
    const hour = tick % cfg.ticksPerDay;

    // Day start event
    if (hour === 7) {
      const activeParticipants = participants.filter((p) =>
        currentStates.some((s) => s.participantId === p.id && s.status !== 'evicted')
      );
      const dayStartEvent = generateDayStartEvent(day, tick, activeParticipants, rng);
      allEvents.push(dayStartEvent);
    }

    // Competition day: hour 14
    if ((day === 2 || day === 5) && hour === 14) {
      const { event, winnerId } = generateCompetitionEvent(
        day,
        tick,
        participants,
        currentStates,
        rng
      );
      allEvents.push(event);
      competitionWinners.add(winnerId);

      // Boost winner mood and energy
      currentStates = currentStates.map((s) => {
        if (s.participantId === winnerId) {
          return { ...s, mood: Math.min(1, s.mood + 0.4), energy: Math.min(1, s.energy + 0.2) };
        }
        return s;
      });
    }

    // Nomination day: hour 20
    if ((day === 3 || day === 6) && hour === 20) {
      const { events: nomEvents, nomineeIds } = generateNominationEvents(
        day,
        tick,
        participants,
        currentStates,
        tracker,
        rng
      );
      allEvents.push(...nomEvents);
      currentNominees = nomineeIds;

      // Update nominee status
      currentStates = currentStates.map((s) => {
        if (nomineeIds.includes(s.participantId)) {
          return { ...s, status: 'nominated' as const, mood: Math.max(-1, s.mood - 0.3) };
        }
        return s;
      });
    }

    // Eviction day: hour 21 on day 7
    if (day === 7 && hour === 21 && currentNominees.length >= 2) {
      const { event: evictEvent, evictedId } = generateEvictionEvent(
        day,
        tick,
        participants,
        currentStates,
        currentNominees,
        tracker,
        rng
      );
      allEvents.push(evictEvent);

      // Update evicted participant status, reset nominees
      currentStates = currentStates.map((s) => {
        if (s.participantId === evictedId) {
          return { ...s, status: 'evicted' as const };
        }
        if (currentNominees.includes(s.participantId) && s.participantId !== evictedId) {
          return { ...s, status: 'active' as const };
        }
        return s;
      });
      currentNominees = [];
    }

    // AI decisions for all active participants
    const decisions: AIDecision[] = [];
    for (const p of participants) {
      const state = currentStates.find((s) => s.participantId === p.id)!;
      if (state.status === 'evicted') continue;

      const decision = makeDecision(
        p,
        state,
        currentStates,
        participants,
        house,
        day,
        hour,
        tracker,
        currentNominees,
        rng
      );
      decisions.push(decision);
    }

    // Generate events from decisions
    const tickEvents = generateEventsFromDecisions(
      decisions,
      participants,
      currentStates,
      tick,
      day,
      hour,
      tracker,
      rng
    );
    allEvents.push(...tickEvents);

    // Apply decisions: update states and tracker
    for (const decision of decisions) {
      updateTracker(tracker, decision);
    }

    currentStates = currentStates.map((state) => {
      const decision = decisions.find((d) => d.participantId === state.participantId);
      if (!decision) return state; // evicted, keep state
      return applyDecision(state, decision, rng);
    });

    // Apply reciprocal trust changes from events
    for (const event of tickEvents) {
      for (const impact of event.impact) {
        const stateIdx = currentStates.findIndex(
          (s) => s.participantId === impact.participantId
        );
        if (stateIdx === -1) continue;
        const state = currentStates[stateIdx];

        // Apply trust deltas from event impacts
        const newTrust = { ...state.trust };
        for (const [targetId, delta] of Object.entries(impact.trustDeltas)) {
          newTrust[targetId] = Math.max(
            -1,
            Math.min(1, (newTrust[targetId] || 0) + delta)
          );
        }

        currentStates[stateIdx] = {
          ...state,
          trust: newTrust,
        };
      }
    }

    // Create snapshot
    const snapshot: Snapshot = {
      tick,
      day,
      hour,
      participantStates: currentStates.map((s) => ({ ...s, trust: { ...s.trust } })),
    };
    allSnapshots.push(snapshot);

    // Day end event and daily recap
    if (hour === 23) {
      const activeParticipants = participants.filter((p) =>
        currentStates.some((s) => s.participantId === p.id && s.status !== 'evicted')
      );
      const dayEndEvent = generateDayEndEvent(day, tick, activeParticipants, rng);
      allEvents.push(dayEndEvent);

      // Gather day's snapshots and events for recap
      const dayStartTick = (day - 1) * cfg.ticksPerDay;
      const dayEndTick = day * cfg.ticksPerDay;
      const daySnapshots = allSnapshots.filter(
        (s) => s.tick >= dayStartTick && s.tick < dayEndTick
      );
      const dayEvents = allEvents.filter(
        (e) => e.day === day
      );

      const recap = generateDailyRecap(day, dayEvents, daySnapshots, participants, tracker);
      dailyRecaps.push(recap);
    }
  }

  // Generate highlights
  let highlights = generateHighlights(allEvents);
  highlights = ensureMinimumHighlights(allEvents, highlights, 10);

  const season: Season = {
    id: `season_${cfg.seed}`,
    name: cfg.seasonName || 'Big Brother AI Season',
    participants,
    house,
    totalDays: cfg.totalDays,
    currentTick: totalTicks - 1,
    ticksPerDay: cfg.ticksPerDay,
    snapshots: allSnapshots,
    events: allEvents,
    dailyRecaps,
    highlights,
  };

  return season;
}

function initializeState(
  participant: Participant,
  house: HouseLayout,
  rng: SeededRandom
): ParticipantState {
  // Initialize trust: slightly positive or neutral for all others
  const trust: Record<string, number> = {};

  // Start in a bedroom
  const startRoom = rng.chance(0.5) ? 'bedroom1' : 'bedroom2';

  return {
    participantId: participant.id,
    roomId: startRoom,
    mood: rng.range(0.1, 0.5), // start mildly positive
    energy: 1.0,
    trust,
    currentGoal: 'settle into the house',
    status: 'active',
    positionInRoom: {
      x: rng.range(0.2, 0.8),
      y: rng.range(0.2, 0.8),
    },
  };
}
