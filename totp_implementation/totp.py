import hashlib
import time

def totp(secret: bytes, hfunc = hashlib.sha1, X: int = 30, T0: int = 0, step: int = None) -> int:
    if step is None:
        cut = int(time.time())
        step = (cut - T0) // X
    return hotp(secret, step, hfunc=hfunc)

def hotp(secret: bytes, counter: bytes | int, digit: int = 6, hfunc = hashlib.sha1) -> int:
    if isinstance(counter, int):
        counter = counter.to_bytes(8, byteorder='big')
    elif len(counter) != 8:
        raise TypeError('Counter must be 8 bytes.')
    
    hs = hmac(secret, counter, hfunc)
    offset = hs[-1] & 0xf
    p = int.from_bytes(hs[offset : offset + 4], 'big')
    return (p & 0x7fffffff) % (10 ** digit)

def hmac(key: bytes, msg: bytes, hfunc):
    def xor(a, b):
        return bytes([aa ^ bb for aa, bb in zip(a, b)])
    
    B = hfunc().block_size
    ipad = b'\x36' * B
    opad = b'\x5c' * B
    
    if len(key) > B:
        key = hfunc(key).digest()
    kpad = key + b'\x00' * (B - len(key))

    k_opad = xor(kpad, opad)
    k_ipad = xor(kpad, ipad)
    return hfunc(k_opad + hfunc(k_ipad + msg).digest()).digest()