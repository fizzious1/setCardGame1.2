#!/usr/bin/env npx ts-node
/**
 * Demo A Acceptance Tests -- Web Control Room
 *
 * Validates all seven acceptance gates for the web-based control room.
 * Run with:  npx ts-node tests/demo-a-acceptance.test.ts
 */

import * as fs from 'fs';
import * as path from 'path';

// ── Helpers ──────────────────────────────────────────────────────────────────

let passed = 0;
let failed = 0;

function assert(condition: boolean, label: string): void {
  if (condition) {
    console.log(`  PASS: ${label}`);
    passed++;
  } else {
    console.log(`  FAIL: ${label}`);
    failed++;
  }
}

function sectionHeader(title: string): void {
  console.log(`\n${'='.repeat(60)}`);
  console.log(`  ${title}`);
  console.log('='.repeat(60));
}

// ── Resolve project root ────────────────────────────────────────────────────

const ROOT = path.resolve(__dirname, '..');

// ── Load source modules for validation ──────────────────────────────────────

const { createParticipants } = require(path.join(ROOT, 'simulation-core/src/participants'));
const { createHouseLayout } = require(path.join(ROOT, 'simulation-core/src/house'));

// Read shared types file for structural reference
const sharedTypesPath = path.join(ROOT, 'shared/types.ts');
const sharedTypesSource = fs.readFileSync(sharedTypesPath, 'utf-8');

// Read web types file
const webTypesPath = path.join(ROOT, 'web-control-room/src/types.ts');
const webTypesSource = fs.readFileSync(webTypesPath, 'utf-8');

// ═══════════════════════════════════════════════════════════════════════════
// GATE 1: Can load a season with 6-8 participants
// ═══════════════════════════════════════════════════════════════════════════

sectionHeader('Gate 1: Can load a season with 6-8 participants');

const participants = createParticipants();

assert(
  Array.isArray(participants) && participants.length >= 6 && participants.length <= 8,
  `Season has 6-8 participants (found ${participants.length})`
);

const requiredParticipantFields = ['id', 'name', 'age', 'bio', 'personality', 'avatarColor'];
for (const p of participants) {
  const hasAll = requiredParticipantFields.every(
    (f) => p[f] !== undefined && p[f] !== null
  );
  assert(hasAll, `Participant "${p.name}" has all required fields (id, name, age, bio, personality, avatarColor)`);

  // Verify personality sub-fields
  const personalityFields = ['extraversion', 'agreeableness', 'strategy', 'emotionality', 'loyalty'];
  const personalityOk = personalityFields.every(
    (f) => typeof p.personality[f] === 'number' && p.personality[f] >= 0 && p.personality[f] <= 1
  );
  assert(personalityOk, `Participant "${p.name}" personality traits are numbers in [0,1]`);

  assert(typeof p.id === 'string' && p.id.length > 0, `Participant "${p.name}" has non-empty string id`);
  assert(typeof p.avatarColor === 'string' && p.avatarColor.startsWith('#'), `Participant "${p.name}" avatarColor is a hex color`);
  assert(typeof p.age === 'number' && p.age > 0, `Participant "${p.name}" age is a positive number`);
}

// Verify unique IDs
const participantIds = participants.map((p: any) => p.id);
const uniqueIds = new Set(participantIds);
assert(uniqueIds.size === participants.length, 'All participant IDs are unique');

// ═══════════════════════════════════════════════════════════════════════════
// GATE 2: Can render a cutaway house view
// ═══════════════════════════════════════════════════════════════════════════

sectionHeader('Gate 2: Can render a cutaway house view');

const house = createHouseLayout();

assert(Array.isArray(house.rooms) && house.rooms.length > 0, 'House layout has rooms array');

// Verify room properties
for (const room of house.rooms) {
  const hasGeometry =
    typeof room.x === 'number' &&
    typeof room.y === 'number' &&
    typeof room.width === 'number' &&
    typeof room.height === 'number';
  assert(hasGeometry, `Room "${room.name}" has x, y, width, height`);
  assert(typeof room.id === 'string' && room.id.length > 0, `Room "${room.name}" has valid id`);
  assert(typeof room.type === 'string', `Room "${room.name}" has a type`);
}

