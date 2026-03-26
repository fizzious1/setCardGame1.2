export interface PipelineState {
  progress: PhaseProgress;
  costs: CostBreakdown;
  agents: AgentPersona[];
  health: HealthStatus;
  validation_results: ValidationResult[];
  events: ActivityEvent[];
}

export interface PhaseProgress {
  current_phase: number;
  current_step: string;
  phase_name: string;
  steps_completed: number;
  total_steps: number;
  progress_percent: number;
  estimated_remaining_seconds: number;
  last_action: string;
  last_action_timestamp: string;
}

export interface CostBreakdown {
  total_cost: number;
  opus_cost: number;
  sonnet_cost: number;
  embedding_cost: number;
  cost_by_phase: Record<string, number>;
  cost_by_agent: Record<string, number>;
  total_input_tokens: number;
  total_output_tokens: number;
  total_api_calls: number;
  opus_calls: number;
  sonnet_calls: number;
  avg_cost_per_opus_call: number;
  avg_cost_per_sonnet_call: number;
  estimated_total_cost: number;
  budget_remaining: number;
  cost_over_time: Array<{ timestamp: string; cumulative_cost: number; model: string }>;
}

export interface AgentPersona {
  id: string;
  tier1: {
    name: string;
    age: number;
    hometown: string;
    occupation: string;
    archetype: string;
    secondary_archetype: string;
    conflict_style: string;
    core_values: string[];
    deepest_fears: string[];
    speech_pattern: { catchphrases: string[]; vocabulary_level: string };
    big_five: {
      openness: number;
      conscientiousness: number;
      extraversion: number;
      agreeableness: number;
      neuroticism: number;
    };
  };
  conditioning_status: string;
  validation_scores: Record<string, number>;
  interactions_completed: number;
  token_count: number;
}

export interface HealthStatus {
  api_status: string;
  api_latency_ms: number;
  rate_limit_remaining: number;
  rate_limit_total: number;
  last_health_check: string;
  last_api_call: string;
  seconds_since_last_call: number;
  error_log: Array<{ timestamp: string; error: string }>;
  is_stuck: boolean;
}

export interface ValidationResult {
  agent_id: string;
  benchmark: string;
  score: number;
  passed: boolean;
  details: string;
}

export interface ActivityEvent {
  timestamp: string;
  phase: string;
  agent_id: string | null;
  event_type: string;
  message: string;
}
