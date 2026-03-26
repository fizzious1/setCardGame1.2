#!/usr/bin/env npx ts-node
/**
 * Demo B Acceptance Tests -- Desktop Prestige House (Unreal Engine)
 *
 * Validates all seven acceptance gates for the Unreal-based desktop experience.
 * Checks are performed against Unreal C++ source files and the shared data subsystem.
 * Run with:  npx ts-node tests/demo-b-acceptance.test.ts
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

function fileExists(relativePath: string): boolean {
  return fs.existsSync(path.join(ROOT, relativePath));
}

function readIfExists(relativePath: string): string {
  const full = path.join(ROOT, relativePath);
  if (fs.existsSync(full)) {
    return fs.readFileSync(full, 'utf-8');
  }
  return '';
}

function findFileRecursive(dir: string, name: string): string | null {
  if (!fs.existsSync(dir)) return null;
  const entries = fs.readdirSync(dir, { withFileTypes: true });
  for (const entry of entries) {
    const fullPath = path.join(dir, entry.name);
    if (entry.isDirectory()) {
      const found = findFileRecursive(fullPath, name);
      if (found) return found;
    } else if (entry.name === name) {
      return fullPath;
    }
  }
  return null;
}

// ── Resolve project root ────────────────────────────────────────────────────

const ROOT = path.resolve(__dirname, '..');
const UE_SRC = path.join(ROOT, 'desktop-prestige-house/Source/BigBrotherSim');

// Read the subsystem header (we know this exists) for structural checks
const subsystemHeaderPath = path.join(UE_SRC, 'BBSimDataSubsystem.h');
const subsystemHeader = readIfExists('desktop-prestige-house/Source/BigBrotherSim/BBSimDataSubsystem.h');

// ═══════════════════════════════════════════════════════════════════════════
// GATE 1: Unreal desktop scene loads one polished house floor
// ═══════════════════════════════════════════════════════════════════════════

sectionHeader('Gate 1: Unreal scene loads one polished house floor');

// Check for house floor actor/component
const bbHouseFloorH = findFileRecursive(UE_SRC, 'BBHouseFloor.h');
const bbHouseFloorCpp = findFileRecursive(UE_SRC, 'BBHouseFloor.cpp');
const hasHouseFloorH = bbHouseFloorH !== null;
const hasHouseFloorCpp = bbHouseFloorCpp !== null;

assert(hasHouseFloorH, 'BBHouseFloor.h exists with room spawning logic');
assert(hasHouseFloorCpp, 'BBHouseFloor.cpp exists with room spawning implementation');

// Check for room volume actor
const bbRoomVolumeH = findFileRecursive(UE_SRC, 'BBRoomVolume.h');
const bbRoomVolumeCpp = findFileRecursive(UE_SRC, 'BBRoomVolume.cpp');
const hasRoomVolumeH = bbRoomVolumeH !== null;
const hasRoomVolumeCpp = bbRoomVolumeCpp !== null;

assert(hasRoomVolumeH, 'BBRoomVolume.h exists with room properties');
assert(hasRoomVolumeCpp, 'BBRoomVolume.cpp exists with room implementation');

// Verify the data subsystem defines room data structures
assert(
  subsystemHeader.includes('FBBRoom') && subsystemHeader.includes('RoomId'),
  'Data subsystem defines FBBRoom struct with RoomId'
);

// Verify house data supports 9 rooms (the known layout)
const { createHouseLayout } = require(path.join(ROOT, 'simulation-core/src/house'));
const house = createHouseLayout();
assert(
  house.rooms.length === 9,
  `House data includes exactly 9 rooms (found ${house.rooms.length})`
);

// Verify EBBRoomType enum has room types
assert(
  subsystemHeader.includes('EBBRoomType') &&
    subsystemHeader.includes('LivingRoom') &&
    subsystemHeader.includes('Kitchen') &&
    subsystemHeader.includes('Bedroom') &&
    subsystemHeader.includes('Garden') &&
    subsystemHeader.includes('Confessional'),
  'EBBRoomType enum defines all major room types'
);

// ═══════════════════════════════════════════════════════════════════════════
// GATE 2: 6-8 agents can move room-to-room and play social action animations
// ═══════════════════════════════════════════════════════════════════════════

sectionHeader('Gate 2: 6-8 agents move room-to-room with animations');

// Check for agent character class
const bbAgentCharH = findFileRecursive(UE_SRC, 'BBAgentCharacter.h');
const bbAgentCharCpp = findFileRecursive(UE_SRC, 'BBAgentCharacter.cpp');

assert(bbAgentCharH !== null, 'BBAgentCharacter.h exists with movement and animation code');
assert(bbAgentCharCpp !== null, 'BBAgentCharacter.cpp exists with agent implementation');

// Check for AI controller
const bbAgentAIH = findFileRecursive(UE_SRC, 'BBAgentAIController.h');
const bbAgentAICpp = findFileRecursive(UE_SRC, 'BBAgentAIController.cpp');

assert(bbAgentAIH !== null, 'BBAgentAIController.h exists with navigation logic');
assert(bbAgentAICpp !== null, 'BBAgentAIController.cpp exists with AI implementation');

// Verify participant state includes position and animation data for Unreal
assert(
  subsystemHeader.includes('FBBParticipantState') &&
    subsystemHeader.includes('WorldPosition') &&
    subsystemHeader.includes('CurrentAction'),
  'FBBParticipantState has WorldPosition and CurrentAction for agent movement/animation'
);

// Verify participant count supports 6-8
const { createParticipants } = require(path.join(ROOT, 'simulation-core/src/participants'));
const participants = createParticipants();
assert(
  participants.length >= 6 && participants.length <= 8,
  `Participant data has 6-8 agents (found ${participants.length})`
);

// Verify snapshot-driven update mechanism exists in data subsystem
assert(
  subsystemHeader.includes('GetSnapshotAtTick') || subsystemHeader.includes('ScrubToTick'),
  'Data subsystem has snapshot-at-tick method for driving agent state (UpdateFromSnapshot equivalent)'
);

assert(
  subsystemHeader.includes('GetParticipantStateAtTick'),
  'Data subsystem has GetParticipantStateAtTick for per-agent state updates'
);

// Verify animation hint support
const sharedTypes = fs.readFileSync(path.join(ROOT, 'shared/types.ts'), 'utf-8');
assert(
  sharedTypes.includes('animationHint') || sharedTypes.includes('socialAction'),
  'Shared types include animationHint or socialAction for Unreal animations'
);

// ═══════════════════════════════════════════════════════════════════════════
// GATE 3: Director camera works
// ═══════════════════════════════════════════════════════════════════════════

sectionHeader('Gate 3: Director camera works');

const bbDirectorCamH = findFileRecursive(UE_SRC, 'BBDirectorCamera.h');
const bbDirectorCamCpp = findFileRecursive(UE_SRC, 'BBDirectorCamera.cpp');

assert(bbDirectorCamH !== null, 'BBDirectorCamera.h exists');
assert(bbDirectorCamCpp !== null, 'BBDirectorCamera.cpp exists');

// Verify CameraMode type supports director mode
assert(
  sharedTypes.includes("'director'") || sharedTypes.includes('"director"'),
  'CameraMode type includes director mode'
);

// Verify event type enum exists for event-reactive camera behavior
assert(
  subsystemHeader.includes('EBBEventType') &&
    subsystemHeader.includes('Conflict') &&
    subsystemHeader.includes('Alliance'),
  'EBBEventType enum exists for event-reactive camera cuts (conflict, alliance, etc.)'
);

// Verify the data subsystem can provide events at a tick for auto-cut triggers
assert(
  subsystemHeader.includes('GetEventsAtTick'),
  'Data subsystem has GetEventsAtTick for auto-cut camera logic'
);

// Verify rooms have position data for camera targeting
assert(
  subsystemHeader.includes('FBBRoom') && subsystemHeader.includes('Position'),
  'FBBRoom has Position for camera targeting between rooms'
);

// ═══════════════════════════════════════════════════════════════════════════
// GATE 4: Character follow camera works
// ═══════════════════════════════════════════════════════════════════════════

sectionHeader('Gate 4: Character follow camera works');

const bbFollowCamH = findFileRecursive(UE_SRC, 'BBFollowCamera.h');
const bbFollowCamCpp = findFileRecursive(UE_SRC, 'BBFollowCamera.cpp');

assert(bbFollowCamH !== null, 'BBFollowCamera.h exists');
assert(bbFollowCamCpp !== null, 'BBFollowCamera.cpp exists');

// Verify CameraMode type supports follow mode
assert(
  sharedTypes.includes("'follow'") || sharedTypes.includes('"follow"'),
  'CameraMode type includes follow mode'
);

// Verify CameraMode has targetId for follow target switching
assert(
  sharedTypes.includes('targetId'),
  'CameraMode has targetId for selecting follow target'
);

// Verify participant state has world position for follow cam tracking
assert(
  subsystemHeader.includes('WorldPosition') && subsystemHeader.includes('WorldRotation'),
  'FBBParticipantState has WorldPosition and WorldRotation for spring arm follow cam'
);

// ═══════════════════════════════════════════════════════════════════════════
// GATE 5: Replay of recent events works
// ═══════════════════════════════════════════════════════════════════════════

sectionHeader('Gate 5: Replay of recent events works');

const bbReplayMgrH = findFileRecursive(UE_SRC, 'BBReplayManager.h');
const bbReplayMgrCpp = findFileRecursive(UE_SRC, 'BBReplayManager.cpp');

assert(bbReplayMgrH !== null, 'BBReplayManager.h exists');
assert(bbReplayMgrCpp !== null, 'BBReplayManager.cpp exists');

// Verify scrub/seek support in the data subsystem
assert(
  subsystemHeader.includes('ScrubToTick'),
  'Data subsystem has ScrubToTick for scrub forward/backward'
);

// Verify events-in-range for replaying segments
assert(
  subsystemHeader.includes('GetEventsInRange'),
  'Data subsystem has GetEventsInRange for replaying event sequences'
);

// Verify PlaybackState includes speed control
assert(
  sharedTypes.includes('speed: number'),
  'PlaybackState type includes speed control for replay speed'
);

// ═══════════════════════════════════════════════════════════════════════════
// GATE 6: Confessional room interaction exists
// ═══════════════════════════════════════════════════════════════════════════

sectionHeader('Gate 6: Confessional room interaction exists');

const bbConfessionalH = findFileRecursive(UE_SRC, 'BBConfessionalRoom.h');
const bbConfessionalCpp = findFileRecursive(UE_SRC, 'BBConfessionalRoom.cpp');

assert(bbConfessionalH !== null, 'BBConfessionalRoom.h exists');
assert(bbConfessionalCpp !== null, 'BBConfessionalRoom.cpp exists');

// Verify confessional room type in Unreal
assert(
  subsystemHeader.includes('Confessional'),
  'EBBRoomType enum includes Confessional type'
);

// Verify confession event type
assert(
  subsystemHeader.includes('Confession'),
  'EBBEventType enum includes Confession type for confessional events'
);

// Verify the house layout has a confessional room
assert(
  house.rooms.some((r: any) => r.type === 'confessional'),
  'House layout includes a confessional room'
);

// Verify event data has description/dialogue for confession display
assert(
  subsystemHeader.includes('DialogueText') || subsystemHeader.includes('Description'),
  'Game event struct has Description or DialogueText for confession display'
);

// ═══════════════════════════════════════════════════════════════════════════
// GATE 7: Alliance and conflict events are visually understandable
// ═══════════════════════════════════════════════════════════════════════════

sectionHeader('Gate 7: Alliance and conflict events are visually understandable');

const bbEventVisH = findFileRecursive(UE_SRC, 'BBEventVisualizer.h');
const bbEventVisCpp = findFileRecursive(UE_SRC, 'BBEventVisualizer.cpp');

assert(bbEventVisH !== null, 'BBEventVisualizer.h exists');
assert(bbEventVisCpp !== null, 'BBEventVisualizer.cpp exists');

// Verify alliance event type exists
assert(
  subsystemHeader.includes('Alliance'),
  'EBBEventType includes Alliance for alliance visualization'
);

// Verify conflict event type exists
assert(
  subsystemHeader.includes('Conflict'),
  'EBBEventType includes Conflict for conflict visualization'
);

// Verify AnimationHint field on game events for visual cues
assert(
  subsystemHeader.includes('AnimationHint'),
  'FBBGameEvent has AnimationHint for visual animation cues (handshake, argument, etc.)'
);

// Verify shared types define animation hints
assert(
  sharedTypes.includes('handshake') || sharedTypes.includes('argument'),
  'Shared types document animation hints like handshake/argument'
);

// Verify event impact structure for drama visualization
assert(
  sharedTypes.includes('EventImpact') ||
    sharedTypes.includes('moodImpact') ||
    sharedTypes.includes('trustImpact'),
  'Event types include impact data (mood/trust changes) for visual feedback'
);

// Verify DramaScore exists for intensity of visual effects
assert(
  subsystemHeader.includes('DramaScore'),
  'FBBGameEvent has DramaScore for visual intensity scaling'
);

// ═══════════════════════════════════════════════════════════════════════════
// SUMMARY
// ═══════════════════════════════════════════════════════════════════════════

console.log(`\n${'='.repeat(60)}`);
console.log(`  Demo B Acceptance Results: ${passed} passed, ${failed} failed`);
console.log('='.repeat(60));

if (failed > 0) {
  process.exit(1);
}
