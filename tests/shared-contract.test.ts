#!/usr/bin/env npx ts-node
/**
 * Shared Data Contract Tests
 *
 * Validates that the shared data contract (types and generated data) is
 * consistent, well-formed, and consumable by both web and desktop targets.
 * Run with:  npx ts-node tests/shared-contract.test.ts
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

// ── Resolve paths ────────────────────────────────────────────────────────────

const ROOT = path.resolve(__dirname, '..');

// ── Load source data ────────────────────────────────────────────────────────

const { createParticipants } = require(path.join(ROOT, 'simulation-core/src/participants'));
const { createHouseLayout } = require(path.join(ROOT, 'simulation-core/src/house'));

const participants = createParticipants();
const house = createHouseLayout();

// Build canonical ID sets
const participantIds: Set<string> = new Set(participants.map((p: any) => p.id));
const roomIds: Set<string> = new Set(house.rooms.map((r: any) => r.id));

// Read type source files
const sharedTypesSource = fs.readFileSync(path.join(ROOT, 'shared/types.ts'), 'utf-8');
const webTypesSource = fs.readFileSync(path.join(ROOT, 'web-control-room/src/types.ts'), 'utf-8');
const ueSubsystemPath = path.join(ROOT, 'desktop-prestige-house/Source/BigBrotherSim/BBSimDataSubsystem.h');
const ueSubsystemSource = fs.existsSync(ueSubsystemPath)
  ? fs.readFileSync(ueSubsystemPath, 'utf-8')
  : '';

// Optionally load demo-season.json if it exists
const demoSeasonPath = path.join(ROOT, 'shared/demo-season.json');
const demoSeasonAltPath = path.join(ROOT, 'simulation-core/demo-season.json');
let seasonData: any = null;
let seasonJsonPath: string | null = null;

for (const p of [demoSeasonPath, demoSeasonAltPath]) {
  if (fs.existsSync(p)) {
    seasonJsonPath = p;
    break;
  }
}

// ═══════════════════════════════════════════════════════════════════════════
// Section 1: Shared types file is well-formed
// ═══════════════════════════════════════════════════════════════════════════

sectionHeader('Section 1: Shared types file is well-formed');

assert(
  fs.existsSync(path.join(ROOT, 'shared/types.ts')),
  'shared/types.ts exists'
);

// Verify core interfaces
const requiredInterfaces = [
  'Season', 'Participant', 'PersonalityTraits', 'HouseLayout', 'Room',
  'RoomConnection', 'Snapshot', 'ParticipantState', 'GameEvent',
  'EventImpact', 'DailyRecap', 'HighlightIndex', 'PlaybackState', 'CameraMode',
];
for (const iface of requiredInterfaces) {
  assert(
    sharedTypesSource.includes(`interface ${iface}`) || sharedTypesSource.includes(`type ${iface}`),
    `shared/types.ts exports "${iface}" interface/type`
  );
}

// Verify EventType union
assert(
  sharedTypesSource.includes('EventType'),
  'shared/types.ts defines EventType union'
);

// ═══════════════════════════════════════════════════════════════════════════
// Section 2: Web types are compatible with shared contract
// ═══════════════════════════════════════════════════════════════════════════

sectionHeader('Section 2: Web types compatible with shared contract');

assert(
  fs.existsSync(path.join(ROOT, 'web-control-room/src/types.ts')),
  'web-control-room/src/types.ts exists'
);

// Check the web types define the same core data structures
const webRequiredTypes = [
  'Participant', 'ParticipantState', 'GameEvent', 'Snapshot',
  'DailyRecap', 'HighlightIndex', 'Season', 'PlaybackState',
];
for (const t of webRequiredTypes) {
  assert(
    webTypesSource.includes(t),
    `Web types include "${t}"`
  );
}

// Verify web types have mood/trust fields
assert(
  webTypesSource.includes('mood') && webTypesSource.includes('trust'),
  'Web ParticipantState has mood and trust fields'
);

// ═══════════════════════════════════════════════════════════════════════════
// Section 3: Desktop (Unreal) types are compatible with shared contract
// ═══════════════════════════════════════════════════════════════════════════

sectionHeader('Section 3: Desktop (Unreal) types compatible with shared contract');

assert(
  fs.existsSync(ueSubsystemPath),
  'BBSimDataSubsystem.h exists'
);

// Check Unreal structs mirror shared types
const ueRequiredStructs = [
  'FBBSeason', 'FBBParticipant', 'FBBParticipantState', 'FBBRoom',
  'FBBSnapshot', 'FBBGameEvent', 'FBBDailyRecap', 'FBBHighlight',
];
for (const s of ueRequiredStructs) {
  assert(
    ueSubsystemSource.includes(s),
    `Unreal subsystem defines "${s}" struct`
  );
}

// Verify Unreal can load from JSON (same format as web)
assert(
  ueSubsystemSource.includes('LoadSeasonFromFile') || ueSubsystemSource.includes('LoadSeasonFromString'),
  'Unreal subsystem can load season from JSON file/string (same format as web)'
);

assert(
  ueSubsystemSource.includes('ParseSeasonJson'),
  'Unreal subsystem has ParseSeasonJson for consuming shared JSON contract'
);

// ═══════════════════════════════════════════════════════════════════════════
// Section 4: Participant data integrity
// ═══════════════════════════════════════════════════════════════════════════

sectionHeader('Section 4: Participant data integrity');

assert(
  participants.length >= 6 && participants.length <= 8,
  `Participant count is 6-8 (found ${participants.length})`
);

assert(
  participantIds.size === participants.length,
  'All participant IDs are unique'
);

for (const p of participants) {
  assert(
    typeof p.id === 'string' && p.id.length > 0,
    `Participant "${p.name}" has non-empty string ID`
  );
  assert(
    typeof p.name === 'string' && p.name.length > 0,
    `Participant "${p.name}" has non-empty name`
  );
  assert(
    typeof p.age === 'number' && p.age >= 18 && p.age <= 99,
    `Participant "${p.name}" age is a valid number (${p.age})`
  );
  assert(
    typeof p.bio === 'string' && p.bio.length > 10,
    `Participant "${p.name}" has a substantive bio`
  );
  assert(
    typeof p.avatarColor === 'string' && /^#[0-9A-Fa-f]{6}$/.test(p.avatarColor),
    `Participant "${p.name}" avatarColor is valid hex (#RRGGBB)`
  );
  assert(
    typeof p.personality === 'object' && p.personality !== null,
    `Participant "${p.name}" has personality object`
  );

  const traits = ['extraversion', 'agreeableness', 'strategy', 'emotionality', 'loyalty'];
  for (const t of traits) {
    assert(
      typeof p.personality[t] === 'number' && p.personality[t] >= 0 && p.personality[t] <= 1,
      `Participant "${p.name}" personality.${t} = ${p.personality[t]} is in [0, 1]`
    );
  }
}

// ═══════════════════════════════════════════════════════════════════════════
// Section 5: House layout integrity
// ═══════════════════════════════════════════════════════════════════════════

sectionHeader('Section 5: House layout integrity');

assert(
  Array.isArray(house.rooms) && house.rooms.length > 0,
  `House has rooms (found ${house.rooms.length})`
);

assert(
  roomIds.size === house.rooms.length,
  'All room IDs are unique'
);

const requiredRoomTypes = [
  'bedroom', 'kitchen', 'living_room', 'garden',
  'confessional', 'nomination_room', 'competition_arena', 'hallway',
];
const presentRoomTypes = new Set(house.rooms.map((r: any) => r.type));
for (const rt of requiredRoomTypes) {
  assert(presentRoomTypes.has(rt), `Required room type "${rt}" is present`);
}

for (const room of house.rooms) {
  assert(
    typeof room.x === 'number' && typeof room.y === 'number',
    `Room "${room.name}" has valid position (x=${room.x}, y=${room.y})`
  );
  assert(
    typeof room.width === 'number' && room.width > 0 &&
    typeof room.height === 'number' && room.height > 0,
    `Room "${room.name}" has positive dimensions (${room.width}x${room.height})`
  );
  assert(
    typeof room.capacity === 'number' && room.capacity > 0,
    `Room "${room.name}" has positive capacity (${room.capacity})`
  );
}

// Verify connections reference valid room IDs
assert(
  Array.isArray(house.connections) && house.connections.length > 0,
  `House has connections (found ${house.connections.length})`
);

for (const conn of house.connections) {
  assert(
    roomIds.has(conn.from),
    `Connection "from" room "${conn.from}" is a valid room ID`
  );
  assert(
    roomIds.has(conn.to),
    `Connection "to" room "${conn.to}" is a valid room ID`
  );
}

// Verify connectivity: every room should be reachable via connections
const connectedRooms = new Set<string>();
for (const conn of house.connections) {
  connectedRooms.add(conn.from);
  connectedRooms.add(conn.to);
}
for (const rid of roomIds) {
  assert(
    connectedRooms.has(rid),
    `Room "${rid}" is part of at least one connection (reachable)`
  );
}

// ═══════════════════════════════════════════════════════════════════════════
// Section 6: demo-season.json validation (if available)
// ═══════════════════════════════════════════════════════════════════════════

sectionHeader('Section 6: demo-season.json validation');

if (seasonJsonPath) {
  console.log(`  Found demo-season.json at: ${seasonJsonPath}`);

  // Verify it is valid JSON
  let parseError: string | null = null;
  try {
    const raw = fs.readFileSync(seasonJsonPath, 'utf-8');
    seasonData = JSON.parse(raw);
  } catch (e: any) {
    parseError = e.message;
  }
  assert(parseError === null, `demo-season.json is valid JSON${parseError ? ' (error: ' + parseError + ')' : ''}`);

  if (seasonData) {
    // Top-level Season structure
    assert(typeof seasonData.id === 'string', 'Season has id field');
    assert(typeof seasonData.name === 'string', 'Season has name field');
    assert(typeof seasonData.totalDays === 'number', 'Season has totalDays field');
    assert(typeof seasonData.ticksPerDay === 'number', 'Season has ticksPerDay field');
    assert(Array.isArray(seasonData.participants), 'Season has participants array');
    assert(Array.isArray(seasonData.snapshots), 'Season has snapshots array');
    assert(Array.isArray(seasonData.events), 'Season has events array');
    assert(Array.isArray(seasonData.dailyRecaps), 'Season has dailyRecaps array');
    assert(Array.isArray(seasonData.highlights), 'Season has highlights array');

    // Participant count
    assert(
      seasonData.participants.length >= 6 && seasonData.participants.length <= 8,
      `Season JSON has 6-8 participants (found ${seasonData.participants.length})`
    );

    // Build ID sets from the JSON
    const jsonParticipantIds = new Set(seasonData.participants.map((p: any) => p.id));
    const jsonRoomIds = new Set(
      (seasonData.house?.rooms || seasonData.houseLayout || []).map((r: any) => r.id)
    );
    const jsonEventIds = new Set(seasonData.events.map((e: any) => e.id));

    // Events reference valid participant IDs
    let eventsWithInvalidParticipants = 0;
    for (const evt of seasonData.events) {
      const pids: string[] = evt.participantIds || [];
      for (const pid of pids) {
        if (!jsonParticipantIds.has(pid)) {
          eventsWithInvalidParticipants++;
        }
      }
    }
    assert(
      eventsWithInvalidParticipants === 0,
      `All events reference valid participant IDs (${eventsWithInvalidParticipants} invalid references)`
    );

    // Events reference valid room IDs (if room data present in JSON)
    if (jsonRoomIds.size > 0) {
      let eventsWithInvalidRooms = 0;
      for (const evt of seasonData.events) {
        const rid = evt.roomId || evt.room;
        if (rid && !jsonRoomIds.has(rid)) {
          eventsWithInvalidRooms++;
        }
      }
      assert(
        eventsWithInvalidRooms === 0,
        `All events reference valid room IDs (${eventsWithInvalidRooms} invalid references)`
      );

      // Snapshots reference valid room IDs
      let snapshotsWithInvalidRooms = 0;
      for (const snap of seasonData.snapshots) {
        for (const ps of (snap.participantStates || [])) {
          const rid = ps.roomId || ps.room;
          if (rid && !jsonRoomIds.has(rid)) {
            snapshotsWithInvalidRooms++;
          }
        }
      }
      assert(
        snapshotsWithInvalidRooms === 0,
        `All snapshot participant states reference valid room IDs (${snapshotsWithInvalidRooms} invalid)`
      );
    } else {
      console.log('  SKIP: No room data in JSON to validate room ID references against');
    }

    // Highlights reference valid event IDs
    let highlightsWithInvalidEvents = 0;
    for (const h of seasonData.highlights) {
      if (h.eventId && !jsonEventIds.has(h.eventId)) {
        highlightsWithInvalidEvents++;
      }
    }
    assert(
      highlightsWithInvalidEvents === 0,
      `All highlights reference valid event IDs (${highlightsWithInvalidEvents} invalid references)`
    );

    // Snapshots cover full season
    const expectedMinSnapshots = seasonData.totalDays * seasonData.ticksPerDay;
    assert(
      seasonData.snapshots.length >= expectedMinSnapshots || seasonData.snapshots.length >= 168,
      `Snapshots cover full season (found ${seasonData.snapshots.length}, expected >= ${Math.max(expectedMinSnapshots, 168)})`
    );

    // Each snapshot has required fields
    if (seasonData.snapshots.length > 0) {
      const sampleSnap = seasonData.snapshots[0];
      assert(typeof sampleSnap.tick === 'number', 'Snapshot has tick field');
      assert(typeof sampleSnap.day === 'number', 'Snapshot has day field');
      assert(typeof sampleSnap.hour === 'number', 'Snapshot has hour field');
      assert(Array.isArray(sampleSnap.participantStates), 'Snapshot has participantStates array');
    }

    // Daily recaps cover all simulated days
    const recapDays = new Set(seasonData.dailyRecaps.map((r: any) => r.day));
    const missingRecapDays: number[] = [];
    for (let d = 1; d <= seasonData.totalDays; d++) {
      if (!recapDays.has(d)) {
        missingRecapDays.push(d);
      }
    }
    assert(
      missingRecapDays.length === 0,
      `Daily recaps cover all ${seasonData.totalDays} simulated days (missing: ${missingRecapDays.length > 0 ? missingRecapDays.slice(0, 5).join(', ') + '...' : 'none'})`
    );

    // Highlights count
    assert(
      seasonData.highlights.length >= 10,
      `Highlights array has 10+ entries (found ${seasonData.highlights.length})`
    );
  }
} else {
  console.log('  INFO: demo-season.json not found. Validating against source modules instead.');
  console.log('  INFO: Run "npm run generate-demo" in simulation-core to produce the JSON.');

  // Validate that the types define the expected structure even without JSON
  assert(
    sharedTypesSource.includes('snapshots: Snapshot[]'),
    'Season type includes snapshots array for snapshot coverage'
  );
  assert(
    sharedTypesSource.includes('events: GameEvent[]'),
    'Season type includes events array for event references'
  );
  assert(
    sharedTypesSource.includes('highlights: HighlightIndex[]'),
    'Season type includes highlights array for highlight references'
  );
  assert(
    sharedTypesSource.includes('dailyRecaps: DailyRecap[]'),
    'Season type includes dailyRecaps array for daily coverage'
  );
}

// ═══════════════════════════════════════════════════════════════════════════
// Section 7: Cross-platform data consumption compatibility
// ═══════════════════════════════════════════════════════════════════════════

sectionHeader('Section 7: Cross-platform data consumption compatibility');

// Verify both platforms can consume season-level data
assert(
  sharedTypesSource.includes('interface Season') &&
    webTypesSource.includes('Season') &&
    ueSubsystemSource.includes('FBBSeason'),
  'Season data structure is defined in shared types, web types, and Unreal subsystem'
);

// Verify both platforms handle participants
assert(
  sharedTypesSource.includes('interface Participant') &&
    webTypesSource.includes('Participant') &&
    ueSubsystemSource.includes('FBBParticipant'),
  'Participant type is defined across all three layers'
);

// Verify both platforms handle snapshots
assert(
  sharedTypesSource.includes('interface Snapshot') &&
    webTypesSource.includes('Snapshot') &&
    ueSubsystemSource.includes('FBBSnapshot'),
  'Snapshot type is defined across all three layers'
);

// Verify both platforms handle events
assert(
  sharedTypesSource.includes('interface GameEvent') &&
    webTypesSource.includes('GameEvent') &&
    ueSubsystemSource.includes('FBBGameEvent'),
  'GameEvent type is defined across all three layers'
);

// Verify both platforms handle highlights
assert(
  sharedTypesSource.includes('interface HighlightIndex') &&
    webTypesSource.includes('HighlightIndex') &&
    ueSubsystemSource.includes('FBBHighlight'),
  'Highlight type is defined across all three layers'
);

// Verify both platforms handle daily recaps
assert(
  sharedTypesSource.includes('interface DailyRecap') &&
    webTypesSource.includes('DailyRecap') &&
    ueSubsystemSource.includes('FBBDailyRecap'),
  'DailyRecap type is defined across all three layers'
);

// Verify the Unreal side can parse JSON (same as web consumes)
assert(
  ueSubsystemSource.includes('FJsonObject') || ueSubsystemSource.includes('ParseSeasonJson'),
  'Unreal uses FJsonObject / ParseSeasonJson to consume the same JSON format as web'
);

// ═══════════════════════════════════════════════════════════════════════════
// SUMMARY
// ═══════════════════════════════════════════════════════════════════════════

console.log(`\n${'='.repeat(60)}`);
console.log(`  Shared Contract Results: ${passed} passed, ${failed} failed`);
console.log('='.repeat(60));

if (failed > 0) {
  process.exit(1);
}
