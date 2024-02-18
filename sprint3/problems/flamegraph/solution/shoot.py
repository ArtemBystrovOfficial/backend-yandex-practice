import argparse
import subprocess
import signal
import time
import random
import shlex
import os

RANDOM_LIMIT = 1000
SEED = 123456789
random.seed(SEED)

AMMUNITION = [
    'localhost:8080/api/v1/maps/map1',
    'localhost:8080/api/v1/maps'
]

SHOOT_COUNT = 100
COOLDOWN = 0.1


def start_server():
    parser = argparse.ArgumentParser()
    parser.add_argument('server', type=str)
    return parser.parse_args().server

def start_prof(server):	
    return run('perf record -g -o perf.data ' + server)

def run(command, output=None):
    process = subprocess.Popen(shlex.split(command), stdout=output, stderr=subprocess.DEVNULL)
    return process
    
def runShell(command, output=None):
    process = subprocess.Popen(command,shell=True, stdout=output, stderr=subprocess.DEVNULL)
    return process

def stop(process, wait=False):
    if process.poll() is None and wait:
        process.wait()
    process.terminate()

def make_flame():
    process = subprocess.Popen(shlex.split('perf script -i perf.data'), stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)
    output = subprocess.Popen(('./FlameGraph/stackcollapse-perf.pl'), stdin=process.stdout, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)
    with open('graph.svg', 'w') as f:
    	output2 = subprocess.Popen(('./FlameGraph/flamegraph.pl'), stdin=output.stdout, stdout=f, stderr=subprocess.DEVNULL)
    	output2.wait()

def shoot(ammo):
    hit = run('curl ' + ammo, output=subprocess.DEVNULL)
    time.sleep(COOLDOWN)
    stop(hit, wait=True)


def make_shots():
    for _ in range(SHOOT_COUNT):
        ammo_number = random.randrange(RANDOM_LIMIT) % len(AMMUNITION)
        shoot(AMMUNITION[ammo_number])
    print('Shooting complete')


server = start_prof(start_server())
time.sleep(2)
make_shots()
time.sleep(1)
stop(server)
time.sleep(1)
make_flame()
time.sleep(1)
print('Job done')