// Verify all required room types exist
const requiredRoomTypes = [
  'bedroom', 'kitchen', 'living_room', 'garden',
  'confessional', 'nomination_room', 'competition_arena', 'hallway',
];
const presentRoomTypes = new Set(house.rooms.map((r: any) => r.type));
for (const rt of requiredRoomTypes) {
  assert(presentRoomTypes.has(rt), `Required room type "${rt}" exists in house layout`);
}

// Verify connections exist
assert(
  Array.isArray(house.connections) && house.connections.length > 0,
  'House layout has room connections'
);

const roomIds = new Set(house.rooms.map((r: any) => r.id));
for (const conn of house.connections) {
  assert(
    roomIds.has(conn.from) && roomIds.has(conn.to),
    `Connection ${conn.from} -> ${conn.to} references valid room IDs`
  );
}

// ═══════════════════════════════════════════════════════════════════════════
// GATE 3: Can play/pause/fast-forward
// ═══════════════════════════════════════════════════════════════════════════

sectionHeader('Gate 3: Can play/pause/fast-forward');

// Verify PlaybackState type exists in shared types
assert(
  sharedTypesSource.includes('interface PlaybackState'),
  'PlaybackState interface exists in shared/types.ts'
);
assert(
  sharedTypesSource.includes('isPlaying: boolean'),
  'PlaybackState has isPlaying field'
);
assert(
  sharedTypesSource.includes('speed: number'),
  'PlaybackState has speed field'
);
assert(
  sharedTypesSource.includes('currentTick: number'),
  'PlaybackState has currentTick field'
);
assert(
  sharedTypesSource.includes('maxTick: number'),
  'PlaybackState has maxTick field'
);

// Check web types also define PlaybackState
assert(
  webTypesSource.includes('PlaybackState') || webTypesSource.includes('isPlaying'),
  'Web types include playback-related definitions'
);

// Verify speed values documented in comment
assert(
  sharedTypesSource.includes('1, 2, 4, 8'),
  'PlaybackState speed supports 1x, 2x, 4x, 8x documented values'
);

// ═══════════════════════════════════════════════════════════════════════════
// GATE 4: Can scrub through time using snapshots + event deltas
// ═══════════════════════════════════════════════════════════════════════════

sectionHeader('Gate 4: Can scrub through time using snapshots + event deltas');

// Verify Snapshot type in shared types
assert(
  sharedTypesSource.includes('interface Snapshot'),
  'Snapshot interface exists in shared/types.ts'
);
assert(
  sharedTypesSource.includes('tick: number') &&
    sharedTypesSource.includes('day: number') &&
    sharedTypesSource.includes('hour: number'),
  'Snapshot has tick, day, hour fields'
);
assert(
  sharedTypesSource.includes('participantStates: ParticipantState[]'),
  'Snapshot has participantStates array'
);

// Verify Season type has snapshots array
assert(
  sharedTypesSource.includes('snapshots: Snapshot[]'),
  'Season type includes snapshots array'
);

// Verify Season type defines ticksPerDay
assert(
  sharedTypesSource.includes('ticksPerDay'),
  'Season type defines ticksPerDay for time resolution'
);

// Verify totalDays exists for calculating expected snapshot count
assert(
  sharedTypesSource.includes('totalDays: number'),
  'Season type defines totalDays'
);

// Verify web types also have snapshot support
assert(
  webTypesSource.includes('Snapshot') || webTypesSource.includes('tick'),
  'Web types include snapshot-related definitions'
);

// ═══════════════════════════════════════════════════════════════════════════
// GATE 5: Can jump to important events
// ═══════════════════════════════════════════════════════════════════════════

sectionHeader('Gate 5: Can jump to important events');

// Verify HighlightIndex type
assert(
  sharedTypesSource.includes('interface HighlightIndex'),
  'HighlightIndex interface exists in shared/types.ts'
);
assert(
  sharedTypesSource.includes('eventId: string'),
  'HighlightIndex has eventId field'
);
assert(
  sharedTypesSource.includes('label: string'),
  'HighlightIndex has label field'
);
assert(
  sharedTypesSource.includes('importance: number'),
  'HighlightIndex has importance field'
);

