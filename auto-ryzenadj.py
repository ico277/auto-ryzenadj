#!/bin/env python3
import tomllib
import subprocess
from time import sleep
import socket
import os
import signal
import threading
import struct
import sys, traceback
import argparse
import logging
from datetime import datetime
from pathlib import Path

# globals
global CONFIG_PATH
global CONFIG
global EXIT
global VERSION
global PROFILE
global SOCKET_PATH
global UNIX_SOCKET
global LOG
global LOGFILE_OVERRIDE
CONFIG_PATH = "/etc/auto-ryzenadj.conf"
CONFIG = {}
EXIT = False
VERSION = "0.1.0-dev"
PROFILE = "default"
SOCKET_PATH = "/tmp/auto-ryzenadj.socket"
UNIX_SOCKET = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
LOG = logging.getLogger(__name__)
LOGFILE_OVERRIDE = None

def cleanup(_, __):
    global EXIT, UNIX_SOCKET, SOCKET_PATH
    EXIT = True
    UNIX_SOCKET.close()
    os.unlink(SOCKET_PATH)
    os._exit(0)

def read_socket():
    global EXIT, UNIX_SOCKET, LOG
    while not EXIT:
        try:
            connection, client_address = UNIX_SOCKET.accept()
            connection.settimeout(5)
        
            # read command from the client
            data = connection.recv(2)
            LOG.debug("Received data:", "'" + data.decode(encoding="ASCII") + "'")
            response = "OK"
            global PROFILE
            global CONFIG
            match data.decode('ASCII'):
                case "AA": # status
                    response = f'profile:{PROFILE}\ntimer:{CONFIG["main"]["timer"] if CONFIG["main"]["timer"] > 0 else 3}'
                case "AB": # detailed profile information
                    response = ""
                    for key in CONFIG["profiles"]:
                        response += f'{key}:{",".join(CONFIG["profiles"][key])}\n'
                case "BA": # set profile
                    data = connection.recv(4)
                    length = struct.unpack('>I', data)[0] # decode 4 byte unsigned int
                    data = connection.recv(length)
                    data = data.decode(encoding="ASCII")
                    if data in CONFIG["profiles"] or data == "auto":
                        PROFILE = data
                    else:
                        response = "ERR - invalid profile"
                case "BB": # set timer
                    data = connection.recv(4)
                    timer = struct.unpack('>I', data)[0] # decode 4 byte unsigned int
                    if timer < 1:
                        response = "ERR - invalid value range"
                    else:
                        CONFIG["main"]["timer"] = timer
                case _:
                    response = "ERR - invalid command"

            connection.send(struct.pack('>I', len(response))) # encode 4 byte unsigned int
            connection.sendall(bytes(response, "ASCII"))
            connection.close()

        except socket.timeout:
            pass
        except BrokenPipeError:
            pass
        except Exception:
            # critical error
            traceback.print_exc()
            EXIT = True
            cleanup(None, None)
            os._exit(1)

def ryzenadj():
    global PROFILE, LOG
    executable = "ryzenadj"
    if PROFILE == "default":
        PROFILE = CONFIG["main"]["default"]
    profile = PROFILE
    if profile == "auto":
        epp = Path("/sys/devices/system/cpu/cpu0/cpufreq/energy_performance_preference")
        sg = Path("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor")
        if epp.exists():
            profile = epp.read_text().strip()
        elif sg.exists():
            profile = sg.read_text().strip()
    if profile not in CONFIG["profiles"]:
        profile = CONFIG["profiles"].keys()[0]
    if "executable" in CONFIG["main"]:
        executable = CONFIG["main"]["executable"]
    LOG.debug(f'running ryzenadj -> {executable} {" ".join(CONFIG["profiles"][profile])}')
    output = subprocess.run([executable] + CONFIG["profiles"][profile], capture_output=True, text=True)
    LOG.debug(f'output stdout:\n{output.stdout}')
    LOG.debug(f'output stderr:\n{output.stderr}')

def main_loop():
    global EXIT, LOG
    while not EXIT:
        ryzenadj()      # runs ryzenadj
        if CONFIG["main"]["timer"] < 1:
            LOG.error(f'invalid value timer = \'{CONFIG["main"]["timer"]}\'!')
            sleep(3)
        else:
            sleep(CONFIG["main"]["timer"])

