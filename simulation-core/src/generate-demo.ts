import * as fs from 'fs';
import * as path from 'path';
import { runSimulation } from './engine';

function main(): void {
  console.log('Starting Big Brother AI Simulation...');
  console.log('Seed: 42 | Days: 7 | Ticks per day: 24');
  console.log('');

  const season = runSimulation({
    totalDays: 7,
    ticksPerDay: 24,
    seed: 42,
    seasonName: 'Season 1: House of Rivals',
  });

  // Validation
  console.log('=== Simulation Complete ===');
  console.log(`Participants: ${season.participants.length}`);
  console.log(`Total snapshots: ${season.snapshots.length}`);
  console.log(`Total events: ${season.events.length}`);
  console.log(`Daily recaps: ${season.dailyRecaps.length}`);
  console.log(`Highlights: ${season.highlights.length}`);
  console.log('');

  // Count event types
  const eventCounts: Record<string, number> = {};
  for (const event of season.events) {
    eventCounts[event.type] = (eventCounts[event.type] || 0) + 1;
  }
  console.log('Event breakdown:');
  for (const [type, count] of Object.entries(eventCounts).sort((a, b) => b[1] - a[1])) {
    console.log(`  ${type}: ${count}`);
  }
  console.log('');

  // Show daily drama scores
  console.log('Daily drama scores:');
  for (const recap of season.dailyRecaps) {
    console.log(`  Day ${recap.day}: ${recap.dramaScore}/10 - ${recap.summary.substring(0, 80)}...`);
  }
  console.log('');

  // Show highlights
  console.log('Top highlights:');
  for (const h of season.highlights.slice(0, 10)) {
    console.log(`  [Day ${h.day}] ${h.label} (importance: ${h.importance}, ${h.category})`);
  }
  console.log('');

  // Validate requirements
  const allianceCount = season.events.filter((e) => e.type === 'alliance_formed').length;
  const conflictCount = season.events.filter((e) => e.type === 'conflict').length;
  const betrayalCount = season.events.filter((e) => e.type === 'betrayal').length;
  const nominationCount = season.events.filter((e) => e.type === 'nomination').length;
  const evictionCount = season.events.filter((e) => e.type === 'eviction').length;

  console.log('=== Requirement Checks ===');
  console.log(`Participants: ${season.participants.length} (need 8) ${season.participants.length >= 8 ? 'PASS' : 'FAIL'}`);
  console.log(`Snapshots: ${season.snapshots.length} (need 168) ${season.snapshots.length >= 168 ? 'PASS' : 'FAIL'}`);
  console.log(`Events: ${season.events.length} (need 50+) ${season.events.length >= 50 ? 'PASS' : 'FAIL'}`);
  console.log(`Alliances: ${allianceCount} (need 2+) ${allianceCount >= 2 ? 'PASS' : 'FAIL'}`);
  console.log(`Conflicts: ${conflictCount} (need 2+) ${conflictCount >= 2 ? 'PASS' : 'FAIL'}`);
  console.log(`Betrayals: ${betrayalCount} (need 1+) ${betrayalCount >= 1 ? 'PASS' : 'FAIL'}`);
  console.log(`Nominations: ${nominationCount} (need 2+) ${nominationCount >= 2 ? 'PASS' : 'FAIL'}`);
  console.log(`Evictions: ${evictionCount} (need 1) ${evictionCount >= 1 ? 'PASS' : 'FAIL'}`);
  console.log(`Recaps: ${season.dailyRecaps.length} (need 7) ${season.dailyRecaps.length >= 7 ? 'PASS' : 'FAIL'}`);
  console.log(`Highlights: ${season.highlights.length} (need 10+) ${season.highlights.length >= 10 ? 'PASS' : 'FAIL'}`);

  // Write output
  const outputPath = path.resolve(__dirname, '../../shared/demo-season.json');
  const jsonOutput = JSON.stringify(season, null, 2);
  fs.writeFileSync(outputPath, jsonOutput, 'utf-8');
  console.log('');
  console.log(`Output written to: ${outputPath}`);
  console.log(`File size: ${(Buffer.byteLength(jsonOutput) / 1024).toFixed(1)} KB`);
}

main();
