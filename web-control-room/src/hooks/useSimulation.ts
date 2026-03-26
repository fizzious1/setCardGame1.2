import { useMemo, useCallback } from 'react';
import { demoSeason } from '../data/demo-season';
import type {
  Season,
  Snapshot,
  GameEvent,
  ParticipantState,
  DailyRecap,
  Participant,
} from '../types';

export interface SimulationAPI {
  season: Season;
  maxTick: number;
  getSnapshotAtTick: (tick: number) => Snapshot;
  getEventsInRange: (startTick: number, endTick: number) => GameEvent[];
  getParticipantState: (tick: number, participantId: string) => ParticipantState | undefined;
  getParticipantStates: (tick: number) => ParticipantState[];
  getDailyRecap: (day: number) => DailyRecap | undefined;
  getEventById: (id: string) => GameEvent | undefined;
  getParticipant: (id: string) => Participant | undefined;
  getDayForTick: (tick: number) => number;
}

export function useSimulation(): SimulationAPI {
  const season = demoSeason;

  const maxTick = useMemo(() => {
    if (season.snapshots.length === 0) return 0;
    return season.snapshots[season.snapshots.length - 1].tick;
  }, [season]);

  const snapshotMap = useMemo(() => {
    const map = new Map<number, Snapshot>();
    for (const s of season.snapshots) {
      map.set(s.tick, s);
    }
    return map;
  }, [season]);

  const eventMap = useMemo(() => {
    const map = new Map<string, GameEvent>();
    for (const e of season.events) {
      map.set(e.id, e);
    }
    return map;
  }, [season]);

  const participantMap = useMemo(() => {
    const map = new Map<string, Participant>();
    for (const p of season.participants) {
      map.set(p.id, p);
    }
    return map;
  }, [season]);

  const getSnapshotAtTick = useCallback(
    (tick: number): Snapshot => {
      const exact = snapshotMap.get(tick);
      if (exact) return exact;
      // Find the closest snapshot <= tick
      let best: Snapshot = season.snapshots[0];
      for (const s of season.snapshots) {
        if (s.tick <= tick) best = s;
        else break;
      }
      return best;
    },
    [snapshotMap, season],
  );

  const getEventsInRange = useCallback(
    (startTick: number, endTick: number): GameEvent[] => {
      return season.events.filter((e) => e.tick >= startTick && e.tick <= endTick);
    },
    [season],
  );

  const getParticipantStates = useCallback(
    (tick: number): ParticipantState[] => {
      return getSnapshotAtTick(tick).participantStates;
    },
    [getSnapshotAtTick],
  );

  const getParticipantState = useCallback(
    (tick: number, participantId: string): ParticipantState | undefined => {
      return getParticipantStates(tick).find((ps) => ps.participantId === participantId);
    },
    [getParticipantStates],
  );

  const getDailyRecap = useCallback(
    (day: number): DailyRecap | undefined => {
      return season.dailyRecaps.find((r) => r.day === day);
    },
    [season],
  );

  const getEventById = useCallback(
    (id: string): GameEvent | undefined => {
      return eventMap.get(id);
    },
    [eventMap],
  );

  const getParticipant = useCallback(
    (id: string): Participant | undefined => {
      return participantMap.get(id);
    },
    [participantMap],
  );

  const getDayForTick = useCallback(
    (tick: number): number => {
      const snap = getSnapshotAtTick(tick);
      return snap.day;
    },
    [getSnapshotAtTick],
  );

  return {
    season,
    maxTick,
    getSnapshotAtTick,
    getEventsInRange,
    getParticipantState,
    getParticipantStates,
    getDailyRecap,
    getEventById,
    getParticipant,
    getDayForTick,
  };
}
