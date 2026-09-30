export const API_URL = '/api/lightbar';
export const NETWORK_TYPE_AP = 2;
export const SSID_MAX_LENGTH = 32;
export const PASSWORD_MAX_LENGTH = 64;

const WIFI_REQUEST_TYPE = 0x77_69_66_69;
const EOL = 0x45_4f_4c_00;
const HEADER_LENGTH = 8;
const EOL_LENGTH = 4;

export interface DeviceInfo {
  uid: string;
  leds: number;
  network: number;
}

export const parseDeviceInfo = (value: unknown): DeviceInfo | undefined => {
  const { type, data } = (value ?? {}) as { type?: unknown; data?: Partial<DeviceInfo> | null };

  if (type !== 'info' || !data) {
    return undefined;
  }

  const { uid, leds, network } = data;

  return typeof uid === 'string' && typeof leds === 'number' && typeof network === 'number'
    ? { uid, leds, network }
    : undefined;
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
