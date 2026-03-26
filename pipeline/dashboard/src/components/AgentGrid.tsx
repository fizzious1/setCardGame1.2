import React, { useState } from 'react';
import { AgentPersona } from '../types';

const BENCHMARKS: { key: string; label: string; threshold: number; invert?: boolean }[] = [
  { key: 'persona_consistency', label: 'PC', threshold: 80 },
  { key: 'behavioral_distinctiveness', label: 'BD', threshold: 30, invert: true },
  { key: 'strategic_depth', label: 'SD', threshold: 60 },
  { key: 'emotional_authenticity', label: 'EA', threshold: 60 },
  { key: 'anti_drift', label: 'AD', threshold: 75 },
];

function statusLabel(status: string): string {
  switch (status) {
    case 'passed': return 'Passed';
    case 'in_progress': return 'In Progress';
    case 'failed': return 'Failed';
    default: return 'Not Started';
  }
}

function statusClass(status: string): string {
  switch (status) {
    case 'passed': return 'passed';
    case 'in_progress': return 'in-progress';
    case 'failed': return 'failed';
    default: return 'not-started';
  }
}

interface Props {
  agents: AgentPersona[];
}

const AgentGrid: React.FC<Props> = ({ agents }) => {
  const [expandedId, setExpandedId] = useState<string | null>(null);

  const handleClick = (id: string) => {
    setExpandedId(prev => (prev === id ? null : id));
  };

  return (
    <div className="card">
      <div className="card-title">Agent Status Grid</div>
      <div className="agent-grid">
        {agents.map((agent) => {
          const isExpanded = expandedId === agent.id;
          return (
            <div
              key={agent.id}
              className={`agent-card ${isExpanded ? 'expanded' : ''}`}
              onClick={() => handleClick(agent.id)}
            >
              <div className="agent-card-header">
                <div>
                  <div className="agent-name">{agent.tier1.name}</div>
                  <div className="agent-archetype">{agent.tier1.archetype}</div>
                </div>
                <span className={`status-badge ${statusClass(agent.conditioning_status)}`}>
                  {statusLabel(agent.conditioning_status)}
                </span>
              </div>

              <div className="validation-bars">
                {BENCHMARKS.map(({ key, label, threshold, invert }) => {
                  const score = agent.validation_scores[key];
                  const hasScore = score !== undefined;
                  let barClass = 'na';
                  if (hasScore) {
                    const passes = invert ? score < threshold : score >= threshold;
                    barClass = passes ? 'pass' : 'fail';
                  }
                  return (
                    <div className="validation-bar-row" key={key}>
                      <div className="validation-bar-label">{label}</div>
                      <div className="validation-bar-track">
                        <div
                          className={`validation-bar-fill ${barClass}`}
                          style={{ width: hasScore ? `${Math.min(score, 100)}%` : '0%' }}
                        />
                      </div>
                    </div>
                  );
                })}
              </div>

              <div className="agent-meta">
                <span>{agent.token_count > 0 ? `${(agent.token_count / 1000).toFixed(1)}K tok` : '--'}</span>
                <span>{agent.interactions_completed > 0 ? `${agent.interactions_completed} interactions` : '--'}</span>
              </div>

              {isExpanded && (
                <div className="agent-expanded-content" onClick={(e) => e.stopPropagation()}>
                  <div className="agent-detail-section">
                    <h4>Big Five Personality</h4>
                    <div className="big-five-bars">
                      {Object.entries(agent.tier1.big_five).map(([trait, val]) => (
                        <div className="big-five-row" key={trait}>
                          <div className="big-five-label">{trait.charAt(0).toUpperCase() + trait.slice(1)}</div>
                          <div className="big-five-track">
                            <div className="big-five-fill" style={{ width: `${val * 100}%` }} />
                          </div>
                          <div className="big-five-value">{(val * 100).toFixed(0)}%</div>
                        </div>
                      ))}
                    </div>
                  </div>

                  <div className="agent-detail-section">
                    <h4>Profile</h4>
                    <ul className="detail-list">
                      <li>Age: {agent.tier1.age}</li>
                      <li>{agent.tier1.hometown}</li>
                      <li>{agent.tier1.occupation}</li>
                      <li>Archetype: {agent.tier1.archetype}</li>
                      <li>Secondary: {agent.tier1.secondary_archetype}</li>
                      <li>Conflict: {agent.tier1.conflict_style}</li>
                      <li>Vocab: {agent.tier1.speech_pattern.vocabulary_level}</li>
                    </ul>
                  </div>

                  <div className="agent-detail-section">
                    <h4>Values &amp; Fears</h4>
                    <ul className="detail-list">
                      {agent.tier1.core_values.map((v) => (
                        <li key={v}>{v}</li>
                      ))}
                    </ul>
                    <h4 style={{ marginTop: 8 }}>Deepest Fears</h4>
                    <ul className="detail-list">
                      {agent.tier1.deepest_fears.map((f) => (
                        <li key={f}>{f}</li>
                      ))}
                    </ul>
                    <h4 style={{ marginTop: 8 }}>Catchphrases</h4>
                    {agent.tier1.speech_pattern.catchphrases.map((c) => (
                      <div className="catchphrase" key={c}>{c}</div>
                    ))}
                  </div>
                </div>
              )}
            </div>
          );
        })}
      </div>
    </div>
  );
};

export default AgentGrid;
