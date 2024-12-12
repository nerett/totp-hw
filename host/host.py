import time
from typing import List, Optional

import serial

import config
from command import Command


class Site:
    def __init__(self, id_: str, name: str):
        self.id_ = id_
        self.name = name


sites: List[Site] = [
    Site('1', 'github.com$vihlancevk'),
    Site('2', 'totp.danhersam.com')
]

try:
    ser = serial.Serial(port=config.port, baudrate=config.baud_rate, timeout=config.timeout)
except serial.SerialException as se:
    print(f"Failed to open serial port: {se}")
    exit(1)


def choose_site() -> Optional[Site]:
    print('Choose the site:')

    ser.write(Command.GET_LIST_OF_SITES_ID.value)

    line: bytes = ser.readline()
    if not line:
        print("No response from Serial.")
        return None

    if getattr(config, "DEBUG", False):
        print(line)

    ids: List[str] = line.decode().strip().split(',')

    available_sites = [site for site in sites if site.id_ in ids]
    if not available_sites:
        print("No sites available for selection.")
        return None

    for i, site in enumerate(available_sites):
        print(f"{i + 1}. {site.name}")

    while True:
        try:
            choice = int(input('Input number: ')) - 1
            if 0 <= choice < len(available_sites):
                print(f"Chosen site: {available_sites[choice].name}")
                return available_sites[choice]
            raise ValueError
        except ValueError:
            print("Incorrect number. Try again.")


def get_site_auth_code() -> str:
    ser.write(Command.GET_SITE_AUTH_CODE.value)

    current_time: int = int(time.time())
    ser.write(f'{site.id_}\n{site.name}\n{current_time}\n'.encode())

    line = ser.readline()

    if not line:
        print("No response from Serial.")
        return ""

    return line.decode().strip('\r\n')


if __name__ == '__main__':
    try:
        site = choose_site()
        if site:
            site_auth_code = get_site_auth_code()
            print(f"Auth code for {site.name}: '{site_auth_code}'")
    except KeyboardInterrupt:
        print("\nInterrupted by user.")
    finally:
        if ser.is_open:
            ser.close()
