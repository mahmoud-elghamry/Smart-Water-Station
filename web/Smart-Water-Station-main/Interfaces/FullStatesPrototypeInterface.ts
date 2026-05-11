export interface Root {
  connected: boolean
  esp32_connected: boolean
  esp32_last_seen: string
  state: State
  telemetry: Telemetry
  security: Security
  threat_active: boolean
}

export interface State {
  state: string
  error: string
}

export interface Telemetry {
  tds: Tds
  pressure: Pressure
  level: Level
  flow: Flow
}

export interface Tds {
  type: string
  sensor: string
  value: number
}

export interface Pressure {
  type: string
  sensor: string
  value: number
}

export interface Level {
  type: string
  sensor: string
  value: number
}

export interface Flow {
  type: string
  sensor: string
  value: number
}

export interface Security {
  faces_detected: number
  monitoring: boolean
  last_detection: any
}

export interface SensorData {
  tds: number
  pressure: number
  flowRate: number
  flowProgress: number
}
