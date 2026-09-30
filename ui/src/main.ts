import { API_URL, type DeviceInfo, encodeWifiRequest, NETWORK_TYPE_AP, parseDeviceInfo } from './protocol';

const REFRESH_INTERVAL = 300_000;

const select = <T extends HTMLElement>(selector: string) => document.querySelector(selector) as T;

const form = select<HTMLFormElement>('#wifi');
const ssid = select<HTMLInputElement>('#ssid');
const password = select<HTMLInputElement>('#password');
const save = select<HTMLButtonElement>('#save');
const info = select<HTMLDListElement>('#info');

const render = (device?: DeviceInfo) => {
  form.hidden = device?.network !== NETWORK_TYPE_AP;
  info.hidden = !device;
  select('#uid').textContent = device?.uid ?? '';
  select('#leds').textContent = device ? String(device.leds) : '';
};

const refresh = async () => {
  try {
    const response = await fetch(API_URL);
    render(response.ok ? parseDeviceInfo(await response.json()) : undefined);
  } catch {
    render();
  }
};

form.addEventListener('input', () => {
  save.disabled = !ssid.value || !password.value;
});

form.addEventListener('submit', async (event) => {
  event.preventDefault();
  const body = encodeWifiRequest(ssid.value, password.value);
  form.reset();
  save.disabled = true;

  try {
    await fetch(API_URL, { method: 'POST', body });
  } catch {
    render();
  }
});

void refresh();
setInterval(refresh, REFRESH_INTERVAL);
