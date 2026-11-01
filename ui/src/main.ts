import { handleBridgeMessage, isAllowedOrigin, parseBridgeOrigin } from './bridge';
import {
  API_URL,
  describeWifi,
  type DeviceInfo,
  encodeWifiRequest,
  hasWifiProblem,
  NETWORK_TYPE_AP,
  parseDeviceInfo,
  WIFI_STATE_CONNECTED,
} from './protocol';

const REFRESH_INTERVAL = 300_000;
const SETUP_REFRESH_INTERVAL = 2000;
const AFTER_SAVE_DELAY = 1000;
const REQUEST_TIMEOUT = 5000;

const select = <T extends HTMLElement>(selector: string) => document.querySelector(selector) as T;

const form = select<HTMLFormElement>('#wifi');
const ssid = select<HTMLInputElement>('#ssid');
const password = select<HTMLInputElement>('#password');
const save = select<HTMLButtonElement>('#save');
const info = select<HTMLDListElement>('#info');

const status = select<HTMLParagraphElement>('#status');
const address = select<HTMLParagraphElement>('#address');
const host = select<HTMLAnchorElement>('#host');

let refreshTimer: ReturnType<typeof setTimeout> | undefined;
let setupUid: string | undefined;
let saveFailed = false;

const showStatus = (message?: string, isProblem = false) => {
  status.hidden = !message;
  status.textContent = message ?? '';
  status.classList.toggle('problem', isProblem);
};

const renderStatus = (device?: DeviceInfo) => {
  if (saveFailed) {
    showStatus('Could not send the settings to the device, try again.', true);
  } else if (!device) {
    showStatus(
      setupUid
        ? `No response from the device. Join the ${setupUid} WiFi again, it can drop while the device connects.`
        : 'No response from the device.',
      true,
    );
  } else {
    showStatus(device.wifi && describeWifi(device.wifi), device.wifi && hasWifiProblem(device.wifi));
  }
};

const render = (device?: DeviceInfo) => {
  form.hidden = device?.network !== NETWORK_TYPE_AP;
  info.hidden = !device;
  select('#uid').textContent = device?.uid ?? '';
  select('#leds').textContent = device ? String(device.leds) : '';

  if (device) {
    setupUid = device.network === NETWORK_TYPE_AP ? device.uid : undefined;
  }

  renderStatus(device);

  const deviceHost = device?.wifi?.state === WIFI_STATE_CONNECTED ? device.host : undefined;
  address.hidden = !deviceHost;
  host.textContent = deviceHost ? `http://${deviceHost}` : '';
  host.href = deviceHost ? `http://${deviceHost}/` : '';
};

const scheduleRefresh = (delay: number) => {
  clearTimeout(refreshTimer);
  refreshTimer = setTimeout(refresh, delay);
};

const fetchDevice = async () => {
  try {
    const response = await fetch(API_URL, { signal: AbortSignal.timeout(REQUEST_TIMEOUT) });
    return response.ok ? parseDeviceInfo(await response.json()) : undefined;
  } catch {
    return undefined;
  }
};

const refresh = async () => {
  const device = await fetchDevice();
  render(device);
  scheduleRefresh(!device || device.network === NETWORK_TYPE_AP ? SETUP_REFRESH_INTERVAL : REFRESH_INTERVAL);
};

form.addEventListener('input', () => {
  save.disabled = !ssid.value || !password.value;
});

form.addEventListener('submit', async (event) => {
  event.preventDefault();
  const body = encodeWifiRequest(ssid.value, password.value);
  form.reset();
  save.disabled = true;
  saveFailed = false;
  showStatus('Sending the settings to the device…');
  address.hidden = true;

  try {
    saveFailed = !(await fetch(API_URL, { method: 'POST', body, signal: AbortSignal.timeout(REQUEST_TIMEOUT) })).ok;
  } catch {
    saveFailed = true;
  }

  scheduleRefresh(AFTER_SAVE_DELAY);
});

const bridgeOrigin = parseBridgeOrigin(location.hash);
const opener = window.opener as Window | null;

if (bridgeOrigin && opener) {
  select('#bridge').hidden = false;

  window.addEventListener('message', async ({ origin, data, source }) => {
    if (!isAllowedOrigin(origin) || !source) {
      return;
    }

    const response = await handleBridgeMessage(data, (input, init) => fetch(input, init));

    if (response) {
      (source as Window).postMessage(response, origin);
    }
  });

  opener.postMessage({ type: 'ready' }, bridgeOrigin);
}

void refresh();