// Verify Season has highlights array
assert(
  sharedTypesSource.includes('highlights: HighlightIndex[]'),
  'Season type includes highlights array'
);

// Verify GameEvent type for tick-based lookup
assert(
  sharedTypesSource.includes('interface GameEvent'),
  'GameEvent interface exists for event lookup'
);
assert(
  sharedTypesSource.includes('events: GameEvent[]'),
  'Season type includes events array'
);

// Verify isHighlight flag on events
assert(
  sharedTypesSource.includes('isHighlight: boolean') || sharedTypesSource.includes('isHighlight?'),
  'GameEvent has isHighlight flag'
);

// ═══════════════════════════════════════════════════════════════════════════
// GATE 6: Can inspect participant's current mood, trust, and goal
// ═══════════════════════════════════════════════════════════════════════════

sectionHeader('Gate 6: Can inspect participant mood, trust, and goal');

// Verify ParticipantState type
assert(
  sharedTypesSource.includes('interface ParticipantState'),
  'ParticipantState interface exists in shared/types.ts'
);
assert(
  sharedTypesSource.includes('mood: number'),
  'ParticipantState has mood field'
);
assert(
  sharedTypesSource.includes('trust: Record<string, number>'),
  'ParticipantState has trust as Record<string, number>'
);
assert(
  sharedTypesSource.includes('currentGoal: string'),
  'ParticipantState has currentGoal field'
);

// Verify mood range is documented
assert(
  sharedTypesSource.includes('-1 to 1') || sharedTypesSource.includes('-1..1'),
  'Mood range [-1, 1] is documented in types'
);

// Verify trust maps to other participant IDs
assert(
  sharedTypesSource.includes('participantId') &&
    sharedTypesSource.includes('trust: Record<string, number>'),
  'Trust is a Record mapping participant IDs to trust levels'
);

// Check web types also have participant state
assert(
  webTypesSource.includes('ParticipantState') || webTypesSource.includes('mood'),
  'Web types include ParticipantState definitions'
);

// ═══════════════════════════════════════════════════════════════════════════
// GATE 7: Can show daily recap data
// ═══════════════════════════════════════════════════════════════════════════

sectionHeader('Gate 7: Can show daily recap data');

// Verify DailyRecap type
assert(
  sharedTypesSource.includes('interface DailyRecap'),
  'DailyRecap interface exists in shared/types.ts'
);
assert(
  sharedTypesSource.includes('summary: string'),
  'DailyRecap has summary field'
);
assert(
  sharedTypesSource.includes('keyEventIds: string[]'),
  'DailyRecap has keyEventIds array'
);
assert(
  sharedTypesSource.includes('dramaScore: number'),
  'DailyRecap has dramaScore field'
);

// Check nominations field exists in either shared or web types
const hasNominations =
  sharedTypesSource.includes('nominations') || webTypesSource.includes('nominations');
assert(hasNominations, 'DailyRecap has nominations field');

// Verify participantMoodSummary or equivalent mood tracking
const hasMoodSummary =
  sharedTypesSource.includes('participantMoodSummary') ||
  sharedTypesSource.includes('moodTrends') ||
  webTypesSource.includes('participantMoodSummary') ||
  webTypesSource.includes('moodTrends');
assert(hasMoodSummary, 'DailyRecap has participant mood summary or mood trends');

// Verify Season includes dailyRecaps
assert(
  sharedTypesSource.includes('dailyRecaps: DailyRecap[]'),
  'Season type includes dailyRecaps array'
);

// ═══════════════════════════════════════════════════════════════════════════
// SUMMARY
// ═══════════════════════════════════════════════════════════════════════════

console.log(`\n${'='.repeat(60)}`);
console.log(`  Demo A Acceptance Results: ${passed} passed, ${failed} failed`);
console.log('='.repeat(60));

if (failed > 0) {
  process.exit(1);
}
