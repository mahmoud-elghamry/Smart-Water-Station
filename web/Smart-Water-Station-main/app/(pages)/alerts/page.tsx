'use client';
import React, { useState } from 'react';
import PageHeader from '@/_components/fixed-components/PageHeader/PageHeader';
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/_components/ui/card';
import { Bell, AlertTriangle, AlertCircle, Info, CheckCircle2, Clock, Check, X } from 'lucide-react';

export default function Alerts() {
  const [filter, setFilter] = useState('all');

  const alerts = [
    {
      id: 1,
      title: 'High Water Pressure Detected',
      message: 'Water pressure in Tank A has exceeded safe levels (65 PSI)',
      type: 'warning',
      time: '2 minutes ago',
      resolved: false,
    },
    {
      id: 2,
      title: 'pH Level Out of Range',
      message: 'pH sensor reading is 6.2, recommended range is 6.5-8.5',
      type: 'critical',
      time: '15 minutes ago',
      resolved: false,
    },
    {
      id: 3,
      title: 'Filter Maintenance Due',
      message: 'Filter system requires scheduled maintenance',
      type: 'info',
      time: '1 hour ago',
      resolved: false,
    },
    {
      id: 4,
      title: 'Sensor Calibration Complete',
      message: 'Temperature sensor calibration completed successfully',
      type: 'success',
      time: '3 hours ago',
      resolved: true,
    },
    {
      id: 5,
      title: 'Low Water Flow Detected',
      message: 'Flow rate has dropped below normal threshold',
      type: 'warning',
      time: '5 hours ago',
      resolved: true,
    },
  ];

  const getAlertIcon = (type: string) => {
    const icons: { [key: string]: React.ReactNode } = {
      warning: <AlertTriangle className="w-5 h-5" />,
      critical: <AlertCircle className="w-5 h-5" />,
      info: <Info className="w-5 h-5" />,
      success: <CheckCircle2 className="w-5 h-5" />,
    };
    return icons[type] || <Bell className="w-5 h-5" />;
  };

  const getAlertColor = (type: string) => {
    const colors: { [key: string]: string } = {
      warning: 'text-yellow-400',
      critical: 'text-red-400',
      info: 'text-blue-400',
      success: 'text-green-400',
    };
    return colors[type] || 'text-blue-400';
  };

  const filteredAlerts = filter === 'all' ? alerts : alerts.filter((alert) => alert.type === filter);
  const unreadCount = alerts.filter((a) => !a.resolved).length;

  return (
    <>
      <PageHeader />
      <section className="min-h-screen px-5 py-10" style={{ background: "linear-gradient(135deg, #cfd9f7 0%, #d8dff9 100%)" }}>
      <div className="max-w-5xl mx-auto">
        {/* Header */}
        <div className="flex justify-between items-start mb-8">
          <div>
            <h1 className="text-5xl font-bold mb-1" style={{ color: '#4169b3' }}>
              System Alerts
            </h1>
            <p className="text-lg" style={{ color: '#4169b3' }}>
              Monitor and manage system notifications
            </p>
          </div>
          <div
            className="flex items-center gap-2 px-4 py-2 rounded-lg"
            style={{ backgroundColor: 'rgba(65, 105, 179, 0.2)', color: '#4169b3' }}
          >
            <Bell className="w-5 h-5" />
            <span className="font-semibold">{unreadCount} Active</span>
          </div>
        </div>

        {/* Filter Buttons */}
        <div className="flex gap-2 mb-5 flex-wrap">
          {['all', 'critical', 'warning', 'info', 'success'].map((filterType) => (
            <button
              key={filterType}
              onClick={() => setFilter(filterType)}
              className="px-4 py-2 rounded-lg font-medium transition-all flex items-center gap-2"
              style={{
                backgroundColor: filter === filterType ? '#4169b3' : 'rgba(255, 255, 255, 0.1)',
                color: 'black',
                border: 'none',
                cursor: 'pointer',
              }}
            >
              {filterType === 'critical' && <AlertCircle className="w-4 h-4" />}
              {filterType === 'warning' && <AlertTriangle className="w-4 h-4" />}
              {filterType === 'info' && <Info className="w-4 h-4" />}
              {filterType === 'success' && <CheckCircle2 className="w-4 h-4" />}
              {filterType.charAt(0).toUpperCase() + filterType.slice(1)}
            </button>
          ))}
        </div>

        {/* Alerts List */}
        <div className="flex flex-col gap-5 mb-5">
          {filteredAlerts.length === 0 ? (
            <Card className="rounded-2xl bg-white/10 border-white/20 backdrop-blur-sm">
              <CardContent className="flex flex-col items-center justify-center py-12 gap-4">
                <CheckCircle2 className="w-12 h-12" style={{ color: '#4169b3' }} />
                <p style={{ color: '#4169b3' }} className="text-lg font-medium">
                  No alerts found
                </p>
              </CardContent>
            </Card>
          ) : (
            filteredAlerts.map((alert) => (
              <Card
                key={alert.id}
                className={`rounded-2xl border-white/20 backdrop-blur-sm ${
                  alert.resolved ? 'bg-white/5 opacity-75' : 'bg-white/10'
                }`}
              >
                <CardContent className="p-6 flex gap-4 items-start">
                  <div
                    className={`p-3 rounded-lg flex-shrink-0 ${getAlertColor(alert.type)}`}
                    style={{ backgroundColor: 'rgba(255, 255, 255, 0.1)' }}
                  >
                    {getAlertIcon(alert.type)}
                  </div>

                  <div className="flex-grow">
                    <div className="flex items-start justify-between gap-2 mb-1">
                      <h4 className="text-lg font-semibold" style={{ color: '#4169b3' }}>
                        {alert.title}
                      </h4>
                      {!alert.resolved && (
                        <span
                          className="px-2 py-1 rounded text-xs font-bold text-white"
                          style={{ backgroundColor: '#4169b3' }}
                        >
                          New
                        </span>
                      )}
                    </div>
                    <p style={{ color: '#4169b3' }} className="text-sm mb-3 opacity-90">
                      {alert.message}
                    </p>
                    <div className="flex items-center gap-2" style={{ color: '#4169b3' }}>
                      <Clock className="w-4 h-4" />
                      <span className="text-xs">{alert.time}</span>
                    </div>
                  </div>

                  <div className="flex gap-2 flex-shrink-0">
                    {!alert.resolved && (
                      <button
                        className="p-2 rounded-lg transition-all hover:bg-white/20"
                        style={{ color: '#4169b3' }}
                        title="Mark as Resolved"
                      >
                        <Check className="w-5 h-5" />
                      </button>
                    )}
                    <button
                      className="p-2 rounded-lg transition-all hover:bg-white/20"
                      style={{ color: '#4169b3' }}
                      title="Dismiss"
                    >
                      <X className="w-5 h-5" />
                    </button>
                  </div>
                </CardContent>
              </Card>
            ))
          )}
        </div>

        {/* Action Buttons */}
        <div className="flex gap-3">
          <button
            className="px-4 py-2 rounded-lg font-medium transition-all text-white flex items-center gap-2"
            style={{ backgroundColor: '#4169b3' }}
          >
            <CheckCircle2 className="w-4 h-4" />
            Mark All as Read
          </button>
          <button
            className="px-4 py-2 rounded-lg font-medium transition-all text-white"
            style={{ backgroundColor: 'rgba(65, 105, 179, 0.6)' }}
          >
            <X className="w-4 h-4 inline mr-2" />
            Clear Resolved
          </button>
        </div>
      </div>
    </section>
    </>
  );
}
