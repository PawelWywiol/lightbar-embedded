import { describe, expect, it } from 'vitest';
import { describeWifi, encodeWifiRequest, parseDeviceInfo, WIFI_STATE_CONNECTING } from './protocol';

describe('encodeWifiRequest', () => {
  it('encodes header, zero padded credentials and EOL', () => {
    const buffer = encodeWifiRequest('home', 'secret');
    const view = new DataView(buffer.buffer);

    expect(buffer.length).toBe(110);
    expect(view.getUint32(0, true)).toBe(0x77_69_66_69);
    expect(view.getUint32(4, true)).toBe(98);
    expect(new TextDecoder().decode(buffer.subarray(8, 12))).toBe('home');
    expect(buffer[12]).toBe(0);
    expect(new TextDecoder().decode(buffer.subarray(41, 47))).toBe('secret');
    expect(buffer[47]).toBe(0);
    expect(view.getUint32(106, true)).toBe(0x45_4f_4c_00);
  });

  it('keeps trailing zero when credentials exceed max byte length', () => {
    const buffer = encodeWifiRequest('ś'.repeat(32), 'p'.repeat(80));

    expect(buffer[40]).toBe(0);
    expect(buffer[105]).toBe(0);
    expect(new DataView(buffer.buffer).getUint32(106, true)).toBe(0x45_4f_4c_00);
  });
});

describe('parseDeviceInfo', () => {
  it('returns device info for valid response', () => {
    expect(parseDeviceInfo({ type: 'info', data: { uid: 'a1', leds: 16, network: 2 } })).toEqual({
      uid: 'a1',
      leds: 16,
      network: 2,
    });
  });

  it('keeps host and wifi status', () => {
    const wifi = { state: 3, ssid: 'home', reason: 0, attempt: 1 };
    const data = { uid: 'a1', leds: 16, network: 2, host: '192.168.0.2', wifi };

    expect(parseDeviceInfo({ type: 'info', data })).toEqual(data);
  });

  it('drops malformed wifi status', () => {
    const data = { uid: 'a1', leds: 16, network: 2, wifi: { state: '3' } };

    expect(parseDeviceInfo({ type: 'info', data })?.wifi).toBeUndefined();
  });

  it.each([null, 1, {}, { type: 'info' }, { type: 'info', data: { uid: 'a1', leds: '16', network: 2 } }])(
    'returns undefined for %j',
    (value) => {
      expect(parseDeviceInfo(value)).toBeUndefined();
    },
  );
});

describe('describeWifi', () => {
  const wifi = { state: WIFI_STATE_CONNECTING, ssid: 'home', reason: 0, attempt: 1 };

  it('reports progress without a failure', () => {
    expect(describeWifi(wifi)).toBe('Connecting to home…');
  });

  it('explains a known failure', () => {
    expect(describeWifi({ ...wifi, reason: 201, attempt: 3 })).toBe(
      'Cannot connect to home. Network not found, check its name. The device sees 2.4 GHz networks only. Retrying automatically, attempt 3.',
    );
  });

  it('shows the code of an unknown failure', () => {
    expect(describeWifi({ ...wifi, reason: 99 })).toContain('Connection failed (reason 99).');
  });

  it('ignores an old failure once connected', () => {
    expect(describeWifi({ ...wifi, state: 3, reason: 201 })).toBe('Connected to home.');
  });
});
