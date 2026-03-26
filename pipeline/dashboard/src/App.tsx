import React from 'react';
import { PipelineState } from './types';
import { usePolling } from './hooks/usePolling';
import { mockData } from './mock-data';
import PipelineProgress from './components/PipelineProgress';
import CostTracker from './components/CostTracker';
import AgentGrid from './components/AgentGrid';
import ValidationResults from './components/ValidationResults';
import HealthMonitor from './components/HealthMonitor';
import ActivityFeed from './components/ActivityFeed';

const App: React.FC = () => {
  const { data, error, loading } = usePolling<PipelineState>('/api/state', 5000);

  const state = data ?? mockData;
  const isLive = data !== null && error === null;
  const isMock = data === null && !loading;

  if (loading && !data) {
    return (
      <div className="loading-overlay">
        <div className="loading-spinner" />
        Connecting to pipeline...
      </div>
    );
  }

  return (
    <div className="dashboard">
      <div className="dashboard-header">
        <h1>
          <span>BB</span> Conditioning Pipeline
        </h1>
        <div className="connection-status">
          <div className={`connection-dot ${isLive ? 'live' : isMock ? 'mock' : 'error'}`} />
          {isLive ? 'LIVE' : isMock ? 'DEMO (mock data)' : 'ERROR'}
          {error && !data && <span style={{ color: 'var(--red)', marginLeft: 8 }}>{error}</span>}
        </div>
      </div>

      <div className="dashboard-grid">
        <div className="dashboard-column">
          <PipelineProgress progress={state.progress} />
          <HealthMonitor health={state.health} />
        </div>
        <div className="dashboard-column">
          <CostTracker costs={state.costs} />
          <ActivityFeed events={state.events} agents={state.agents} />
        </div>
        <div className="dashboard-column">
          <AgentGrid agents={state.agents} />
          <ValidationResults results={state.validation_results} agents={state.agents} />
        </div>
      </div>
    </div>
  );
};

export default App;
