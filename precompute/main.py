import datetime
import json
import os
import subprocess
import sys

def main(day: datetime.datetime):
    step = calc_step_from_day(day)

    TMP_DIR = './tmp'
    C_FILE = f'{TMP_DIR}/precompute.c'
    EXE_FILE = f'{TMP_DIR}/precompute'
    SECRETS_BYTE_FILE = f'{TMP_DIR}/output.bin'

    if not os.path.isdir(TMP_DIR):
        os.mkdir(TMP_DIR)

    with open('./precompute.c', 'r') as f1, open(C_FILE, 'w') as f2:
        f2.write(f1.read().replace('{{STEP_START}}', str(step)).replace('{{OUTPUT_FILE}}', SECRETS_BYTE_FILE))
    
    subprocess.run(f'gcc -O3 -w -march=native -mtune=native -flto -funroll-loops {C_FILE} -o {EXE_FILE} -lcrypto', shell=True)
    subprocess.run(EXE_FILE)

    DAY = day.strftime('%Y-%m-%d')
    PRECOMPUTED_DIR = '../public/precomputed_data'
    PRECOMPUTED_INDEX_FILE = f'{PRECOMPUTED_DIR}/index.json'
    PRECOMPUTED_SECRET_FILE = f'{PRECOMPUTED_DIR}/{DAY}.txt'

    if not os.path.isdir(PRECOMPUTED_DIR):
        os.mkdir(PRECOMPUTED_DIR)

    with open(SECRETS_BYTE_FILE, 'rb') as f1, open(PRECOMPUTED_SECRET_FILE, 'w') as f2:
        data = f1.read()
        secrets = [data[i : i + 20] for i in range(0, len(data), 20)]
        for secret in secrets:
            f2.write(f'{secret.hex()}\n')
    
    if not os.path.isfile(PRECOMPUTED_INDEX_FILE):
        with open(PRECOMPUTED_INDEX_FILE, 'w') as f:
            f.write(json.dumps({'dates': []}))

    with open(PRECOMPUTED_INDEX_FILE, 'r+') as f:
        index_json = json.loads(f.read())
        index_json['dates'].append(DAY)
        index_json['dates'].sort()
        f.seek(0)
        f.write(json.dumps(index_json, indent=4))

def calc_step_from_day(day: datetime.datetime):
    dt = int(day.timestamp())
    X = 30
    T0 = 0
    step = (dt - T0) // X
    return step

if __name__ == '__main__':
    try:
        assert len(sys.argv) == 2

        arg = sys.argv[1]
        if '-' in arg:
            arg_s, arg_e = arg.split('-')
            day_s = datetime.datetime.strptime(arg_s, '%Y/%m/%d')
            day_e = datetime.datetime.strptime(arg_e, '%Y/%m/%d')
        else:
            day_s = day_e = datetime.datetime.strptime(arg, '%Y/%m/%d')

    except Exception as e:
        print(e)
        print(f'Usage: py {__file__} YYYY/mm/dd')
        print(f'       py {__file__} YYYY/mm/dd-YYYY/mm/dd')
        exit()

    cday = day_s
    while cday <= day_e:
        print('-' * 64)
        print(f'precomputing... ({cday})')
        print('-' * 64)
        main(cday)
        cday += datetime.timedelta(1)
        print()
