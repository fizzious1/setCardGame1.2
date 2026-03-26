import { GameEvent, HighlightIndex } from '../../shared/types';

export function generateHighlights(events: GameEvent[]): HighlightIndex[] {
  const highlights: HighlightIndex[] = [];

  for (const event of events) {
    if (!event.isHighlight) continue;

    const importance = computeImportance(event);
    const category = categorizeEvent(event);

    highlights.push({
      eventId: event.id,
      tick: event.tick,
      day: event.day,
      label: event.title,
      importance,
      category,
    });
  }

  // Sort by importance descending
  highlights.sort((a, b) => b.importance - a.importance);

  return highlights;
}

function computeImportance(event: GameEvent): number {
  let base: number;

  switch (event.type) {
    case 'eviction':
      base = 10;
      break;
    case 'betrayal':
      base = 9;
      break;
    case 'nomination':
      base = 8;
      break;
    case 'competition_won':
      base = 7;
      break;
    case 'alliance_formed':
      base = 6;
      break;
    case 'alliance_broken':
      base = 8;
      break;
    case 'conflict':
      base = 5;
      break;
    case 'reconciliation':
      base = 4;
      break;
    case 'confession':
      base = 3;
      break;
    case 'strategy_meeting':
      base = 3;
      break;
    case 'conversation':
      base = 2;
      break;
    case 'day_start':
      base = 1;
      break;
    case 'day_end':
      base = 1;
      break;
    default:
      base = 1;
  }

  // Boost importance for events with large mood impacts
  const totalMoodImpact = event.impact.reduce(
    (sum, imp) => sum + Math.abs(imp.moodDelta),
    0
  );
  if (totalMoodImpact > 0.5) base = Math.min(10, base + 1);

  // Events involving more participants are slightly more important
  if (event.participantIds.length > 2) base = Math.min(10, base + 0.5);

  return Math.round(Math.min(10, base) * 10) / 10;
}

function categorizeEvent(
  event: GameEvent
): 'drama' | 'strategy' | 'social' | 'competition' | 'eviction' {
  switch (event.type) {
    case 'eviction':
      return 'eviction';
    case 'nomination':
      return 'eviction';
    case 'competition_won':
      return 'competition';
    case 'alliance_formed':
      return 'strategy';
    case 'alliance_broken':
      return 'drama';
    case 'betrayal':
      return 'drama';
    case 'conflict':
      return 'drama';
    case 'reconciliation':
      return 'social';
    case 'confession':
      return 'drama';
    case 'strategy_meeting':
      return 'strategy';
    case 'conversation':
      return 'social';
    default:
      return 'social';
  }
}

// After the full simulation, ensure we have enough highlights by
// promoting the most dramatic non-highlight events
export function ensureMinimumHighlights(
  events: GameEvent[],
  currentHighlights: HighlightIndex[],
  minCount: number
): HighlightIndex[] {
  if (currentHighlights.length >= minCount) return currentHighlights;

  const highlightEventIds = new Set(currentHighlights.map((h) => h.eventId));
  const candidates = events
    .filter(
      (e) =>
        !highlightEventIds.has(e.id) &&
        e.type !== 'day_start' &&
        e.type !== 'day_end' &&
        e.type !== 'conversation'
    )
    .sort((a, b) => {
      const aImp = computeImportance(a);
      const bImp = computeImportance(b);
      return bImp - aImp;
    });

  const additional = candidates.slice(0, minCount - currentHighlights.length);
  for (const event of additional) {
    currentHighlights.push({
      eventId: event.id,
      tick: event.tick,
      day: event.day,
      label: event.title,
      importance: computeImportance(event),
      category: categorizeEvent(event),
    });
  }

  currentHighlights.sort((a, b) => b.importance - a.importance);
  return currentHighlights;
}
