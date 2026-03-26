import React from 'react';
import type { PlaybackAPI } from '../hooks/usePlayback';

interface Props {
  playback: PlaybackAPI;
  day: number;
  hour: number;
}

const SPEEDS = [1, 2, 4, 8];

export const PlaybackControls: React.FC<Props> = ({ playback, day, hour }) => {
  const {
    isPlaying,
    speed,
    currentTick,
    maxTick,
    togglePlay,
    setSpeed,
    stepForward,
    stepBackward,
  } = playback;

  const formatTime = (h: number): string => {
    const hh = Math.floor(h) % 24;
    return `${hh.toString().padStart(2, '0')}:00`;
  };

  return (
    <div className="playback-controls">
      <div className="playback-left">
        <button
          className="playback-btn"
          onClick={stepBackward}
          title="Step backward"
        >
          <svg width="16" height="16" viewBox="0 0 16 16" fill="currentColor">
            <rect x="2" y="3" width="2" height="10" />
            <polygon points="12,3 5,8 12,13" />
          </svg>
        </button>
        <button
          className="playback-btn playback-btn-play"
          onClick={togglePlay}
          title={isPlaying ? 'Pause' : 'Play'}
        >
          {isPlaying ? (
            <svg width="20" height="20" viewBox="0 0 20 20" fill="currentColor">
              <rect x="4" y="3" width="4" height="14" rx="1" />
              <rect x="12" y="3" width="4" height="14" rx="1" />
            </svg>
          ) : (
            <svg width="20" height="20" viewBox="0 0 20 20" fill="currentColor">
              <polygon points="4,2 18,10 4,18" />
            </svg>
          )}
        </button>
        <button
          className="playback-btn"
          onClick={stepForward}
          title="Step forward"
        >
          <svg width="16" height="16" viewBox="0 0 16 16" fill="currentColor">
            <polygon points="4,3 11,8 4,13" />
            <rect x="12" y="3" width="2" height="10" />
          </svg>
        </button>
      </div>
      <div className="playback-center">
        <span className="playback-time">
          Day {day} / {formatTime(hour)}
        </span>
        <span className="playback-tick">
          Tick {currentTick} / {maxTick}
        </span>
      </div>
      <div className="playback-right">
        <div className="speed-group">
          {SPEEDS.map((s) => (
            <button
              key={s}
              className={`speed-btn ${speed === s ? 'speed-btn-active' : ''}`}
              onClick={() => setSpeed(s)}
            >
              {s}x
            </button>
          ))}
        </div>
      </div>
    </div>
  );
};
