"use client";

import PageHeader from "@/_components/fixed-components/PageHeader/PageHeader";
import { useWaterStation } from "@/lib/use-water-station";

export default function Prototype() {
  const { state, transport, sendControl } = useWaterStation();
  const progress = Math.min(Math.max(state.level, 0), 100);

  function getFlowStatus() {
    if (state.flowRate > 8) return { text: "Flow is stable", color: "#22c55e" };
    if (state.flowRate > 4) return { text: "Flow is moderate", color: "#f59e0b" };
    return { text: "Low flow detected", color: "#ef4444" };
  }

  const flowStatus = getFlowStatus();

  return (
    <>
      <PageHeader />
      <section
        className="min-h-screen px-5 py-10"
        style={{ background: "linear-gradient(135deg, #cfd9f7 0%, #d8dff9 100%)" }}
      >
      <div className="max-w-5xl mx-auto">

        <div className="flex justify-between items-start mb-8">
          <div>
            <h2 className="text-5xl font-bold mb-1" style={{ color: "#4169b3" }}>
              Prototype Demo
            </h2>
            <p className="text-gray-800 text-lg">
              Live ESP32-S3 Access Point demo
            </p>
          </div>
          <button
            onClick={() => window.history.back()}
            className="text-3xl p-2 transition-transform hover:-translate-x-1"
            style={{ color: "#4169b3", background: "none", border: "none" }}
          >
            &larr;
          </button>
        </div>

        <div className="rounded-2xl p-5 mb-5" style={{ backgroundColor: "#c7d8f5" }}>
          <div className="flex justify-between items-center flex-wrap gap-4">
            <div>
              <p className="text-sm font-bold text-gray-800 mb-3">Connection:</p>
              <div className="flex gap-2">
                <span
                  className="px-4 py-2 rounded-lg font-medium"
                  style={{
                    backgroundColor: transport === "websocket" ? "#4169b3" : "#d8c3f7",
                    color: transport === "websocket" ? "white" : "#2d3a7a",
                  }}
                >
                  {transport === "websocket" ? "Connected" : "Simulation"}
                </span>
              </div>
            </div>

            <div className="flex items-center gap-3">
              <div>
                <span
                  className="w-3 h-3 rounded-full inline-block mr-2"
                  style={{ backgroundColor: transport === "websocket" ? "#22c55e" : "#f59e0b" }}
                />
                <span className="text-sm font-semibold">
                  {transport === "websocket" ? "Connected" : "Waiting for ESP32"}
                </span>
              </div>
              <button
                onClick={() => sendControl({ pumpOn: !state.pumpOn })}
                className="px-4 py-2 rounded-lg font-medium transition-all"
                style={{
                  backgroundColor: state.pumpOn ? "#ef4444" : "#22c55e",
                  color: "white",
                  border: "none",
                  cursor: "pointer",
                }}
              >
                {state.pumpOn ? "Stop Pump" : "Start Pump"}
              </button>
            </div>
          </div>
        </div>

        <div className="rounded-2xl p-8 mb-5" style={{ backgroundColor: "#c7d8f5" }}>
          <h3 className="text-2xl font-semibold mb-5 flex items-center gap-3" style={{ color: "#4169b3" }}>
            Sensors Reading
          </h3>
          <div className="space-y-3">
            {[
              { label: "TDS Sensor:", value: `${Math.round(state.tds)} ppm` },
              { label: "Pressure Sensor:", value: `${state.pressure.toFixed(1)} bar` },
              { label: "Flow Rate:", value: `${state.flowRate.toFixed(1)} L/min` },
              { label: "Temperature:", value: `${state.temperature.toFixed(1)} C` },
            ].map(({ label, value }) => (
              <p key={label} className="text-lg">
                <span className="font-medium" style={{ color: "#4169b3" }}>{label}</span>
                <span className="ml-2 font-semibold text-gray-800">{value}</span>
              </p>
            ))}
          </div>
        </div>

        <div className="rounded-2xl p-8" style={{ backgroundColor: "#c7d8f5" }}>
          <h3 className="text-2xl font-semibold mb-5" style={{ color: "#4169b3" }}>
            Water Flow Simulation
          </h3>
          <div className="w-full h-8 bg-gray-400 rounded-full overflow-hidden mb-4">
            <div
              className="h-full rounded-full transition-all duration-500"
              style={{ width: `${progress}%`, backgroundColor: "#4169b3" }}
            />
          </div>
          <div className="flex justify-between items-center">
            <span className="text-base" style={{ color: flowStatus.color }}>
              {flowStatus.text}
            </span>
            <span className="text-5xl font-bold" style={{ color: "#4169b3" }}>
              {Math.round(progress)}%
            </span>
          </div>
        </div>

      </div>
    </section>
    </>
  );
}
