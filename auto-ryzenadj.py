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

# globals
global CONFIG_PATH
global CONFIG
global EXIT
global VERSION
global PROFILE
global SOCKET_PATH
global UNIX_SOCKET
CONFIG_PATH = "/etc/auto-ryzenadj.conf"
CONFIG = {}
EXIT = False
VERSION = "0.0.1-dev"
PROFILE = "default"
SOCKET_PATH = "/tmp/auto-ryzenadj.socket"
UNIX_SOCKET = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)

def cleanup(_, __):
    global EXIT, UNIX_SOCKET, SOCKET_PATH
    EXIT = True
    UNIX_SOCKET.close()
    os.unlink(SOCKET_PATH)
    os._exit(0)

def read_socket():
    global EXIT, UNIX_SOCKET
    while not EXIT:
        try:
            connection, client_address = UNIX_SOCKET.accept()
            connection.settimeout(5)
        
            # read command from the client
            data = connection.recv(2)
            print("Received data:", "'" + data.decode(encoding="ASCII") + "'")
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
                    if data in CONFIG["profiles"]:
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
    global PROFILE
    executable = "ryzenadj"
    if PROFILE == "default":
        PROFILE = CONFIG["main"]["default"]
    if "executable" in CONFIG["main"]:
        executable = CONFIG["main"]["executable"]
    print(f'-> {[executable] + CONFIG["profiles"][PROFILE]}')
    output = subprocess.run([executable] + CONFIG["profiles"][PROFILE], capture_output=True, text=True)
    print(f'output stdout:\n{output.stdout}')
    print(f'output stderr:\n{output.stderr}')

def main_loop():
    global EXIT
    while not EXIT:
        ryzenadj()      # runs ryzenadj
        if CONFIG["main"]["timer"] < 1:
            print(f'invalid value timer = \'{CONFIG["main"]["timer"]}\'!')
            sleep(3)
        else:
            sleep(CONFIG["main"]["timer"])

def init():
    global CONFIG_PATH, CONFIG, SOCKET_PATH, UNIX_SOCKET
    # read config
    with open(CONFIG_PATH, "rb") as f:
        CONFIG = tomllib.load(f)

    # try to connect to SOCKET_PATH to check if another instance is running
    try:
        sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        sock.connect(SOCKET_PATH)
        sock.sendall("AA".encode(encoding="ASCII"))
        sock.recv(1)
        # if there is a response, exit
        print("Error socket already opened (another instance running?)")
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


def main():
    global CONFIG_PATH, SOCKET_PATH
    parser = argparse.ArgumentParser(prog='auto-ryzenadj', description='Automatically applies custom ryzenadj profiles')
    parser.add_argument("-c", "--config", action="store", help=f"Config file path (default: {CONFIG_PATH})", required=False)
    parser.add_argument("-s", "--socket", action="store", help=f"Socket file path (default: {SOCKET_PATH})", required=False)
    args = parser.parse_args()

    if args.config != None:
        CONFIG_PATH = args.config
    if args.socket != None:
        SOCKET_PATH = args.socket

    init()

    # create and start the socket thread
    socket_thread = threading.Thread(target=read_socket)
    socket_thread.start()

    # ensure a clean shutdown
    signal.signal(signal.SIGINT, cleanup)
    signal.signal(signal.SIGTERM, cleanup)

    # main loop
    main_loop()

    # ensure cleanup is called before exiting
    cleanup(None, None)

if __name__ == "__main__":
    main()
