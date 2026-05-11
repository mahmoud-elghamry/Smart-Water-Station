'use client';
import React, { useEffect } from 'react';
import Link from 'next/link';
import { usePathname } from 'next/navigation';
import { X, Beaker, Bell, Cpu, Gauge, Microscope, Waves } from 'lucide-react';

const dashboardItems = [
  { label: 'Main Analytics', href: '/home', icon: Waves, color: '#5b7cc4 to #4169b3' },
  { label: 'Sensors', href: '/sensors', icon: Waves, color: '#5b7cc4 to #4169b3' },
  { label: 'Control', href: '/control', icon: Gauge, color: '#5b7cc4 to #4169b3' },
  { label: 'AI', href: '/ai', icon: Cpu, color: '#5b7cc4 to #4169b3' },
  { label: 'Prototype', href: '/prototype', icon: Beaker, color: '#5b7cc4 to #4169b3' },
  { label: 'Reports', href: '/reports', icon: Microscope, color: '#5b7cc4 to #4169b3' },
  { label: 'Alerts', href: '/alerts', icon: Bell, color: '#5b7cc4 to #4169b3' },
];

// Add heartbeat animation styles
const heartbeatStyles = `
  @keyframes heartbeat {
    0%, 100% {
      transform: scale(1);
    }
    14% {
      transform: scale(1.1);
    }
    28% {
      transform: scale(1);
    }
    42% {
      transform: scale(1.1);
    }
    70% {
      transform: scale(1);
    }
  }
  
  .heartbeat {
    animation: heartbeat 1.3s ease-in-out infinite;
  }
`;

interface DashboardProps {
  isOpen: boolean;
  onClose: () => void;
}

export default function Dashboard({ isOpen, onClose }: DashboardProps) {
  const pathname = usePathname();

  // Handle Escape key
  useEffect(() => {
    if (!isOpen) return;

    const handleEscape = (e: KeyboardEvent) => {
      if (e.key === 'Escape') {
        onClose();
      }
    };

    document.addEventListener('keydown', handleEscape);
    return () => document.removeEventListener('keydown', handleEscape);
  }, [isOpen, onClose]);

  // Add heartbeat styles to document
  useEffect(() => {
    const styleTag = document.createElement('style');
    styleTag.textContent = heartbeatStyles;
    document.head.appendChild(styleTag);
    return () => {
      document.head.removeChild(styleTag);
    };
  }, []);

  // Prevent body scroll when dashboard is open
  useEffect(() => {
    if (isOpen) {
      document.body.style.overflow = 'hidden';
    } else {
      document.body.style.overflow = 'unset';
    }

    return () => {
      document.body.style.overflow = 'unset';
    };
  }, [isOpen]);

  const handleItemClick = () => {
    onClose();
  };

  return (
    <>
      {/* Overlay */}
      {isOpen && (
        <div
          className="fixed inset-0 z-40 bg-black/50 backdrop-blur-sm transition-opacity duration-300"
          onClick={onClose}
        />
      )}

      {/* Dashboard Modal */}
      <div
        className={`fixed inset-0 z-50 transition-all duration-300 flex items-center justify-center p-4 ${
          isOpen ? 'opacity-100 pointer-events-auto' : 'opacity-0 pointer-events-none'
        }`}
      >
        <div
          className={`w-full max-w-2xl rounded-3xl bg-gradient-to-br from-blue-50 to-indigo-50 p-8 shadow-2xl transition-transform duration-300 ${
            isOpen ? 'scale-100' : 'scale-95'
          }`}
          onClick={(e) => e.stopPropagation()}
        >
          {/* Header */}
          <div className="flex items-center justify-between mb-8">
            <div>
              <h1 className="text-4xl font-bold" style={{ color: '#4169b3' }}>
                Dashboard
              </h1>
              <p style={{ color: '#4169b3' }} className="text-sm opacity-75 mt-1">
                Select a page to navigate
              </p>
            </div>
            <button
              onClick={onClose}
              className="p-2 rounded-lg hover:bg-white/50 transition-colors"
              style={{ color: '#4169b3' }}
            >
              <X className="w-6 h-6" />
            </button>
          </div>

          {/* Grid of Dashboard Items */}
          <div className="grid grid-cols-2 md:grid-cols-3 gap-4 mb-6 text-blue-600">
            {dashboardItems.map((item, index) => {
              const Icon = item.icon;
              const isActive = pathname === item.href;
              const isPrototype = item.href === '/prototype';

              return (
                <Link
                  key={item.href}
                  href={item.href}
                  onClick={handleItemClick}
                  className={`p-6 rounded-2xl transition-all duration-300 group cursor-pointer transform hover:scale-105 ${
                    isActive ? 'ring-2 ring-offset-2 ring-offset-blue-50' : ''
                  } ${isPrototype ? 'heartbeat' : ''}`}
                  style={{
                    background: `linear-gradient(135deg, var(--color-start) 0%, var(--color-end) 100%)`,
                    '--color-start': item.color.split(' to ')[0],
                    '--color-end': item.color.split(' to ')[1],
                  } as React.CSSProperties}
                >
                  <div className="flex flex-col items-center gap-3 text-white">
                    <div className="p-3 rounded-xl bg-white/20 group-hover:bg-white/30 transition-colors transform group-hover:scale-110">
                      <Icon className="w-6 h-6" />
                    </div>
                    <span className="font-semibold text-sm text-center">{item.label}</span>
                  </div>
                </Link>
              );
            })}
          </div>

          {/* Footer */}
          <div className="pt-6 border-t border-blue-200">
            <p style={{ color: '#4169b3' }} className="text-xs text-center opacity-60">
              💡 Press <kbd className="px-2 py-1 rounded bg-white/50 font-mono text-xs">Esc</kbd> to close • Click outside to close
            </p>
          </div>
        </div>
      </div>
    </>
  );
}
