import { useState, useCallback, useRef, useEffect } from 'react';
import type { PlaybackState } from '../types';

export interface PlaybackAPI extends PlaybackState {
  play: () => void;
  pause: () => void;
  togglePlay: () => void;
  setSpeed: (speed: number) => void;
  stepForward: () => void;
  stepBackward: () => void;
  scrubTo: (tick: number) => void;
}

export function usePlayback(maxTick: number): PlaybackAPI {
  const [isPlaying, setIsPlaying] = useState(false);
  const [speed, setSpeedState] = useState(1);
  const [currentTick, setCurrentTick] = useState(0);
  const intervalRef = useRef<ReturnType<typeof setInterval> | null>(null);
  const speedRef = useRef(speed);
  const isPlayingRef = useRef(isPlaying);

  speedRef.current = speed;
  isPlayingRef.current = isPlaying;

  const clearTimer = useCallback(() => {
    if (intervalRef.current !== null) {
      clearInterval(intervalRef.current);
      intervalRef.current = null;
    }
  }, []);

  const startTimer = useCallback(() => {
    clearTimer();
    intervalRef.current = setInterval(() => {
      setCurrentTick((prev) => {
        const next = prev + 1;
        if (next > maxTick) {
          setIsPlaying(false);
          return maxTick;
        }
        return next;
      });
    }, 1000 / speedRef.current);
  }, [clearTimer, maxTick]);

  useEffect(() => {
    if (isPlaying) {
      startTimer();
    } else {
      clearTimer();
    }
    return clearTimer;
  }, [isPlaying, speed, startTimer, clearTimer]);

  const play = useCallback(() => {
    setCurrentTick((prev) => {
      if (prev >= maxTick) return 0;
      return prev;
    });
    setIsPlaying(true);
  }, [maxTick]);

  const pause = useCallback(() => {
    setIsPlaying(false);
  }, []);

  const togglePlay = useCallback(() => {
    setIsPlaying((prev) => {
      if (!prev) {
        setCurrentTick((t) => (t >= maxTick ? 0 : t));
      }
      return !prev;
    });
  }, [maxTick]);

  const setSpeed = useCallback((s: number) => {
    setSpeedState(s);
  }, []);

  const stepForward = useCallback(() => {
    setIsPlaying(false);
    setCurrentTick((prev) => Math.min(prev + 1, maxTick));
  }, [maxTick]);

  const stepBackward = useCallback(() => {
    setIsPlaying(false);
    setCurrentTick((prev) => Math.max(prev - 1, 0));
  }, []);

  const scrubTo = useCallback(
    (tick: number) => {
      setCurrentTick(Math.max(0, Math.min(tick, maxTick)));
    },
    [maxTick],
  );

  return {
    isPlaying,
    speed,
    currentTick,
    maxTick,
    play,
    pause,
    togglePlay,
    setSpeed,
    stepForward,
    stepBackward,
    scrubTo,
  };
}
