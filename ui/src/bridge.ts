import { API_URL } from './protocol';

const ALLOWED_ORIGIN = 'https://lightbar.wywiol.eu';
const ALLOWED_PREVIEW_ORIGIN = /^https:\/\/[a-z0-9-]+-lightbar-web\.pawel-wywiol\.workers\.dev$/;
const BRIDGE_HASH_PREFIX = '#bridge=';

type Request = (input: string, init?: RequestInit) => Promise<Response>;

type BridgeMessage = { id: number; type: 'info' } | { id: number; type: 'send'; bytes: Uint8Array<ArrayBuffer> };

export type BridgeResponse =
  | { id: number; type: 'info'; data: unknown }
  | { id: number; type: 'sent'; ok: boolean }
  | { id: number; type: 'error' };

export const isAllowedOrigin = (origin: string) => origin === ALLOWED_ORIGIN || ALLOWED_PREVIEW_ORIGIN.test(origin);

export const parseBridgeOrigin = (hash: string): string | undefined => {
  if (!hash.startsWith(BRIDGE_HASH_PREFIX)) {
    return undefined;
  }

  try {
    const origin = decodeURIComponent(hash.slice(BRIDGE_HASH_PREFIX.length));
    return isAllowedOrigin(origin) ? origin : undefined;
  } catch {
    return undefined;
  }
};

const parseBridgeMessage = (value: unknown): BridgeMessage | undefined => {
  const { id, type, bytes } = (value ?? {}) as { id?: unknown; type?: unknown; bytes?: unknown };

  if (typeof id !== 'number') {
    return undefined;
  }

  if (type === 'info') {
    return { id, type };
  }

  return type === 'send' && bytes instanceof Uint8Array
    ? { id, type, bytes: bytes as Uint8Array<ArrayBuffer> }
    : undefined;
};

export const handleBridgeMessage = async (value: unknown, request: Request): Promise<BridgeResponse | undefined> => {
  const message = parseBridgeMessage(value);

  if (!message) {
    return undefined;
  }

  try {
    if (message.type === 'info') {
      const response = await request(API_URL);
      return { id: message.id, type: 'info', data: (await response.json()) as unknown };
    }

    const response = await request(API_URL, { method: 'POST', body: message.bytes });
    return { id: message.id, type: 'sent', ok: response.ok };
  } catch {
    return { id: message.id, type: 'error' };
  }
};
