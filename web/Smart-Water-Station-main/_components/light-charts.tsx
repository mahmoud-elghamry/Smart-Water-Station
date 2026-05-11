type Point = {
  label: string;
  value: number;
};

function pointsFor(data: Point[], width: number, height: number, padding: number) {
  const values = data.map((item) => item.value);
  const min = Math.min(...values);
  const max = Math.max(...values);
  const range = max - min || 1;

  return data.map((item, index) => {
    const x = padding + (index / Math.max(data.length - 1, 1)) * (width - padding * 2);
    const y = height - padding - ((item.value - min) / range) * (height - padding * 2);
    return { ...item, x, y };
  });
}

export function SimpleLineChart({ data, color = "#4169b3" }: { data: Point[]; color?: string }) {
  const width = 640;
  const height = 260;
  const padding = 28;
  const points = pointsFor(data, width, height, padding);
  const line = points.map((point) => `${point.x},${point.y}`).join(" ");

  return (
    <svg viewBox={`0 0 ${width} ${height}`} className="h-[300px] w-full overflow-visible">
      {[0, 1, 2, 3].map((row) => {
        const y = padding + row * ((height - padding * 2) / 3);
        return <line key={row} x1={padding} x2={width - padding} y1={y} y2={y} stroke="#e5e7eb" strokeDasharray="4 4" />;
      })}
      <polyline fill="none" stroke={color} strokeWidth="4" strokeLinecap="round" strokeLinejoin="round" points={line} />
      {points.map((point) => (
        <circle key={point.label} cx={point.x} cy={point.y} r="4" fill={color} />
      ))}
      {points.filter((_, index) => index % 2 === 0).map((point) => (
        <text key={point.label} x={point.x} y={height - 4} textAnchor="middle" fontSize="12" fill={color}>
          {point.label}
        </text>
      ))}
    </svg>
  );
}

export function SimpleAreaChart({ data, color = "#4169b3" }: { data: Point[]; color?: string }) {
  const width = 280;
  const height = 130;
  const padding = 16;
  const points = pointsFor(data, width, height, padding);
  const line = points.map((point) => `${point.x},${point.y}`).join(" ");
  const area = `${padding},${height - padding} ${line} ${width - padding},${height - padding}`;

  return (
    <svg viewBox={`0 0 ${width} ${height}`} className="h-32 w-full">
      <polygon points={area} fill={color} opacity="0.14" />
      <polyline fill="none" stroke={color} strokeWidth="3" strokeLinecap="round" strokeLinejoin="round" points={line} />
    </svg>
  );
}

export function DonutChart({ value, color = "#4ade80" }: { value: number; color?: string }) {
  const radius = 46;
  const circumference = 2 * Math.PI * radius;
  const offset = circumference - (Math.min(Math.max(value, 0), 100) / 100) * circumference;

  return (
    <svg viewBox="0 0 120 120" className="h-[220px] w-full">
      <circle cx="60" cy="60" r={radius} fill="none" stroke="#e5e7eb" strokeWidth="16" />
      <circle
        cx="60"
        cy="60"
        r={radius}
        fill="none"
        stroke={color}
        strokeWidth="16"
        strokeLinecap="round"
        strokeDasharray={circumference}
        strokeDashoffset={offset}
        transform="rotate(-90 60 60)"
      />
      <text x="60" y="67" textAnchor="middle" fontSize="24" fontWeight="700" fill="#4169b3">
        {value}%
      </text>
    </svg>
  );
}
