import { useCallback, useEffect, useRef, useState } from "react";

export type OperationMode = "manual" | "ai";

export type StationState = {
  connected: boolean;
  pumpOn: boolean;
  mode: OperationMode;
  tds: number;
  pressure: number;
  flowRate: number;
  level: number;
  temperature: number;
  updatedAt: number;
};

const initialState: StationState = {
  connected: false,
  pumpOn: false,
  mode: "ai",
  tds: 0,
  pressure: 0,
  flowRate: 0,
  level: 0,
  temperature: 0,
  updatedAt: 0,
};

function wsUrl() {
  const protocol = window.location.protocol === "https:" ? "wss:" : "ws:";
  return `${protocol}//${window.location.host}/ws`;
}

function simulatedState(previous: StationState): StationState {
  const now = Date.now();
  const phase = now / 1000;
  return {
    ...previous,
    connected: false,
    tds: Math.round(285 + Math.sin(phase / 2) * 20),
    pressure: Number((2.7 + Math.sin(phase / 3) * 0.35).toFixed(1)),
    flowRate: Number((7.5 + Math.cos(phase / 2.5) * 1.8).toFixed(1)),
    level: Math.round(55 + Math.sin(phase / 4) * 18),
    temperature: Number((22 + Math.cos(phase / 5) * 1.2).toFixed(1)),
    updatedAt: now,
  };
}

export function useWaterStation() {
  const socketRef = useRef<WebSocket | null>(null);
  const lastMessageAtRef = useRef(0);
  const [state, setState] = useState<StationState>(initialState);
  const [transport, setTransport] = useState<"websocket" | "simulation">("simulation");

  useEffect(() => {
    let closedByEffect = false;
    let reconnectTimer: number | undefined;
    let simulationTimer: number | undefined;
    let staleConnectionTimer: number | undefined;

    const startSimulation = () => {
      window.clearInterval(simulationTimer);
      setTransport("simulation");
      simulationTimer = window.setInterval(() => {
        setState((previous) => simulatedState(previous));
      }, 1000);
    };

    const connect = () => {
      try {
        const socket = new WebSocket(wsUrl());
        socketRef.current = socket;

        socket.onopen = () => {
          lastMessageAtRef.current = Date.now();
          window.clearInterval(simulationTimer);
          setTransport("websocket");
          setState((previous) => ({ ...previous, connected: true }));
        };

        socket.onmessage = (event) => {
          lastMessageAtRef.current = Date.now();
          const payload = JSON.parse(event.data) as Partial<StationState>;
          setState((previous) => ({
            ...previous,
            ...payload,
            connected: true,
            updatedAt: Date.now(),
          }));
        };

        socket.onclose = () => {
          if (closedByEffect) return;
          setState((previous) => ({ ...previous, connected: false }));
          startSimulation();
          reconnectTimer = window.setTimeout(connect, 2000);
        };

        socket.onerror = () => socket.close();
      } catch {
        startSimulation();
      }
    };

    connect();
    startSimulation();
    staleConnectionTimer = window.setInterval(() => {
      const socket = socketRef.current;
      if (
        socket?.readyState === WebSocket.OPEN &&
        lastMessageAtRef.current > 0 &&
        Date.now() - lastMessageAtRef.current > 3500
      ) {
        socket.close();
      }
    }, 1000);

    return () => {
      closedByEffect = true;
      window.clearTimeout(reconnectTimer);
      window.clearInterval(simulationTimer);
      window.clearInterval(staleConnectionTimer);
      socketRef.current?.close();
    };
  }, []);

  const sendControl = useCallback((patch: Partial<Pick<StationState, "pumpOn" | "mode">>) => {
    setState((previous) => ({ ...previous, ...patch }));

    const socket = socketRef.current;
    if (socket?.readyState === WebSocket.OPEN) {
      socket.send(JSON.stringify({ type: "control", ...patch }));
    }
  }, []);

  return { state, transport, sendControl };
}
