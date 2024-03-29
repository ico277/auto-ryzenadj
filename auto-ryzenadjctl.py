#!/bin/env python3
import socket, struct
from typing import List, Dict
import traceback
import sys

VERSION = "0.0.1-dev"

global SOCKET_PATH
SOCKET_PATH = "/tmp/auto-ryzenadj.socket"

def connect() -> socket.socket:
    unix_socket = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    unix_socket.connect(SOCKET_PATH)
    return unix_socket

def get_status() -> str:
    try:
        unix_socket = connect()

        data = "AA"
        unix_socket.sendall(data.encode(encoding="ASCII"))

        length = struct.unpack('>I', unix_socket.recv(4))[0]
        return unix_socket.recv(length).decode(encoding="ASCII")
    except:
        traceback.print_exc()
        return "ERR - unknown communications error"
    try:
        unix_socket.close()
    except:
        pass

def get_profiles() -> List[Dict[str, List]]:
    try:
        unix_socket = connect()

        data = "AB"
        unix_socket.sendall(data.encode(encoding="ASCII"))
        length = struct.unpack('>I', unix_socket.recv(4))[0]

        data = unix_socket.recv(length).decode(encoding="ASCII")
        profiles = list()
        for line in data.splitlines():
            key, args = line.split(":", maxsplit=1)
            profiles.append({key: args.split(",")})
        return profiles
    except:
        traceback.print_exc()
        return "ERR - unknown communications error"
    try:
        unix_socket.close()
    except:
        pass

def set_profile(profile:str) -> str:
    try:
        unix_socket = connect()

        data = "BA"
        unix_socket.sendall(data.encode(encoding="ASCII"))      # send command
        unix_socket.send(struct.pack('>I', len(profile)))       # send length of profile
        unix_socket.sendall(bytes(profile, encoding="ASCII"))   # send profile

        length = struct.unpack('>I', unix_socket.recv(4))[0]
        return unix_socket.recv(length).decode(encoding="ASCII")
    except FileNotFoundError:
        return "ERR - daemon not running"
    except:
        traceback.print_exc()
        return "ERR - unknown communications error"
    try:
        unix_socket.close()
    except:
        pass 

def set_timer(timer:int) -> str:
    try:
        unix_socket = connect()

        data = "BB"
        unix_socket.sendall(data.encode(encoding="ASCII"))      # send command
        unix_socket.send(struct.pack('>I', timer))              # send int

        length = struct.unpack('>I', unix_socket.recv(4))[0]
        return unix_socket.recv(length).decode(encoding="ASCII")
    except FileNotFoundError:
        return "ERR - daemon not running"
    except:
        traceback.print_exc()
        return "ERR - unknown communications error"
    try:
        unix_socket.close()
    except:
        pass 

def help():
    print(f"""Usage: {sys.argv[0]} [ACTION] [...]

Action:
  --help, -h, help                Shows this message
  --version, -v                   Shows version information
  set <profile/timer> <...>       Sets profile/timer (in seconds)
  profiles                        Lists all available profiles and their arguments
  profile <profile>               Lists information about a specific profile\n
  
Optional:
  -s=<socket>, --socket=<socket>  Sets socket path
""")
    exit(0)    

def main():
    global SOCKET_PATH
    argv = sys.argv.copy()
    argv.pop(0)

    for arg in argv:
        if arg.startswith("-s=") or arg.startswith("--socket="):
            _, path = arg.split("=", maxsplit=1)
            SOCKET_PATH = path
            argv.remove(arg)

    result = None
    if len(argv) == 0:
        help()
    elif len(argv) == 1:
        match argv[0]:
            case "help" | "--help" | "-h":
                help()
            case "-v" | "--version":
                print("auto-ryzenadjctl v" + VERSION)
            case "status":
                status = get_status()
                if status.startswith("ERR"):
                    result = status
                else:
                    print(get_status().replace(":", " -> "))
            case "profiles":
                profiles = get_profiles()
                if type(profiles) == str:
                    result = profiles
                else:
                    for profile in profiles:
                        key = list(profile.keys())[0]
                        print(f'{key}: ryzenadj {" ".join(profile[key])}')
            case _:
                print(f'invalid arguments \'{" ".join(argv)}\'')
                exit(1)
    elif len(argv) > 2:
        match argv[0]:
            case "set":
                if argv[1] == "profile":
                    result = set_profile(argv[2])
                elif argv[1] == "timer":
                    result = set_timer(int(argv[2]))
                else:
                    print(f'invalid arguments \'{" ".join(argv)}\'')
                    exit(1)
            case _:
                print(f'invalid arguments \'{" ".join(argv)}\'')
                exit(1)
    elif len(argv) == 2:
        if argv[0] == "profile":
            profiles = get_profiles()
            if type(profiles) == str:
                result = profiles
            elif [i for i in profiles if argv[1] in i]:
                for profile in profiles:
                    if argv[1] in profile:
                        key = list(profile.keys())[0]
                        print(f'{key}: ryzenadj {" ".join(profile[key])}')
                        break
            else:
                print(f'Profile \'{argv[1]}\' does not exist')
                exit(1)
        else:
            print(f'invalid arguments \'{" ".join(argv)}\'')
            exit(1)         
    else:
        print(f'invalid arguments \'{" ".join(argv)}\'')
        exit(1)
    
    if result != None and result.startswith("ERR"):
        print("there was an error: " + result)

if __name__ == "__main__":
    main()
