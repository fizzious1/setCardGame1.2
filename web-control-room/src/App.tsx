import React, { useState, useMemo, useCallback } from 'react';
import { useSimulation } from './hooks/useSimulation';
import { usePlayback } from './hooks/usePlayback';
import type { PlaybackAPI } from './hooks/usePlayback';
import { PlaybackControls } from './components/PlaybackControls';
import { ParticipantList } from './components/ParticipantList';
import { HouseView } from './components/HouseView';
import { Timeline } from './components/Timeline';
import { ParticipantInspector } from './components/ParticipantInspector';
import { DailyRecap } from './components/DailyRecap';
import { EventPopup } from './components/EventPopup';

type RightTab = 'inspector' | 'recap';

const App: React.FC = () => {
  const sim = useSimulation();
  const playback: PlaybackAPI = usePlayback(sim.maxTick);
  const [selectedParticipantId, setSelectedParticipantId] = useState<string | null>(null);
  const [rightTab, setRightTab] = useState<RightTab>('inspector');
  const [popupEventId, setPopupEventId] = useState<string | null>(null);

  const { currentTick } = playback;
  const snapshot = sim.getSnapshotAtTick(currentTick);
  const currentDay = snapshot.day;
  const currentHour = snapshot.hour;
  const states = snapshot.participantStates;

  // Selected participant data
  const selectedParticipant = selectedParticipantId
    ? sim.getParticipant(selectedParticipantId)
    : undefined;
  const selectedState = selectedParticipantId
    ? sim.getParticipantState(currentTick, selectedParticipantId)
    : undefined;

  // Recent events for the selected participant (last 24 ticks)
  const recentEvents = useMemo(() => {
    if (!selectedParticipantId) return [];
    const start = Math.max(0, currentTick - 24);
    return sim
      .getEventsInRange(start, currentTick)
      .filter((e) => e.participantIds.includes(selectedParticipantId))
      .reverse();
  }, [sim, currentTick, selectedParticipantId]);

  // Daily recap
  const recap = sim.getDailyRecap(currentDay);

  // Event popup
  const popupEvent = popupEventId ? sim.getEventById(popupEventId) : undefined;

  const handleEventClick = useCallback((eventId: string) => {
    setPopupEventId(eventId);
  }, []);

  const handleClosePopup = useCallback(() => {
    setPopupEventId(null);
  }, []);

  const handleSelectParticipant = useCallback((id: string) => {
    setSelectedParticipantId((prev) => (prev === id ? null : id));
    setRightTab('inspector');
  }, []);

  const handleRecapDayChange = useCallback(
    (day: number) => {
      // Scrub to start of that day
      const dayTick = (day - 1) * sim.season.ticksPerDay;
      playback.scrubTo(dayTick);
    },
    [sim.season.ticksPerDay, playback],
  );

  const handleTimelineEventClick = useCallback(
    (eventId: string) => {
      const evt = sim.getEventById(eventId);
      if (evt) {
        playback.scrubTo(evt.tick);
        setPopupEventId(eventId);
      }
    },
    [sim, playback],
  );

  return (
    <div className="app-layout">
      {/* Top Bar */}
      <div className="top-bar">
        <div className="top-bar-logo">
          <svg width="20" height="20" viewBox="0 0 20 20" fill="none">
            <circle cx="10" cy="10" r="9" stroke="#3b82f6" strokeWidth="1.5" />
            <circle cx="10" cy="10" r="4" fill="#3b82f6" />
            <circle cx="10" cy="10" r="1.5" fill="#0a0e17" />
          </svg>
          Control Room
        </div>
        <div className="top-bar-divider" />
        <PlaybackControls playback={playback} day={currentDay} hour={currentHour} />
      </div>

      {/* Left Sidebar */}
      <ParticipantList
        participants={sim.season.participants}
        states={states}
        selectedId={selectedParticipantId}
        onSelect={handleSelectParticipant}
      />

      {/* Center */}
      <div className="center-area">
        <div className="house-view-container">
          <HouseView
            rooms={sim.season.houseLayout}
            participants={sim.season.participants}
            states={states}
            selectedParticipantId={selectedParticipantId}
            onSelectParticipant={handleSelectParticipant}
          />
        </div>
        <div className="timeline-container">
          <Timeline
            currentTick={currentTick}
            maxTick={sim.maxTick}
            ticksPerDay={sim.season.ticksPerDay}
            totalDays={sim.season.totalDays}
            events={sim.season.events}
            highlights={sim.season.highlights}
            onScrub={playback.scrubTo}
            onEventClick={handleTimelineEventClick}
          />
        </div>
      </div>

      {/* Right Sidebar */}
      <div className="right-sidebar">
        <div className="right-sidebar-tabs">
          <button
            className={`right-sidebar-tab${rightTab === 'inspector' ? ' active' : ''}`}
            onClick={() => setRightTab('inspector')}
          >
            Inspector
          </button>
          <button
            className={`right-sidebar-tab${rightTab === 'recap' ? ' active' : ''}`}
            onClick={() => setRightTab('recap')}
          >
            Day Recap
          </button>
        </div>
        <div className="right-sidebar-content">
          {rightTab === 'inspector' && selectedParticipant && selectedState ? (
            <ParticipantInspector
              participant={selectedParticipant}
              state={selectedState}
              allParticipants={sim.season.participants}
              recentEvents={recentEvents}
              onEventClick={handleEventClick}
            />
          ) : rightTab === 'inspector' ? (
            <div className="inspector-empty">
              Select a participant from the<br />sidebar or house view to inspect.
            </div>
          ) : (
            <DailyRecap
              recap={recap}
              currentDay={currentDay}
              totalDays={sim.season.totalDays}
              participants={sim.season.participants}
              getEventById={sim.getEventById}
              onDayChange={handleRecapDayChange}
              onEventClick={handleEventClick}
            />
          )}
        </div>
      </div>

      {/* Event Popup */}
      {popupEvent && (
        <EventPopup
          event={popupEvent}
          participants={sim.season.participants}
          onClose={handleClosePopup}
        />
      )}
    </div>
  );
};

export default App;
