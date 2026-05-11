'use client';
import React, { useState } from 'react';
import PageHeader from '@/_components/fixed-components/PageHeader/PageHeader';
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/_components/ui/card';
import { Progress } from '@/_components/ui/progress';
import { CheckCircle2, AlertCircle } from 'lucide-react';

export default function Reports() {
  const [selectedPeriod, setSelectedPeriod] = useState('week');

  const logData = [
    { id: 1, date: '5/9/2025', tds: 480, flow: 12.2, pressure: 5.5, status: 'normal' },
    { id: 2, date: '8/9/2025', tds: 550, flow: 14.8, pressure: 5.6, status: 'warning' },
    { id: 3, date: '10/9/2025', tds: 520, flow: 13.5, pressure: 5.4, status: 'normal' },
    { id: 4, date: '12/9/2025', tds: 490, flow: 12.8, pressure: 5.7, status: 'normal' },
    { id: 5, date: '15/9/2025', tds: 510, flow: 13.2, pressure: 5.5, status: 'normal' },
  ];

  const overallEfficiency = 97.5;

  return (
    <>
      <PageHeader />
      <section className="min-h-screen px-5 py-10" style={{ background: "linear-gradient(135deg, #cfd9f7 0%, #d8dff9 100%)" }}>
      <div className="max-w-5xl mx-auto">
        {/* Header */}
        <div className="mb-8">
          <h1 className="text-5xl font-bold mb-1" style={{ color: "#4169b3" }}>Performance Summary</h1>
          <p className="text-lg" style={{ color: "#4169b3" }}>Monitor your system's efficiency and detailed logs</p>
        </div>

        {/* Overall Efficiency Card */}
        <Card className="rounded-2xl bg-white/10 border-white/20 backdrop-blur-sm mb-5">
          <CardHeader>
            <CardTitle style={{ color: "#4169b3" }}>Overall Efficiency</CardTitle>
            <CardDescription style={{ color: "#4169b3" }}>
              Performance for the selected period
            </CardDescription>
          </CardHeader>
          <CardContent className="flex flex-col gap-6">
            <div className="flex items-end gap-4">
              <div className="text-6xl font-bold" style={{ color: "#4169b3" }}>{overallEfficiency}%</div>
              <p style={{ color: "#4169b3" }} className="mb-2">Target: 90%</p>
            </div>
            <div className="w-full">
              <Progress value={overallEfficiency} className="h-3" />
            </div>
            <p style={{ color: "#4169b3" }}>
              Performance for the selected period is well above the target threshold of 90%. Your system is operating at peak efficiency.
            </p>
          </CardContent>
        </Card>

        {/* Period Selection */}
        <div className="flex gap-3 mb-5">
          {['day', 'week', 'month', 'year'].map((period) => (
            <button
              key={period}
              onClick={() => setSelectedPeriod(period)}
              className={`px-4 py-2 rounded-lg font-medium transition-all ${
                selectedPeriod === period
                  ? 'text-white shadow-lg'
                  : 'text-white/70 hover:bg-white/20'
              }`}
              style={{
                backgroundColor: selectedPeriod === period ? '#4169b3' : 'transparent',
              }}
            >
              {period.charAt(0).toUpperCase() + period.slice(1)}
            </button>
          ))}
        </div>

        {/* Detailed Log Card */}
        <Card className="rounded-2xl bg-white/10 border-white/20 backdrop-blur-sm mb-5">
          <CardHeader>
            <CardTitle style={{ color: "#4169b3" }}>Detailed Log</CardTitle>
            <CardDescription style={{ color: "#4169b3" }}>
              Sensor readings for {selectedPeriod} period
            </CardDescription>
          </CardHeader>
          <CardContent>
            <div className="overflow-x-auto">
              <table className="w-full">
                <thead>
                  <tr className="border-b border-white/20">
                    <th className="text-left py-3 px-4 font-semibold" style={{ color: "#4169b3" }}>Date</th>
                    <th className="text-left py-3 px-4 font-semibold" style={{ color: "#4169b3" }}>TDS (PPM)</th>
                    <th className="text-left py-3 px-4 font-semibold" style={{ color: "#4169b3" }}>Flow (MMM/H)</th>
                    <th className="text-left py-3 px-4 font-semibold" style={{ color: "#4169b3" }}>Pressure (BAR)</th>
                    <th className="text-left py-3 px-4 font-semibold" style={{ color: "#4169b3" }}>Status</th>
                  </tr>
                </thead>
                <tbody>
                  {logData.map((row, index) => (
                    <tr
                      key={row.id}
                      className={`border-b border-white/10 hover:bg-white/5 transition-colors ${
                        index % 2 === 0 ? 'bg-white/5' : 'bg-transparent'
                      }`}
                    >
                      <td className="py-3 px-4" style={{ color: "#4169b3" }}>{row.date}</td>
                      <td className="py-3 px-4" style={{ color: "#4169b3" }}>{row.tds}</td>
                      <td className="py-3 px-4" style={{ color: "#4169b3" }}>{row.flow}</td>
                      <td className="py-3 px-4" style={{ color: "#4169b3" }}>{row.pressure}</td>
                      <td className="py-3 px-4">
                        <div className="flex items-center gap-2">
                          {row.status === 'normal' ? (
                            <>
                              <CheckCircle2 className="w-5 h-5 text-green-400" />
                              <span className="text-green-400 font-medium">Normal</span>
                            </>
                          ) : (
                            <>
                              <AlertCircle className="w-5 h-5 text-yellow-400" />
                              <span className="text-yellow-400 font-medium">Warning</span>
                            </>
                          )}
                        </div>
                      </td>
                    </tr>
                  ))}
                </tbody>
              </table>
            </div>
          </CardContent>
        </Card>
      </div>
    </section>
    </>
  );
}
