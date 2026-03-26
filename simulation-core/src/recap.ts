import {
  DailyRecap,
  GameEvent,
  ParticipantState,
  Participant,
  Snapshot,
} from '../../shared/types';
import { AllianceTracker } from './ai-behavior';

export function generateDailyRecap(
  day: number,
  dayEvents: GameEvent[],
  daySnapshots: Snapshot[],
  participants: Participant[],
  tracker: AllianceTracker
): DailyRecap {
  // Compute mood summary: average mood for each participant across the day
  const moodSums: Record<string, number> = {};
  const moodCounts: Record<string, number> = {};

  for (const snapshot of daySnapshots) {
    for (const state of snapshot.participantStates) {
      if (!moodSums[state.participantId]) {
        moodSums[state.participantId] = 0;
        moodCounts[state.participantId] = 0;
      }
      moodSums[state.participantId] += state.mood;
      moodCounts[state.participantId] += 1;
    }
  }

  const participantMoodSummary: Record<string, number> = {};
  for (const [pid, sum] of Object.entries(moodSums)) {
    participantMoodSummary[pid] = Math.round((sum / moodCounts[pid]) * 100) / 100;
  }

  // Find key events (highlights and important types)
  const keyEvents = dayEvents.filter(
    (e) =>
      e.isHighlight ||
      e.type === 'nomination' ||
      e.type === 'eviction' ||
      e.type === 'competition_won' ||
      e.type === 'alliance_formed' ||
      e.type === 'betrayal'
  );
  const keyEventIds = keyEvents.map((e) => e.id);

  // Find nominations
  const nominations = dayEvents
    .filter((e) => e.type === 'nomination')
    .flatMap((e) => e.participantIds);

  // Find eviction
  const evictionEvent = dayEvents.find((e) => e.type === 'eviction');
  const evictedId = evictionEvent ? evictionEvent.participantIds[0] : undefined;

  // Build alliance map from tracker
  const allianceMap: Record<string, string[]> = {};
  for (const p of participants) {
    const allies = tracker.alliances.get(p.id);
    if (allies && allies.size > 0) {
      allianceMap[p.id] = Array.from(allies);
    }
  }

  // Compute drama score
  const dramaScore = computeDramaScore(dayEvents);

  // Generate summary
  const summary = generateSummaryText(day, dayEvents, participants, keyEvents, evictedId);

  return {
    day,
    summary,
    keyEventIds,
    participantMoodSummary,
    nominations,
    evictedId,
    allianceMap,
    dramaScore,
  };
}

function computeDramaScore(events: GameEvent[]): number {
  let score = 1; // baseline

  for (const event of events) {
    switch (event.type) {
      case 'conflict':
        score += 1.5;
        break;
      case 'betrayal':
        score += 2.5;
        break;
      case 'alliance_formed':
        score += 0.5;
        break;
      case 'alliance_broken':
        score += 2;
        break;
      case 'nomination':
        score += 1;
        break;
      case 'eviction':
        score += 2;
        break;
      case 'competition_won':
        score += 0.8;
        break;
      case 'reconciliation':
        score += 0.3;
        break;
      case 'confession':
        score += 0.3;
        break;
      case 'strategy_meeting':
        score += 0.2;
        break;
      case 'conversation':
        score += 0.1;
        break;
    }
  }

  return Math.min(10, Math.round(score * 10) / 10);
}

function generateSummaryText(
  day: number,
  events: GameEvent[],
  participants: Participant[],
  keyEvents: GameEvent[],
  evictedId: string | undefined
): string {
  const pMap = new Map(participants.map((p) => [p.id, p]));

  const parts: string[] = [];
  parts.push(`Day ${day} in the Big Brother house`);

  const allianceEvents = keyEvents.filter((e) => e.type === 'alliance_formed');
  const conflictEvents = events.filter((e) => e.type === 'conflict');
  const betrayalEvents = keyEvents.filter((e) => e.type === 'betrayal');
  const nominationEvents = keyEvents.filter((e) => e.type === 'nomination');
  const competitionEvents = keyEvents.filter((e) => e.type === 'competition_won');

  if (allianceEvents.length > 0) {
    const allianceNames = allianceEvents.map((e) =>
      e.participantIds.map((id) => pMap.get(id)?.name || id).join(' and ')
    );
    parts.push(`saw new alliances form between ${allianceNames.join('; ')}`);
  }

  if (conflictEvents.length > 0) {
    parts.push(`with ${conflictEvents.length} conflict${conflictEvents.length > 1 ? 's' : ''} erupting`);
  }

  if (betrayalEvents.length > 0) {
    const betrayer = pMap.get(betrayalEvents[0].participantIds[0]);
    parts.push(`and a shocking betrayal by ${betrayer?.name}`);
  }

  if (competitionEvents.length > 0) {
    const winner = pMap.get(competitionEvents[0].participantIds[0]);
    parts.push(`${winner?.name} won the competition`);
  }

  if (nominationEvents.length > 0) {
    const nominees = nominationEvents.flatMap((e) =>
      e.participantIds.map((id) => pMap.get(id)?.name || id)
    );
    parts.push(`and ${nominees.join(' and ')} were nominated for eviction`);
  }

  if (evictedId) {
    const evicted = pMap.get(evictedId);
    parts.push(`culminating in the eviction of ${evicted?.name}`);
  }

  return parts.join(', ') + '.';
}
