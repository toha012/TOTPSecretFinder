import { resolve } from 'path'
import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'

// https://vite.dev/config/
export default defineConfig(({ mode }) => {
    const base = mode === 'development' ? '/' : '/TOTPSecretFinder/'

    return {
        plugins: [react()],
        base: base,
        build: {
            rollupOptions: {
                input: {
                    main: resolve(__dirname, 'index.html'),
                    404: resolve(__dirname, '404.html'),
                }
            }
        }
    }
})