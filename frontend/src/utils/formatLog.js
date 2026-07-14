const MAX_LOG_CHARS = 120;
const MAX_LOG_LINES = 80;

export function formatLogLine(line) {
  if (!line) return '';
  if (line.startsWith('[OSC_DATA]')) {
    try {
      const data = JSON.parse(line.slice('[OSC_DATA]'.length).trim());
      return `[OSC_DATA] node=${data.node} points=${data.times?.length ?? 0} range=[${data.t_start}, ${data.t_end}]`;
    } catch {
      return '[OSC_DATA] (waveform exported)';
    }
  }
  if (line.length > MAX_LOG_CHARS) return `${line.slice(0, MAX_LOG_CHARS)}…`;
  return line;
}

export function trimLogHistory(logs) {
  return logs.slice(-MAX_LOG_LINES).map(formatLogLine);
}

export { MAX_LOG_LINES };
