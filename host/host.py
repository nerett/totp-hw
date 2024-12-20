import time
import csv
from typing import Optional, Tuple, List, Any

import serial
import pyperclip

import config
from command import Command


class Site:
    def __init__(self, id_: str, name: str, login: str) -> None:
        self.id_: str = id_
        self.name: str = name
        self.login: str = login

    def __repr__(self) -> str:
        return f"Site(id_={id_}, name={name}, login={login})"


sites: List[Site] = []
with open(config.Database.name, "r") as file:
    reader = csv.reader(file)

    row: str
    for row in reader:
        id_: str = row[0]
        name: str = row[1]
        login: str = row[2]
        sites.append(Site(id_, name, login))

try:
    ser = serial.Serial(port=config.Serial.port, baudrate=config.Serial.baud_rate, timeout=config.Serial.timeout)
except serial.SerialException as se:
    print(f"Failed to open serial port: {se}")
    exit(1)


def go_to_previous_line() -> None:
    print("\033[F", end="")
    print(" " * 30, end="\r")


def get_line_from_serial() -> Optional[str]:
    line = ser.readline()
    if not line:
        print("No response from Serial")
        return None

    return line.decode().strip('\r\n')


def choose(variants: List[Any]) -> Any:
    while True:
        choice = input("> ")

        if not choice.isdigit() or not (1 <= int(choice) <= len(commands)):
            print("Invalid input. Please enter a valid number.")
            continue

        return variants[int(choice) - 1]


def choose_site() -> Optional[Site]:
    if not sites:
        return None

    print("Choose the site:")

    i: int
    site: Site
    for i, site in enumerate(sites):
        print(f"{i + 1}. {site.name}?login={site.login}")

    chosen_site: Site = choose(sites)
    go_to_previous_line()
    print(f"Chosen site: {chosen_site.name}?login={chosen_site.login}")

    return chosen_site


def set_time() -> None:
    ser.write(Command.SET_TIME.value)

    current_time_in_sec: int = int(time.time())
    ser.write(f"{current_time_in_sec}\n".encode())

    line: str = get_line_from_serial()
    if not line:
        return

    print(line)


def add_site() -> None:
    id_: str = str(len(sites))
    name: str = input("Input site name: ")
    login: str = input("Input site login: ")
    site: Site = Site(id_, name, login)

    sites.append(site)
    with open(config.Database.name, 'a') as file:
        file.write(f"{id_},{name},{login}\n")

    encoded_code: str = input("Input site token: ")

    ser.write(Command.ADD_SITE.value)
    ser.write(f'{site.id_}\n{site.name}\n{site.login}\n{encoded_code}\n'.encode())

    line: str = get_line_from_serial()
    if not line:
        return

    print(line)


def get_otp() -> Tuple[Optional[Site], Optional[str]]:
    site: Optional[Site] = choose_site()
    if not site:
        print("No sites")
        return None, None

    ser.write(Command.GET_OTP.value)
    ser.write(f'{site.id_}\n{site.name}\n{site.login}\n'.encode())

    line: str = get_line_from_serial()
    if not line:
        return site, None

    pyperclip.copy(line)

    return site, line


def erase_db() -> None:
    ser.write(Command.ERASE_DB.value)

    global sites
    sites = []

    with open(config.Database.name, 'w'):
        pass

    line: str = get_line_from_serial()
    if not line:
        return

    print(line)


if __name__ == '__main__':
    try:
        commands: List[Command] = [command for command in Command]

        print('Available actions:')

        i: int
        command: Command
        for i, command in enumerate(commands):
            print(f"{i + 1}. {command.name}")

        print()

        while True:
            print("Choose the action:")

            chosen_command: Command = choose(commands)
            go_to_previous_line()
            print(f'> {chosen_command.name}')

            if chosen_command == Command.SET_TIME:
                set_time()
            elif chosen_command == Command.ADD_SITE:
                add_site()
            elif chosen_command == Command.GET_OTP:
                site: Site
                otp: str
                site, otp = get_otp()
                if site and otp:
                    print(f"OTP for {site.name}?login={site.login}: {otp}")
            elif chosen_command == Command.ERASE_DB:
                erase_db()
            else:
                print("Unknown command")
    except (KeyboardInterrupt, EOFError):
        print("\nInterrupted by user")
    finally:
        if ser.is_open:
            ser.close()
