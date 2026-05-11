export interface SensorCardProp {
  id: string;
  title: string;
  description?: string;
  unit: string;
  currentValue: number;
  status: string;
  statusColor: string;
  trend: string;
  chartData: { time: string; value: number }[];
  color: string;
}
