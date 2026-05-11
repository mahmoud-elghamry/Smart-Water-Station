'use client';
import React, { useEffect, useState } from 'react';
import { AlertCircle, TrendingUp } from 'lucide-react';
import Dashboard from '@/_components/fixed-components/Dashboard/Dashboard';
import { useWaterStation } from '@/lib/use-water-station';
import { SimpleLineChart } from '@/_components/light-charts';

export default function HomePage() {
  const [isDashboardOpen, setIsDashboardOpen] = useState(false);
  const { state, transport } = useWaterStation();
  const [tdsHistory, setTdsHistory] = useState(() =>
    Array.from({ length: 12 }, (_, index) => ({
      label: `${index + 1}`,
      value: 280,
    }))
  );
  const pressureWarning = state.pressure > 3.0;

  useEffect(() => {
    if (!state.updatedAt) return;

    setTdsHistory((previous) => [
      ...previous.slice(-11),
      {
        label: new Date().toLocaleTimeString([], { minute: "2-digit", second: "2-digit" }),
        value: Math.round(state.tds),
      },
    ]);
  }, [state.updatedAt, state.tds]);

  return (
    <>
      <Dashboard isOpen={isDashboardOpen} onClose={() => setIsDashboardOpen(false)} />
      <div className="w-full min-h-screen bg-[#d8e2f4] p-6">
      <div className="flex justify-between items-center mb-8">
        <div>
          <h1 className="text-4xl font-bold" style={{ color: '#4169b3' }}>Main Analytics</h1>
          <p className="text-sm mt-1" style={{ color: '#4169b3' }}>
            {transport === "websocket" ? "Connected" : "Waiting for ESP32"}
          </p>
        </div>
        <button
          onClick={() => setIsDashboardOpen(true)}
          className="bg-blue-800 text-white hover:bg-blue-500 px-6 py-2 rounded-lg font-semibold shadow-md transition-all "
          style={{  border: '2px solid #4169b3' }}
        >
          View Dashboard
        </button>
      </div>

      <div className={`mb-6 border rounded-lg p-4 flex items-center gap-3 ${pressureWarning ? "bg-red-50 border-red-200" : "bg-green-50 border-green-200"}`}>
        <AlertCircle className={`w-6 h-6 ${pressureWarning ? "text-red-600" : "text-green-600"}`} />
        <p className="font-semibold" style={{ color: '#4169b3' }}>
          {pressureWarning ? "Warning: water pressure is above the safe test threshold." : "System telemetry is stable."}
        </p>
      </div>

      <div className="grid grid-cols-1 md:grid-cols-3 gap-6 mb-8">
        <div className="bg-white rounded-lg shadow p-6">
          <h3 className="text-sm font-semibold mb-2" style={{ color: '#4169b3' }}>Water Quality (TDS)</h3>
          <div className="text-4xl font-bold mb-2" style={{ color: '#4169b3' }}>{Math.round(state.tds)} ppm</div>
          <div className="flex items-center gap-1 text-sm" style={{ color: '#4169b3' }}>
            <TrendingUp className="w-4 h-4" />
            <span>Live</span>
          </div>
        </div>

        <div className="bg-white rounded-lg shadow p-6">
          <h3 className="text-sm font-semibold mb-2" style={{ color: '#4169b3' }}>Water Temperature</h3>
          <div className="text-4xl font-bold mb-2" style={{ color: '#4169b3' }}>{state.temperature.toFixed(1)} C</div>
          <div className="text-sm" style={{ color: '#4169b3' }}>Optimal Range</div>
        </div>

        <div className="bg-white rounded-lg shadow p-6">
          <h3 className="text-sm font-semibold mb-2" style={{ color: '#4169b3' }}>Flow Rate</h3>
          <div className="text-4xl font-bold mb-2" style={{ color: '#4169b3' }}>{state.flowRate.toFixed(1)} L/m</div>
          <div className="text-sm" style={{ color: '#4169b3' }}>Live Reading</div>
        </div>
      </div>

      <div className="grid grid-cols-1 lg:grid-cols-3 gap-6">
        <div className="lg:col-span-2 bg-white rounded-lg shadow p-6">
          <div className="mb-4">
            <h2 className="text-xl font-bold mb-1" style={{ color: '#4169b3' }}>Water Quality Over Time</h2>
            <p className="text-sm" style={{ color: '#4169b3' }}>{Math.round(state.tds)} ppm <span style={{ color: '#4169b3' }}>live</span></p>
            <p className="text-xs mt-1" style={{ color: '#4169b3' }}>Last 24 Hours</p>
          </div>
          <SimpleLineChart data={tdsHistory} />
        </div>

        <div className="bg-white rounded-lg shadow p-6 flex flex-col justify-center">
          <div className="flex items-center gap-2 mb-4">
            <div className="text-2xl">AI</div>
            <h3 className="text-lg font-bold" style={{ color: '#4169b3' }}>AI Forecast</h3>
          </div>
          <p className="text-center" style={{ color: '#4169b3' }}>System is predicted to operate</p>
          <p className="text-2xl font-bold text-center my-3" style={{ color: '#4169b3' }}>at 92% efficiency</p>
          <p className="text-center text-sm" style={{ color: '#4169b3' }}>for the next 6 hours based on current trends</p>
        </div>
      </div>
    </div>
    </>
  );
}
