import React, { useState, useMemo } from 'react';
import { ValidationResult, AgentPersona } from '../types';

const BENCHMARKS = [
  { key: 'persona_consistency', label: 'PC', threshold: 80, invert: false },
  { key: 'behavioral_distinctiveness', label: 'BD', threshold: 30, invert: true },
  { key: 'strategic_depth', label: 'SD', threshold: 60, invert: false },
  { key: 'emotional_authenticity', label: 'EA', threshold: 60, invert: false },
  { key: 'anti_drift', label: 'AD', threshold: 75, invert: false },
];

type SortKey = 'name' | 'PC' | 'BD' | 'SD' | 'EA' | 'AD' | 'status';

interface AgentRow {
  id: string;
  name: string;
  scores: Record<string, number | null>;
  allPassed: boolean;
  anyFailed: boolean;
  tested: boolean;
}

interface Props {
  results: ValidationResult[];
  agents: AgentPersona[];
}

const ValidationResults: React.FC<Props> = ({ results, agents }) => {
  const [sortKey, setSortKey] = useState<SortKey>('name');
  const [sortAsc, setSortAsc] = useState(true);

  const agentMap = useMemo(() => {
    const m = new Map<string, string>();
    agents.forEach((a) => m.set(a.id, a.tier1.name));
    return m;
  }, [agents]);

  const rows: AgentRow[] = useMemo(() => {
    return agents.map((agent) => {
      const agentResults = results.filter((r) => r.agent_id === agent.id);
      const scores: Record<string, number | null> = {};
      let allPassed = agentResults.length > 0;
      let anyFailed = false;

      BENCHMARKS.forEach(({ key }) => {
        const r = agentResults.find((ar) => ar.benchmark === key);
        scores[key] = r ? r.score : null;
        if (r && !r.passed) {
          allPassed = false;
          anyFailed = true;
        }
      });

      if (agentResults.length === 0) allPassed = false;

      return {
        id: agent.id,
        name: agent.tier1.name,
        scores,
        allPassed,
        anyFailed,
        tested: agentResults.length > 0,
      };
    });
  }, [agents, results]);

  const sortedRows = useMemo(() => {
    const sorted = [...rows];
    sorted.sort((a, b) => {
      let cmp = 0;
      if (sortKey === 'name') {
        cmp = a.name.localeCompare(b.name);
      } else if (sortKey === 'status') {
        const aVal = a.allPassed ? 2 : a.anyFailed ? 0 : 1;
        const bVal = b.allPassed ? 2 : b.anyFailed ? 0 : 1;
        cmp = aVal - bVal;
      } else {
        const benchKey = BENCHMARKS.find((bm) => bm.label === sortKey)?.key ?? '';
        const aScore = a.scores[benchKey] ?? -1;
        const bScore = b.scores[benchKey] ?? -1;
        cmp = aScore - bScore;
      }
      return sortAsc ? cmp : -cmp;
    });
    return sorted;
  }, [rows, sortKey, sortAsc]);

  const passCount = rows.filter((r) => r.allPassed).length;
  const testedCount = rows.filter((r) => r.tested).length;

  const handleSort = (key: SortKey) => {
    if (sortKey === key) {
      setSortAsc(!sortAsc);
    } else {
      setSortKey(key);
      setSortAsc(true);
    }
  };

  const arrow = (key: SortKey) => {
    if (sortKey !== key) return '';
    return sortAsc ? ' \u25B2' : ' \u25BC';
  };

  return (
    <div className="card">
      <div className="card-title">Validation Results</div>

      <div className="validation-summary">
        <div className="validation-pass-rate" style={{ color: passCount === testedCount && testedCount > 0 ? 'var(--green)' : 'var(--yellow)' }}>
          {passCount}/{agents.length}
        </div>
        <div className="validation-pass-label">
          agents passed all benchmarks
          {testedCount < agents.length && (
            <span style={{ display: 'block', fontSize: 10, color: 'var(--text-muted)' }}>
              ({agents.length - testedCount} not yet tested)
            </span>
          )}
        </div>
      </div>

      <div style={{ overflowX: 'auto' }}>
        <table className="validation-table">
          <thead>
            <tr>
              <th onClick={() => handleSort('name')}>Agent{arrow('name')}</th>
              {BENCHMARKS.map(({ label }) => (
                <th key={label} onClick={() => handleSort(label as SortKey)}>
                  {label}{arrow(label as SortKey)}
                </th>
              ))}
              <th onClick={() => handleSort('status')}>Status{arrow('status')}</th>
            </tr>
          </thead>
          <tbody>
            {sortedRows.map((row) => {
              const needsReconditioning = row.anyFailed;
              return (
                <tr key={row.id} className={needsReconditioning ? 'needs-reconditioning' : ''}>
                  <td>{row.name}</td>
                  {BENCHMARKS.map(({ key, threshold, invert }) => {
                    const score = row.scores[key];
                    if (score === null) {
                      return <td key={key} className="score-cell na">--</td>;
                    }
                    const passes = invert ? score < threshold : score >= threshold;
                    return (
                      <td key={key} className={`score-cell ${passes ? 'pass' : 'fail'}`}>
                        {score}%
                      </td>
                    );
                  })}
                  <td className={`status-cell ${row.allPassed ? 'pass' : row.anyFailed ? 'fail' : 'pending'}`}>
                    {row.allPassed ? 'PASS' : row.anyFailed ? 'FAIL' : '--'}
                  </td>
                </tr>
              );
            })}
          </tbody>
        </table>
      </div>
    </div>
  );
};

export default ValidationResults;