def init():
    global CONFIG_PATH, CONFIG, SOCKET_PATH, UNIX_SOCKET, LOGFILE_OVERRIDE, LOG
    
    LOG.debug("reading config file...")
    # read config
    with open(CONFIG_PATH, "rb") as f:
        CONFIG = tomllib.load(f)
    LOG.debug("done.")

    # init logging
    level = logging.DEBUG
    LOG = logging.getLogger(__name__)
    if "logging" in CONFIG:
        if "level" in CONFIG["logging"]:
            if CONFIG["logging"]["level"] == 0:
                level = logging.CRITICAL
            elif CONFIG["logging"]["level"] == 1:
                level = logging.INFO
            elif CONFIG["logging"]["level"] == 2:
                level = logging.WARNING
            elif CONFIG["logging"]["level"] == 3:
                level = logging.DEBUG
        if LOGFILE_OVERRIDE != None:
            logging.basicConfig(filename=LOGFILE_OVERRIDE,
                                encoding='utf-8', level=level,
                                format='[%(asctime)s %(levelname)s] %(message)s',
                                datefmt='%Y-%m-%d %H:%M:%S')
        elif "file" in CONFIG["logging"]:
            # get time formats
            cur_date = datetime.now()
            date = cur_date.strftime("%Y-%m-%d")
            time = cur_date.strftime("%H-%M-%S")
            file = CONFIG["logging"]["file"].replace("%date%", date).replace("%time%", time)
            
            # get directory from file path
            file_dir = os.path.dirname(file)
            # create if it doesn't exist
            os.makedirs(file_dir, exist_ok=True)
            
            logging.basicConfig(filename=file,
                                encoding='utf-8', level=level,
                                format='[%(asctime)s %(levelname)s] %(message)s',
                                datefmt='%Y-%m-%d %H:%M:%S')
        else:
            logging.basicConfig(encoding='utf-8', level=level,
                                format='[%(asctime)s %(levelname)s] %(message)s',
                                datefmt='%Y-%m-%d %H:%M:%S')
        LOG.setLevel(level)
#    LOG.debug('This is a debug message')    # lowest level
#    LOG.info('This is an info message')
#    LOG.warning('This is a warning message')
#    LOG.error('This is an error message')
#    LOG.critical('This is a critical message')  # highest level


    LOG.debug("opening socket...")
    # try to connect to SOCKET_PATH to check if another instance is running
    try:
        sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        sock.connect(SOCKET_PATH)
        sock.sendall("AA".encode(encoding="ASCII"))
        sock.recv(1)
        # if there is a response, exit
        LOG.critical("Error socket already opened (another instance running?)")
        os._exit(1)
    except:
        try:
            os.unlink(SOCKET_PATH)
        except:
            pass
        pass
    # open socket
    UNIX_SOCKET = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    UNIX_SOCKET.bind(SOCKET_PATH)
    UNIX_SOCKET.listen(5)
    LOG.debug("done.")


def main():
    global CONFIG_PATH, SOCKET_PATH, LOGFILE_OVERRIDE
    parser = argparse.ArgumentParser(prog='auto-ryzenadj', description='Automatically applies custom ryzenadj profiles')
    parser.add_argument("-c", "--config", action="store", help=f"Config file path (default: {CONFIG_PATH})", required=False)
    parser.add_argument("-s", "--socket", action="store", help=f"Socket file path (default: {SOCKET_PATH})", required=False)
    parser.add_argument("-l", "--logfile", action="store", help=f"Log file path (default: none)", required=False)
    args = parser.parse_args()

    if args.config != None:
        CONFIG_PATH = args.config
    if args.socket != None:
        SOCKET_PATH = args.socket
    if args.logfile != None:
        LOGFILE_OVERRIDE = args.logfile

    init()

    # create and start the socket thread
    LOG.debug("starting the socket thread...")
    socket_thread = threading.Thread(target=read_socket)
    socket_thread.start()
    LOG.debug("done.")

    LOG.debug("setting signal handlers...")
    # ensure a clean shutdown
    signal.signal(signal.SIGINT, cleanup)
    signal.signal(signal.SIGTERM, cleanup)
    LOG.debug("done.")

    LOG.debug("running main loop...")
    # main loop
    main_loop()
    LOG.debug("main loop exited")

    # ensure cleanup is called before exiting
    cleanup(None, None)

if __name__ == "__main__":
    main()
