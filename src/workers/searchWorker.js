let wasmInstance = null;
let wasmMemory = null;
let randomSize = 0;
let cancelled = false;

const SECRET_LENGTH = 20;

async function initWasm() {
    try {
        const imports = {
        };
        const wasmResponse = await fetch('../wasm/totp_search.wasm');
        const wasmBuffer = await wasmResponse.arrayBuffer();
        
        const memory = new WebAssembly.Memory({
            initial: 1024,   // wasm 側の memory 設定と合わせる
            maximum: 8192
        });
        const importObject = {
            env: {
                memcpy: (dst, src, len) => {
                    const mem = new Uint8Array(wasmInstance.exports.memory.buffer);
                    mem.copyWithin(dst, src, src + len);
                    return dst;
                }
            }
        };
        
        const result = await WebAssembly.instantiate(wasmBuffer, importObject);
        wasmInstance = result.instance;
        wasmMemory = wasmInstance.exports.memory;
        
        console.log('WASM module initialized successfully (standalone)');
        return true;
    } catch (error) {
        console.error('Failed to initialize WASM:', error);
        return false;
    }
}

async function initializeRandomData(size) {
    randomSize = size;
    
    const result = wasmInstance.exports.init_random_data(size);
    if (result !== 0) {
        throw new Error('Failed to allocate memory in WASM');
    }
    
    const ptr = wasmInstance.exports.get_random_buffer();
    if (ptr === 0) {
        throw new Error('Failed to get random buffer pointer');
    }
    
    const heapU8 = new Uint8Array(wasmMemory.buffer);
    const chunkSize = 65536;
    for (let offset = 0; offset < size; offset += chunkSize) {
        const remaining = Math.min(chunkSize, size - offset);
        const chunk = new Uint8Array(remaining);
        crypto.getRandomValues(chunk);
        heapU8.set(chunk, ptr + offset);
    }
    
    return size;
}

async function search(stepStart, stepEnd, target) {
    cancelled = false;
    const totalSteps = stepEnd - stepStart + 1;
    const exports = wasmInstance.exports;
    
    for (let step = stepStart; step <= stepEnd; step++) {
        if (cancelled) {
            break;
        }
        
        await new Promise(resolve => setTimeout(resolve, 0));
        
        if (cancelled) {
            break;
        }
        
        const currentStep = step - stepStart + 1;
        
        self.postMessage({
            type: 'progress',
            currentStep: currentStep,
            totalSteps: totalSteps,
            index: 0,
            maxIndex: randomSize - SECRET_LENGTH
        });
        
        const stepLow = step >>> 0;
        const stepHigh = Math.floor(step / 0x100000000) >>> 0;
        const foundIndex = exports.search_step(stepLow, stepHigh, target);
        
        if (foundIndex === -2) {
            break;
        }
        
        if (foundIndex >= 0) {
            const secretPtr = exports.get_found_secret();
            const heapU8 = new Uint8Array(wasmMemory.buffer);
            const secret = heapU8.slice(secretPtr, secretPtr + SECRET_LENGTH);
            
            const secretHex = Array.from(secret)
                .map(b => b.toString(16).padStart(2, '0'))
                .join('');
            
            self.postMessage({
                type: 'found',
                result: {
                    step: step,
                    secret: Array.from(secret),
                    secretHex: secretHex
                }
            });
        }
        
        self.postMessage({
            type: 'stepComplete',
            currentStep: currentStep,
            totalSteps: totalSteps
        });
    }
    
    self.postMessage({ type: 'complete' });
}

function handleCancel() {
    cancelled = true;
    if (wasmInstance) {
        wasmInstance.exports.cancel_search();
    }
}

self.onmessage = async function(e) {
    const { type, data } = e.data;
    
    switch (type) {
        case 'init':
            if (!wasmInstance) {
                const success = await initWasm();
                if (!success) {
                    self.postMessage({ type: 'error', message: 'Failed to initialize WASM' });
                    return;
                }
            }
            
            try {
                const size = await initializeRandomData(data.randomSize);
                self.postMessage({ type: 'initialized', size: size });
            } catch (error) {
                self.postMessage({ type: 'error', message: error.message });
            }
            break;
            
        case 'search':
            search(data.stepStart, data.stepEnd, data.target);
            break;
            
        case 'cancel':
            handleCancel();
            break;
    }
};