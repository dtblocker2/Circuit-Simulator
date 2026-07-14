import { useMemo } from 'react';
import {
  LineChart,
  Line,
  XAxis,
  YAxis,
  CartesianGrid,
  Tooltip,
  ResponsiveContainer,
  ReferenceLine,
} from 'recharts';

function formatTime(seconds) {
  if (Math.abs(seconds) >= 1) return `${(seconds * 1e3).toFixed(2)} ms`;
  if (Math.abs(seconds) >= 1e-3) return `${(seconds * 1e6).toFixed(2)} µs`;
  return `${(seconds * 1e9).toFixed(2)} ns`;
}

export default function Oscilloscope({ waveform, cursor, onCursorChange }) {
  const data = useMemo(() => {
    if (!waveform?.times?.length) return [];
    return waveform.times.map((t, i) => ({
      time: t,
      voltage: waveform.voltages[i],
      timeLabel: formatTime(t),
    }));
  }, [waveform]);

  if (!data.length) {
    return (
      <div className="chart-empty">
        Run a simulation to display the waveform
      </div>
    );
  }

  return (
    <ResponsiveContainer width="100%" height="100%">
      <LineChart
        data={data}
        margin={{ top: 10, right: 20, left: 10, bottom: 10 }}
        onMouseMove={(state) => {
          if (state?.activePayload?.[0]) {
            const { time, voltage } = state.activePayload[0].payload;
            onCursorChange?.({ time, voltage });
          }
        }}
        onMouseLeave={() => onCursorChange?.(null)}
      >
        <CartesianGrid strokeDasharray="3 3" stroke="#1e2a3a" />
        <XAxis
          dataKey="time"
          tickFormatter={formatTime}
          stroke="#8b9cb3"
          fontSize={11}
          fontFamily="JetBrains Mono"
        />
        <YAxis
          stroke="#8b9cb3"
          fontSize={11}
          fontFamily="JetBrains Mono"
          tickFormatter={(v) => `${v.toFixed(2)} V`}
          domain={['auto', 'auto']}
        />
        <Tooltip
          contentStyle={{
            background: '#111820',
            border: '1px solid #2a3544',
            borderRadius: 8,
            fontFamily: 'JetBrains Mono',
            fontSize: 12,
          }}
          formatter={(value) => [`${Number(value).toFixed(4)} V`, 'Voltage']}
          labelFormatter={(t) => `Time: ${formatTime(t)}`}
        />
        {cursor && (
          <ReferenceLine x={cursor.time} stroke="#f07178" strokeDasharray="4 4" />
        )}
        <Line
          type="monotone"
          dataKey="voltage"
          stroke="#3dd68c"
          strokeWidth={2}
          dot={false}
          isAnimationActive={false}
        />
      </LineChart>
    </ResponsiveContainer>
  );
}
