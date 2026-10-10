import { describe, expect, it, vi } from 'vitest';
import { handleBridgeMessage, isAllowedOrigin, parseBridgeOrigin } from './bridge';

describe('isAllowedOrigin', () => {
  it.each([
    ['https://lightbar.wywiol.eu', true],
    ['https://feature-save-schema-lightbar-web.pawel-wywiol.workers.dev', true],
    ['http://lightbar.wywiol.eu', false],
    ['https://lightbar.wywiol.eu.evil.com', false],
    ['https://evil-lightbar-web.attacker.workers.dev', false],
    ['https://a.b-lightbar-web.pawel-wywiol.workers.dev', false],
    ['null', false],
  ])('%s -> %s', (origin, expected) => {
    expect(isAllowedOrigin(origin)).toBe(expected);
  });
});

describe('parseBridgeOrigin', () => {
  it('returns allowed origin from hash', () => {
    expect(parseBridgeOrigin(`#bridge=${encodeURIComponent('https://lightbar.wywiol.eu')}`)).toBe(
      'https://lightbar.wywiol.eu',
    );
  });

  it.each(['', '#bridge', '#bridge=', '#other=x', `#bridge=${encodeURIComponent('https://evil.com')}`, '#bridge=%E0'])(
    'returns undefined for %s',
    (hash) => {
      expect(parseBridgeOrigin(hash)).toBeUndefined();
    },
  );
});

describe('handleBridgeMessage', () => {
  it('answers info with device response', async () => {
    const request = vi.fn(async () => new Response(JSON.stringify({ type: 'info', data: { uid: 'a' } })));

    await expect(handleBridgeMessage({ id: 1, type: 'info' }, request)).resolves.toEqual({
      id: 1,
      type: 'info',
      data: { type: 'info', data: { uid: 'a' } },
    });
    expect(request).toHaveBeenCalledWith('/api/lightbar');
  });

  it('posts bytes on send', async () => {
    const request = vi.fn(async () => new Response('{"ok":true}'));
    const bytes = new Uint8Array([1, 2, 3]);

    await expect(handleBridgeMessage({ id: 2, type: 'send', bytes }, request)).resolves.toEqual({
      id: 2,
      type: 'sent',
      ok: true,
    });
    expect(request).toHaveBeenCalledWith('/api/lightbar', { method: 'POST', body: bytes });
  });

  it('reports not ok send', async () => {
    const request = vi.fn(async () => new Response('', { status: 400 }));

    await expect(handleBridgeMessage({ id: 3, type: 'send', bytes: new Uint8Array([1]) }, request)).resolves.toEqual({
      id: 3,
      type: 'sent',
      ok: false,
    });
  });

  it('returns error when request fails', async () => {
    const request = vi.fn(async () => {
      throw new Error('offline');
    });

    await expect(handleBridgeMessage({ id: 4, type: 'info' }, request)).resolves.toEqual({ id: 4, type: 'error' });
  });

  it.each([null, {}, { id: 'x', type: 'info' }, { id: 5, type: 'unknown' }, { id: 6, type: 'send', bytes: [1] }])(
    'ignores invalid message %j',
    async (message) => {
      const request = vi.fn();

      await expect(handleBridgeMessage(message, request)).resolves.toBeUndefined();
      expect(request).not.toHaveBeenCalled();
    },
  );
});
