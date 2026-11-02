export const API_URL = '/api/lightbar';
export const NETWORK_TYPE_AP = 2;
export const SSID_MAX_LENGTH = 32;
export const PASSWORD_MAX_LENGTH = 64;

const WIFI_REQUEST_TYPE = 0x77_69_66_69;
const EOL = 0x45_4f_4c_00;
const HEADER_LENGTH = 8;
const EOL_LENGTH = 4;

export const WIFI_STATE_NO_CREDENTIALS = 0;
export const WIFI_STATE_CONNECTING = 1;
export const WIFI_STATE_WAITING_FOR_IP = 2;
export const WIFI_STATE_CONNECTED = 3;

const CHECK_PASSWORD = 'The network rejected the connection, check the password.';

const WIFI_PROBLEMS: Record<number, string> = {
  2: 'The router did not answer. Move the device closer, or check the password.',
  14: CHECK_PASSWORD,
  15: CHECK_PASSWORD,
  200: 'Signal lost, the device may be too far from the router.',
  201: 'Network not found, check its name. The device sees 2.4 GHz networks only.',
  202: CHECK_PASSWORD,
  203: 'The router refused the device.',
  204: CHECK_PASSWORD,
  210: 'The network security type is not supported.',
  211: 'The network security type is not supported.',
  212: 'The signal is too weak.',
};

export interface WifiStatus {
  state: number;
  ssid: string;
  reason: number;
  attempt: number;
}

export interface DeviceInfo {
  uid: string;
  leds: number;
  network: number;
  host?: string;
  wifi?: WifiStatus;
}

const parseWifiStatus = (value: Partial<WifiStatus> | null | undefined): WifiStatus | undefined => {
  const { state, ssid, reason, attempt } = value ?? {};

  return typeof state === 'number' &&
    typeof ssid === 'string' &&
    typeof reason === 'number' &&
    typeof attempt === 'number'
    ? { state, ssid, reason, attempt }
    : undefined;
};

export const parseDeviceInfo = (value: unknown): DeviceInfo | undefined => {
  const { type, data } = (value ?? {}) as { type?: unknown; data?: Partial<DeviceInfo> | null };

  if (type !== 'info' || !data) {
    return undefined;
  }

  const { uid, leds, network, host, wifi } = data;

  if (typeof uid !== 'string' || typeof leds !== 'number' || typeof network !== 'number') {
    return undefined;
  }

  return { uid, leds, network, host: typeof host === 'string' ? host : undefined, wifi: parseWifiStatus(wifi) };
};

export const hasWifiProblem = ({ state, reason }: WifiStatus) => state === WIFI_STATE_CONNECTING && reason !== 0;

export const describeWifi = (wifi: WifiStatus): string => {
  const { state, ssid, reason, attempt } = wifi;

  if (state === WIFI_STATE_NO_CREDENTIALS) {
    return 'Not connected to a WiFi network yet.';
  }

  if (state === WIFI_STATE_CONNECTED) {
    return `Connected to ${ssid}.`;
  }

  if (state === WIFI_STATE_WAITING_FOR_IP) {
    return `Joined ${ssid}, waiting for an IP address…`;
  }

  if (!hasWifiProblem(wifi)) {
    return `Connecting to ${ssid}…`;
  }

  const problem = WIFI_PROBLEMS[reason] ?? `Connection failed (reason ${reason}).`;

  return `Cannot connect to ${ssid}. ${problem} Retrying automatically, attempt ${attempt}.`;
};

export const encodeWifiRequest = (ssid: string, password: string): Uint8Array<ArrayBuffer> => {
  const ssidSize = SSID_MAX_LENGTH + 1;
  const dataSize = ssidSize + PASSWORD_MAX_LENGTH + 1;
  const buffer = new Uint8Array(HEADER_LENGTH + dataSize + EOL_LENGTH);
  const view = new DataView(buffer.buffer);
  const encoder = new TextEncoder();

  view.setUint32(0, WIFI_REQUEST_TYPE, true);
  view.setUint32(4, dataSize, true);
  buffer.set(encoder.encode(ssid).subarray(0, SSID_MAX_LENGTH), HEADER_LENGTH);
  buffer.set(encoder.encode(password).subarray(0, PASSWORD_MAX_LENGTH), HEADER_LENGTH + ssidSize);
  view.setUint32(HEADER_LENGTH + dataSize, EOL, true);

  return buffer;
};
