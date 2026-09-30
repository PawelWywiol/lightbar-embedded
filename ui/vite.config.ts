import { mkdirSync, readFileSync, rmSync, writeFileSync } from 'node:fs';
import { gzipSync } from 'node:zlib';
import { defineConfig, type Plugin } from 'vite';
import { viteSingleFile } from 'vite-plugin-singlefile';

const PUBLIC_HTML = '../data/public_html';

const deployToFirmwareData = (): Plugin => ({
  name: 'deploy-to-firmware-data',
  apply: 'build',
  closeBundle() {
    rmSync(PUBLIC_HTML, { recursive: true, force: true });
    mkdirSync(PUBLIC_HTML, { recursive: true });
    writeFileSync(`${PUBLIC_HTML}/index.html.gz`, gzipSync(readFileSync('dist/index.html'), { level: 9 }));
  },
});

export default defineConfig({
  plugins: [viteSingleFile(), deployToFirmwareData()],
  server: {
    proxy: process.env['DEVICE_URL'] ? { '/api': process.env['DEVICE_URL'] } : {},
  },
});
